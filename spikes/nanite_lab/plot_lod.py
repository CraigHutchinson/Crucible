"""Charts the distance dolly (lod_dolly.mjs): projected size against the ladder, and the
per-frame image change a detail switch causes, hard cut against cross-fade.

Usage: python3 plot_lod.py <dolly.json> <out.png>
"""
import json
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

SURFACE = "#fcfcfb"
INK = "#0b0b0b"
INK_2 = "#52514e"
GRID = "#e4e3df"
CUT = "#eb6834"      # categorical slot 2 (orange)
FADE = "#2a78d6"     # categorical slot 1 (blue)
MOTION = "#a3a29c"   # neutral reference
STRUCTURES = ["Silhouette", "Frame", "Shielded", "Articulated"]
SURFACES = ["Flat", "Chamfer", "Bevel", "Hero"]


def main(path, out):
    data = json.load(open(path))
    rows = data["rows"]
    seconds = [r["frame"] / 30 for r in rows]
    fig, (top, bottom) = plt.subplots(2, 1, figsize=(10, 6.4), sharex=True, gridspec_kw={"height_ratios": [1, 1.25]})
    fig.patch.set_facecolor(SURFACE)
    for axis in (top, bottom):
        axis.set_facecolor(SURFACE)
        axis.grid(True, color=GRID, linewidth=0.8)
        axis.set_axisbelow(True)
        for side in ("top", "right"):
            axis.spines[side].set_visible(False)
        for side in ("left", "bottom"):
            axis.spines[side].set_color(GRID)
        axis.tick_params(colors=INK_2, labelsize=9)

    top.plot(seconds, [r["size"] for r in rows], color=FADE, linewidth=2)
    top.set_yscale("log")
    ticks = [6] + [step["minPixels"] for step in data["ladder"][1:] if step["minPixels"] <= 150] + [150]
    top.set_yticks(ticks)
    top.set_yticklabels([f"{t:g}" for t in ticks])
    for rung, step in enumerate(data["ladder"][1:], start=1):
        if step["minPixels"] > 150:
            continue
        top.hlines(step["minPixels"], seconds[0], seconds[-1], color=INK_2, linewidth=0.8, linestyle=(0, (3, 3)))
        label = f"{STRUCTURES[step['detail'][0]]} {SURFACES[step['detail'][1]]}"
        top.text(seconds[-1] * 1.015, step["minPixels"], label, va="center", fontsize=8, color=INK_2)
    top.set_ylabel("Projected size (px)", color=INK_2, fontsize=9)
    top.set_title("Dolly from 6 to 150 px and back; dashed lines are the ladder's switch sizes",
                  loc="left", fontsize=10, color=INK)

    bottom.plot(seconds, [r["motion"] for r in rows], color=MOTION, linewidth=2, label="Unit's own motion (reference)")
    bottom.plot(seconds, [r["jumpCut"] for r in rows], color=CUT, linewidth=2, label="Switch jump, hard cut")
    bottom.plot(seconds, [r["jumpFade"] for r in rows], color=FADE, linewidth=2, label="Switch jump, 0.25 s cross-fade")
    bottom.set_ylabel("Mean colour change per frame", color=INK_2, fontsize=9)
    bottom.set_xlabel("Seconds", color=INK_2, fontsize=9)
    bottom.set_title("Change a switch adds in one frame, against the change the unit's own motion makes",
                     loc="left", fontsize=10, color=INK)
    legend = bottom.legend(loc="upper right", fontsize=8, frameon=False)
    for text in legend.get_texts():
        text.set_color(INK)
    peak = max(rows, key=lambda r: r["jumpCut"])
    bottom.annotate(f"cut {peak['jumpCut']:.3f}", (peak["frame"] / 30, peak["jumpCut"]), xytext=(6, 4),
                    textcoords="offset points", fontsize=8, color=INK)
    fade_peak = max(rows, key=lambda r: r["jumpFade"])
    bottom.annotate(f"fade {fade_peak['jumpFade']:.3f}", (fade_peak["frame"] / 30, fade_peak["jumpFade"]),
                    xytext=(6, 4), textcoords="offset points", fontsize=8, color=INK)
    fig.tight_layout()
    fig.savefig(out, dpi=150, facecolor=SURFACE)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
