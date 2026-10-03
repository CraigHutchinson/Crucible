"""Run configured CTest cases after repairing restored, owned native test artifacts.

Only commands selected by CTest beneath this repository's build directory qualify.
Source files, symlinks, external tools and non-native files keep their original modes.
Test failures propagate; the runner never retries a suite automatically.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import stat
import subprocess
import sys


def repair_permissions(tests: list[dict], artifact_root: Path) -> list[Path]:
    """Restore execute bits from read bits for owned configured native commands only."""
    if os.name != "posix":
        return []
    root = artifact_root.resolve()
    repaired = []
    seen = set()
    for test in tests:
        command = test.get("command", [])
        if not command:
            continue
        path = Path(command[0])
        if not path.is_absolute() or path.is_symlink():
            continue
        resolved = path.resolve()
        if not resolved.is_relative_to(root) or resolved in seen:
            continue
        seen.add(resolved)
        try:
            info = resolved.stat()
        except FileNotFoundError:
            # Missing artifacts belong to CTest's ordinary Not Run diagnostics.
            continue
        if (not stat.S_ISREG(info.st_mode) or info.st_uid != os.geteuid()
                or info.st_mode & stat.S_IXUSR):
            continue
        with resolved.open("rb") as binary:
            if binary.read(4) not in (
                    b"\x7fELF", b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe",
                    b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe",
                    b"\xbe\xba\xfe\xca", b"\xca\xfe\xba\xbf", b"\xbf\xba\xfe\xca"):
                continue
        mode = stat.S_IMODE(info.st_mode)
        resolved.chmod(mode | ((mode & 0o444) >> 2))
        repaired.append(resolved)
    return repaired


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preset", required=True)
    parser.add_argument("--ctest", default="ctest", help="CTest executable, when absent from PATH")
    args, ctest_options = parser.parse_known_args()
    repo = Path(__file__).resolve().parents[1]
    command = [args.ctest, "--preset", args.preset, *ctest_options]
    try:
        if os.name == "posix":
            inventory = subprocess.run(command + ["--show-only=json-v1"], cwd=repo,
                                       check=True, capture_output=True, text=True)
            tests = json.loads(inventory.stdout)["tests"]
            for path in repair_permissions(tests, repo / "build"):
                print(f"Restored test executable permission: {path.relative_to(repo)}", flush=True)
        return subprocess.run(command, cwd=repo).returncode
    except subprocess.CalledProcessError as error:
        print(error.stdout or "", file=sys.stderr, end="")
        print(error.stderr or "", file=sys.stderr, end="")
        return error.returncode
    except (OSError, ValueError, KeyError) as error:
        print(f"Test preflight failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
