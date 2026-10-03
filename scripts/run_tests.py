"""Run configured CTest cases after repairing restored, owned native test artifacts.

Only selected CTest commands and explicitly declared native prerequisites beneath
this repository's build directory qualify.
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

from check_prerequisites import check_prerequisites


def repair_permissions(tests: list[dict], artifact_root: Path) -> list[Path]:
    """Restore execute bits from read bits for owned configured native commands/prerequisites only."""
    if os.name != "posix":
        return []
    root = artifact_root.resolve()
    repaired = []
    seen = set()
    candidates = []
    for test in tests:
        command = test.get("command", [])
        if command:
            candidates.append(command[0])
        # CTest declares native subprocess prerequisites explicitly; arbitrary
        # argv paths are never interpreted as executable artifacts.
        for prop in test.get("properties", []):
            if prop.get("name") == "REQUIRED_FILES":
                value = prop.get("value", [])
                if isinstance(value, list):
                    candidates.extend(value)
    for candidate in candidates:
        path = Path(candidate)
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
    parser.add_argument("--prerequisite-build-dir", type=Path,
                        help="Explicit configured build directory for custom/UserPresets")
    parser.add_argument("--prerequisite-report", type=Path)
    parser.add_argument("--require-hardware", action="store_true")
    args, ctest_options = parser.parse_known_args()
    repo = Path(__file__).resolve().parents[1]
    command = [args.ctest, "--preset", args.preset, *ctest_options]
    try:
        # Listing is inspection, not test execution or device acceptance.
        if any(option == "-N" or option.startswith("--show-only") for option in ctest_options):
            return subprocess.run(command, cwd=repo).returncode
        inventory = subprocess.run(command + ["--show-only=json-v1"], cwd=repo,
                                   check=True, capture_output=True, text=True)
        tests = json.loads(inventory.stdout)["tests"]
        for path in repair_permissions(tests, repo / "build"):
            print(f"Restored test executable permission: {path.relative_to(repo)}", flush=True)
        build_dir = args.prerequisite_build_dir
        if build_dir is None:
            presets = json.loads((repo / "CMakePresets.json").read_text())
            preset = next((item for item in presets["testPresets"] if item["name"] == args.preset), None)
            if preset is None or not preset.get("configurePreset"):
                raise ValueError("Custom test preset requires --prerequisite-build-dir")
            # Repository presets share base's build/<configurePreset> layout.
            build_dir = repo / "build" / preset["configurePreset"]
        receipt = check_prerequisites(build_dir, tests, stage="test",
                                      require_hardware=args.require_hardware)
        rendered = json.dumps(receipt, indent=2)
        print(rendered, flush=True)
        if args.prerequisite_report:
            args.prerequisite_report.parent.mkdir(parents=True, exist_ok=True)
            args.prerequisite_report.write_text(rendered + "\n")
        if not receipt["ok"]:
            print("Test prerequisites blocked execution; see receipt above.", file=sys.stderr)
            return 1
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
