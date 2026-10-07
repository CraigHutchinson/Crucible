"""Capture actual applied/preview flow states through the production SDL painter."""

import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    repo = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    from PIL import Image

    output.mkdir(parents=True, exist_ok=False)
    captures = []
    for view in ("fit", "zoom"):
        bitmap = output / f"flow-{view}.bmp"
        result = subprocess.run(
            [str(executable), f"--export-flow-{view}", str(bitmap)],
            cwd=repo, check=True, capture_output=True, text=True, timeout=60,
        )
        png = bitmap.with_suffix(".png")
        with Image.open(bitmap) as frame:
            if frame.size != (1280, 864):
                raise ValueError("Unexpected flow fixture canvas size")
            frame.save(png)
            size = list(frame.size)
        captures.append({"view": view, "png": png.name, "dimensions": size,
                         "bmp_sha256": digest(bitmap), "png_sha256": digest(png),
                         "stdout": result.stdout, "stderr": result.stderr})
    metadata = {
        "source_commit": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip(),
        "dirty": bool(subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=repo, text=True)),
        "executable_sha256": digest(executable),
        "platform": platform.system(),
        "source_tree": subprocess.check_output(
            ["git", "rev-parse", "HEAD^{tree}"], cwd=repo, text=True).strip(),
        "dependency_pins_sha256": digest(repo / "cmake/DependencyPins.cmake"),
        "fixture_sha256": digest(repo / "tests/presentation/desktop/ScenePainterTests.cpp"),
        "painter_sha256": digest(repo / "src/presentation/desktop/ScenePainter.cpp"),
        "backend": "production SDL software painter, owned completed tick 60",
        "scenario": {"samples": 2048, "grid": [64, 32, 1], "tick": 60,
                     "applied": ["FLOW slot0 (12,12)->(44,12), radius8 strength4",
                                 "ATTRACT slot1 (24,24), radius8 strength4",
                                 "REPEL slot2 (52,24), radius8 strength-4"],
                     "preview": "FLOW slot3 (18,20)->(46,28), radius8 strength4",
                     "zoom_factor": 2},
        "scope": "Applied flow and radial fields; dashed uncommitted preview; fitted and zoomed camera. No human, hardware or performance acceptance.",
        "captures": captures,
    }
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")


if __name__ == "__main__":
    main()
