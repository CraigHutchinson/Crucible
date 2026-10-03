"""Capture actual Phase 7 diagnostics and visualize retained observations.

Pillow/Matplotlib are capture dependencies only. Output directories must be new;
the receiving executable owns cases, validation and raw evidence production.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import platform
import subprocess


def plot_mission(results: dict, destination: Path) -> None:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    if results["case_count"] != 3 or len(results["cases"]) != 3:
        raise ValueError("The frozen investigation requires all three cases")
    figure, axes = plt.subplots(2, 1, figsize=(9, 6.5), layout="constrained")
    for row, color in zip(results["cases"], ("#0891b2", "#d97706", "#9333ea")):
        checkpoints = row["checkpoints"]
        label = f"{row['case']}: {row['outcome']} at {row['tick']}, {len(row['trace'])} edits"
        for axis in axes:
            axis.plot([p["tick"] for p in checkpoints], [p["reclaimed"] for p in checkpoints],
                      label=label, color=color, linewidth=2, marker=".")
    for axis in axes:
        axis.axhline(results["target"], color="#dc2626", linestyle="--", label="unchanged quota")
        axis.set(xlim=(0, results["deadline"]), ylabel="Reclaimed biomass")
        axis.grid(alpha=.18)
    axes[0].set(title="Frozen command-timing investigation: actual replay-verified observations", ylim=(0, 1900))
    axes[0].legend(loc="lower right", fontsize=9)
    axes[1].set(title="Near the quota (magnified; lines connect retained checkpoints)",
                xlabel="Completed simulation tick", ylim=(1745, 1785))
    figure.savefig(destination, dpi=160)
    plt.close(figure)


def plot_events(source: Path, destination: Path) -> None:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    with source.open(newline="") as stream:
        reader = csv.DictReader(stream)
        fields = reader.fieldnames
        rows = list(reader)
    if not fields or not rows or len(rows) > 100:
        raise ValueError("Missing or unbounded receiving event records")
    figure, axis = plt.subplots(figsize=(10, max(3, .32 * len(rows) + 1.5)), layout="constrained")
    axis.axis("off")
    axis.set_title("Executing receiver: controlled retirement event order\n"
                   "Queries deliberately withheld; event order is not GPU time or performance")
    table = axis.table(cellText=[[row[field] for field in fields] for row in rows],
                       colLabels=fields, cellLoc="left", loc="center")
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1, 1.5)
    figure.savefig(destination, dpi=160)
    plt.close(figure)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("mission", "gpu"))
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--vertex", type=Path)
    parser.add_argument("--fragment", type=Path)
    parser.add_argument("--vulkaninfo", default="vulkaninfo")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    executable, output = args.executable.resolve(), args.output.resolve()
    if output.exists():
        parser.error("Output directory must be new")
    inputs = [executable]
    if args.mode == "mission":
        command = [str(executable), "--study", "--export", str(output)]
        inputs.append(repo / "tests/integration/mission_sensitivity.cpp")
    else:
        if not args.vertex or not args.fragment:
            parser.error("GPU capture needs both shader files")
        inputs.extend((args.vertex.resolve(), args.fragment.resolve()))
        inputs.extend(repo / path for path in (
            "src/presentation/gpu/OffscreenRenderer.cpp",
            "tests/presentation/gpu/FaultTests.cpp",
            "tests/presentation/gpu/FaultController.cpp",
            "tests/presentation/gpu/FaultController.hpp"))
        command = [str(executable), str(inputs[1]), str(inputs[2]), "--export", str(output)]
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
    tree = subprocess.check_output(["git", "rev-parse", "HEAD^{tree}"], cwd=repo, text=True).strip()
    dirty = bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=repo, text=True))
    subprocess.run(command, cwd=repo, check=True)
    from PIL import Image, __version__ as pillow_version
    import matplotlib
    images = sorted(output.glob("*.bmp"))
    results = None
    expected_images = 3
    if args.mode == "mission":
        results = json.loads((output / "results.json").read_text())
        expected_images = sum(1 + any(p["tick"] == 60 for p in row["checkpoints"])
                              for row in results["cases"])
    if len(images) != expected_images:
        raise ValueError(f"Expected {expected_images} completed-state captures, found {len(images)}")
    for source in images:
        with Image.open(source) as frame:
            if frame.size != (1280, 720):
                raise ValueError("Unexpected receiving canvas dimensions")
            frame.save(source.with_suffix(".png"))
    if args.mode == "mission":
        plot_mission(results, output / "timing-comparison.png")
    else:
        plot_events(output / "events.csv", output / "retirement-events.png")
    metadata = {
        "source_commit": revision, "committed_source_tree": tree, "source_dirty": dirty,
        "os": platform.platform(), "command": command, "logical_canvas": [1280, 720],
        "capture_tools": {"python": platform.python_version(), "pillow": pillow_version,
                          "matplotlib": matplotlib.__version__},
        "backend": "production software ScenePainter" if args.mode == "mission" else "executing SDL_GPU Vulkan receiver; controlled delayed queries",
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
        "capture_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in output.iterdir() if p.is_file()},
        "dependency_pins": (repo / "cmake/DependencyPins.cmake").read_text(),
        "scope": "Actual diagnostic outputs; injected faults/retirement are not hardware device-loss or performance evidence. Automated mission replay is not human validation.",
    }
    cache = executable
    while cache != cache.parent and not (cache / "CMakeCache.txt").is_file():
        cache = cache.parent
    if (cache / "CMakeCache.txt").is_file():
        metadata["build_cache"] = (cache / "CMakeCache.txt").read_text()
    if args.mode == "gpu":
        metadata["device_inventory"] = subprocess.check_output([args.vulkaninfo, "--summary"], text=True)
        metadata["shader_compiler"] = (cache / "gpu-shader-toolchain.txt").read_text()
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")


if __name__ == "__main__":
    main()
