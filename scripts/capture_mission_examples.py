"""Capture fixed mission examples and plot their retained replay-verified observations.

Requires Pillow and Matplotlib only for visual artifacts, never for the C++ tests.
The executable owns scenario/strategy rules and refuses an existing output directory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def plot_results(results, destination):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    figure, axes = plt.subplots(2, 1, figsize=(9, 6.5), layout="constrained")
    colors = ["#64748b", "#0891b2", "#d97706", "#9333ea"]
    for strategy, color in zip(results["strategies"], colors):
        rows = strategy["checkpoints"]
        label = f"{strategy['strategy']} ({strategy['outcome']} at {strategy['tick']})"
        for axis in axes:
            axis.plot([row["tick"] for row in rows], [row["reclaimed"] for row in rows],
                      label=label, color=color, linewidth=2, marker=".")
    for axis in axes:
        axis.axhline(results["target"], color="#dc2626", linestyle="--", linewidth=1, label="quota 1,780")
        axis.set(ylabel="Reclaimed biomass")
        axis.grid(alpha=.18)
    axes[0].set(title="Fixed strategies: replay-verified checkpoints", xlim=(0, 900), ylim=(0, 1900))
    axes[0].legend(loc="lower right", fontsize=9)
    axes[1].set(title="Near the quota (magnified)", xlabel="Completed tick", xlim=(60, 900), ylim=(1745, 1785))
    figure.savefig(destination, dpi=160)
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    executable = args.executable.resolve()
    output = args.output.resolve()
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
    dirty = bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=repo, text=True))
    subprocess.run([str(executable), "--export", str(output)], cwd=repo, check=True)
    from PIL import Image
    for name in ("sweep-active", "sweep-won", "passive-lost"):
        with Image.open(output / (name + ".bmp")) as frame:
            frame.save(output / (name + ".png"))
    raw = (output / "results.json").read_bytes()
    plot_results(json.loads(raw), output / "strategy-comparison.png")
    metadata = {
        "source_commit": revision, "dirty": dirty,
        "command": [str(executable), "--export", str(output)],
        "backend": "production SDL software painter, 1280x720 RGBA32 logical pixels",
        "capture_ticks": {"sweep-active": 60, "sweep-won": 267, "passive-lost": 900},
        "raw_sha256": hashlib.sha256(raw).hexdigest(),
        "scope": "Fixed live-tool strategies, conservation and independent full-state terminal replay; no human tuning/device/performance claim",
        "plot": "Retained completed-tick observations; lines only connect checkpoints",
    }
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")


if __name__ == "__main__":
    main()
