"""Capture the optional offscreen receiver with source/tool/shader provenance."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--vertex", type=Path, required=True)
    parser.add_argument("--fragment", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--vulkaninfo", default="vulkaninfo")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    command = [str(args.executable.resolve()), str(args.vertex.resolve()),
               str(args.fragment.resolve()), str(args.output.resolve()), "--palette"]
    subprocess.run(command, cwd=repo, check=True)
    frames = json.loads((args.output / "frames.json").read_text())
    inputs = [args.executable, args.vertex, args.fragment,
              repo / "src/presentation/gpu/shaders/world.vert",
              repo / "src/presentation/gpu/shaders/world.frag"]
    identity = args.vertex.parent.parent / "gpu-shader-toolchain.txt"
    metadata = {
        "source_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip(),
        "committed_source_tree": subprocess.check_output(["git", "rev-parse", "HEAD^{tree}"], cwd=repo, text=True).strip(),
        "source_dirty": bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=repo, text=True)),
        "os": platform.platform(), "command": command,
        "shader_compiler": identity.read_text(),
        "device_inventory": subprocess.check_output([args.vulkaninfo, "--summary"], text=True),
        "dependency_pins": (repo / "cmake/DependencyPins.cmake").read_text(),
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
        "capture_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in args.output.iterdir() if p.is_file()},
        "same_input_pair": "initial.bmp and initial-software.bmp use the same completed tick0 snapshot and fitted camera; compare world rectangle [24,96,1232,520]",
        "applied_trace": frames["applied_trace"],
        "scope": "Actual offscreen world; synthetic palette is presentation only; no faction or physical-GPU performance claim.",
    }
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")


if __name__ == "__main__":
    main()
