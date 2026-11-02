"""Exercise the Actions sync script against a local bare Git remote."""

import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PREVIEW = "config/LiNEA40-OS-preview.keymap"
SOURCE = "config/LiNEA40.keymap"
SCRIPT = "scripts/update_os_preview.sh"


class PreviewSyncTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="linea40-preview-sync-")
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        self.remote = self.directory / "origin.git"
        self.seed = self.directory / "editor"
        self.worker = self.directory / "actions"
        self.env = {
            **os.environ,
            "GIT_CONFIG_GLOBAL": os.devnull,
            "GIT_CONFIG_NOSYSTEM": "1",
            "GIT_AUTHOR_NAME": "Test Editor",
            "GIT_AUTHOR_EMAIL": "test@example.com",
            "GIT_COMMITTER_NAME": "Test Editor",
            "GIT_COMMITTER_EMAIL": "test@example.com",
            "GITHUB_ACTIONS": "true",
            "GITHUB_REF": "refs/heads/main",
        }
        self.run_command(
            self.directory, "git", "init", "--bare", "--initial-branch=main", str(self.remote)
        )
        self.run_command(self.directory, "git", "init", "--initial-branch=main", str(self.seed))
        for name in (
            SOURCE,
            PREVIEW,
            "config/LiNEA40.json",
            "config/LiNEA40-OS-preview.json",
            "dts/behaviors/linea40_profile_keys.dtsi",
            "scripts/generate_os_preview.py",
            SCRIPT,
        ):
            target = self.seed / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)
        self.git(self.seed, "add", ".")
        self.git(self.seed, "commit", "-m", "Initial configuration")
        self.git(self.seed, "remote", "add", "origin", str(self.remote))
        self.git(self.seed, "push", "-u", "origin", "main")
        self.save_editor_change("&kp Q", "&kp ESC", push=True)
        self.run_command(self.directory, "git", "clone", str(self.remote), str(self.worker))

    def run_command(self, cwd, *args, check=True, env=None):
        result = subprocess.run(
            args, cwd=cwd, env=env or self.env, text=True, capture_output=True, check=False
        )
        if check and result.returncode:
            self.fail(f"{args} failed:\n{result.stdout}\n{result.stderr}")
        return result

    def git(self, cwd, *args):
        return self.run_command(cwd, "git", *args).stdout.strip()

    def save_editor_change(self, old, new, *, push):
        path = self.seed / SOURCE
        path.write_text(path.read_text().replace(old, new, 1))
        self.git(self.seed, "add", SOURCE)
        self.git(self.seed, "commit", "-m", "Update keymap from editor")
        if push:
            self.git(self.seed, "push", "origin", "main")

    def test_update_is_scoped_and_second_run_is_a_noop(self):
        self.run_command(self.worker, "bash", SCRIPT)
        preview = self.git(self.remote, "show", f"main:{PREVIEW}")
        self.assertEqual(preview.count("&kp ESC"), 2)
        self.assertEqual(
            self.git(self.remote, "diff-tree", "--no-commit-id", "--name-only", "-r", "main"),
            PREVIEW,
        )
        first_head = self.git(self.remote, "rev-parse", "main")
        self.run_command(self.worker, "bash", SCRIPT)
        self.assertEqual(self.git(self.remote, "rev-parse", "main"), first_head)

    def test_concurrent_save_is_preserved_and_preview_is_regenerated(self):
        self.save_editor_change("&kp W", "&kp F13", push=False)
        marker = self.directory / "race-triggered"
        hook = self.worker / ".git/hooks/post-commit"
        push = shlex.join(["git", "-C", str(self.seed), "push", "origin", "main"])
        hook.write_text(
            "#!/usr/bin/env bash\nset -eu\n"
            f"if [[ ! -e {shlex.quote(str(marker))} ]]; then\n"
            f"  touch {shlex.quote(str(marker))}\n"
            "  unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_PREFIX\n"
            f"  {push}\n"
            "fi\n"
        )
        hook.chmod(0o755)
        result = self.run_command(self.worker, "bash", SCRIPT)
        self.assertIn("refreshing the source", result.stderr)
        source = self.git(self.remote, "show", f"main:{SOURCE}")
        preview = self.git(self.remote, "show", f"main:{PREVIEW}")
        self.assertIn("&kp F13", source)
        self.assertEqual(preview.count("&kp ESC"), 2)
        self.assertEqual(preview.count("&kp F13"), 2)
        self.run_command(self.worker, "python3", "scripts/generate_os_preview.py", "--check")

    def test_refuses_local_or_non_main_checkout_without_changing_files(self):
        original_head = self.git(self.worker, "rev-parse", "HEAD")
        source = self.worker / SOURCE
        source.write_text(source.read_text() + "\n// Local unsaved change\n")
        original_source = source.read_text()
        for override in ({"GITHUB_ACTIONS": "false"}, {"GITHUB_REF": "refs/heads/feature"}):
            result = self.run_command(
                self.worker, "bash", SCRIPT, env={**self.env, **override}, check=False
            )
            self.assertEqual(result.returncode, 2)
            self.assertEqual(self.git(self.worker, "rev-parse", "HEAD"), original_head)
            self.assertEqual(source.read_text(), original_source)


if __name__ == "__main__":
    unittest.main()
