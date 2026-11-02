#!/usr/bin/env python3
"""Compile and exercise the production driver with host substitutes for ZMK APIs."""

import os
from pathlib import Path
import subprocess
import tempfile


def main():
    test_dir = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="linea40-profile-key-") as directory:
        temp = Path(directory)
        for name in (
            "zephyr/device.h",
            "drivers/behavior.h",
            "zmk/behavior.h",
            "zmk/ble.h",
            "zmk/events/keycode_state_changed.h",
        ):
            header = temp / name
            header.parent.mkdir(parents=True, exist_ok=True)
            header.write_text('#include "support.h"\n')

        for metadata in (0, 1):
            binary = temp / f"test-{metadata}"
            subprocess.run(
                [
                    os.environ.get("CC", "cc"),
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-fsanitize=address,undefined",
                    f"-DCONFIG_ZMK_BEHAVIOR_METADATA={metadata}",
                    "-I", str(temp),
                    "-I", str(test_dir),
                    str(test_dir / "test.c"),
                    "-o", str(binary),
                ],
                check=True,
            )
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
