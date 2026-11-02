#!/usr/bin/env bash
set -euo pipefail

# Only run in the disposable Actions checkout; refreshing it discards local changes.
if [[ "${GITHUB_ACTIONS:-}" != "true" || "${GITHUB_REF:-}" != "refs/heads/main" ]]; then
    echo "Run this script only in the main-branch GitHub Actions checkout." >&2
    exit 2
fi

git config user.name "github-actions[bot]"
git config user.email "41898282+github-actions[bot]@users.noreply.github.com"

for attempt in 1 2 3; do
    # Recompute from the latest source after a concurrent editor save. Never force-push.
    git fetch origin main
    git reset --hard origin/main
    python3 scripts/generate_os_preview.py
    git add -- config/LiNEA40-OS-preview.keymap config/LiNEA40-OS-preview.json

    if git diff --cached --quiet; then
        echo "OS preview is already up to date."
        exit 0
    fi

    git commit -m "Update OS keymap comparison"
    # The checkout's GITHUB_TOKEN prevents this push from triggering more workflows.
    if git push origin HEAD:refs/heads/main; then
        echo "OS preview updated."
        exit 0
    fi

    echo "Push failed on attempt ${attempt}; refreshing the source before retrying." >&2
done

echo "Could not push the OS preview after three attempts." >&2
exit 1
