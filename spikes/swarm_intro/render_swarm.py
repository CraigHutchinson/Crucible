#!/usr/bin/env python3
"""Render crucible_swarm_intro frame captures into review images.

Concept2D/dev-test evidence only: flat top-down wedges through the scripted orthographic
camera. It shows choreography, spacing and LOD tiers; it is not production fidelity.

    crucible_swarm_intro --frames frames --every 2
    python3 render_swarm.py frames --sheet beats.svg --tiers tiers.svg --gif intro.gif

Sheets ending in .svg are written as vectors (the committed evidence); other extensions
are rasterised with Pillow.
"""
import argparse
import math
import pathlib
import re

from PIL import Image, ImageDraw

BACKGROUND = (14, 16, 20)
SILVER = (196, 202, 210)
SILVER_DIM = (120, 126, 134)
CYAN = (60, 230, 255)
CYAN_DIM = (30, 90, 110)
TIER_COLOURS = {0: (255, 190, 60), 1: (80, 210, 255), 2: (150, 110, 255), 3: (90, 90, 90)}
TIER_NAMES = {0: "hero", 1: "near", 2: "far", 3: "hidden"}
MAGNET_COLOURS = {1: (60, 230, 255), 2: (255, 150, 40), 3: (250, 250, 250)}   # lift, push, seat
HEADER = re.compile(r"# time=([\d.]+) cx=([-\d.]+) cy=([-\d.]+) half_width=([\d.]+) aspect=([\d.]+)"
                    r"(?: view=(\w+) tilt=([\d.]+))?(?: tile=([\d.]+))?")


def load(path):
    lines = path.read_text().splitlines()
    groups = HEADER.match(lines[0]).groups()
    t, cx, cy, hw, aspect = (float(v) for v in groups[:5])
    view = groups[5] or "top"
    tilt = float(groups[6] or 0.4)
    tile = float(groups[7] or 0.0)
    rows = []
    for line in lines[2:]:
        fields = line.split(",")
        x, y, z, h, act, tier, since = fields[:7]
        plane = float(fields[7]) if len(fields) > 7 else 0.0   # tilt about x: 0 ground, pi/2 wall
        magnet = int(fields[8]) if len(fields) > 8 else 0      # 1 lift, 2 push, 3 seat
        rows.append((float(x), float(y), float(z), float(h), act, int(tier), float(since), plane, magnet))
    return {"time": t, "cx": cx, "cy": cy, "hw": hw, "aspect": aspect, "view": view, "tilt": tilt, "tile": tile, "rows": rows}


class Canvas:
    """Records primitives so one frame can be rasterised (PNG/GIF) or written as SVG."""

    def __init__(self, size):
        self.size = size
        self.items = []

    def polygon(self, points, fill):
        self.items.append(("polygon", points, fill))

    def ellipse(self, box, fill):
        self.items.append(("ellipse", box, fill))

    def point(self, xy, fill):
        self.items.append(("point", xy, fill))

    def text(self, xy, text, fill):
        self.items.append(("text", xy, text, fill))

    def image(self):
        out = Image.new("RGB", self.size, BACKGROUND)
        draw = ImageDraw.Draw(out)
        for item in self.items:
            if item[0] == "polygon":
                draw.polygon(item[1], fill=item[2])
            elif item[0] == "ellipse":
                draw.ellipse(item[1], fill=item[2])
            elif item[0] == "point":
                draw.point(item[1], fill=item[2])
            else:
                draw.text(item[1], item[2], fill=item[3])
        return out

    def svg(self, ox, oy):
        def colour(c):
            return "#%02x%02x%02x" % c
        w, h = self.size
        parts = [f'<rect x="{ox}" y="{oy}" width="{w}" height="{h}" fill="{colour(BACKGROUND)}"/>']
        parts.append(f'<svg x="{ox}" y="{oy}" width="{w}" height="{h}" overflow="hidden">')
        for item in self.items:
            if item[0] == "polygon":
                pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in item[1])
                parts.append(f'<polygon points="{pts}" fill="{colour(item[2])}"/>')
            elif item[0] == "ellipse":
                x0, y0, x1, y1 = item[1]
                parts.append(f'<circle cx="{(x0 + x1) / 2:.1f}" cy="{(y0 + y1) / 2:.1f}" r="{(x1 - x0) / 2:.1f}" fill="{colour(item[2])}"/>')
            elif item[0] == "point":
                parts.append(f'<rect x="{item[1][0]:.1f}" y="{item[1][1]:.1f}" width="1" height="1" fill="{colour(item[2])}"/>')
            else:
                parts.append(f'<text x="{item[1][0] + 2}" y="{item[1][1] + 11}" font-family="monospace" font-size="11" fill="{colour(item[3])}">{item[2]}</text>')
        parts.append("</svg>")
        return "\n".join(parts)


def draw_frame(frame, size=(960, 540), mode="beauty"):
    w, h = size
    draw = Canvas(size)
    scale = w / (2.0 * frame["hw"])
    cx, cy = frame["cx"], frame["cy"]
    t = frame["time"]

    oblique = frame["view"] == "oblique"
    tilt = frame["tilt"]

    def project(x, y, z):
        """World (z up) to screen. Top: looking down, lift nudges up. Oblique: front view."""
        vx, vy = (x, z + tilt * y) if oblique else (x, y + 0.35 * z)
        return (w * 0.5 + (vx - cx) * scale, h * 0.5 - (vy - cy) * scale)

    rest = ("dormant", "locked", "moored", "reserve")
    if oblique:
        # Painter's order: farther (larger depth y) first; on the wall, the face over the slot.
        order = sorted(frame["rows"], key=lambda r: (-r[1], r[2]))
    else:
        order = sorted(frame["rows"], key=lambda r: (r[4] not in rest, r[2]))
    for x, y, z, heading, act, tier, since, plane, magnet in order:
        px, py = project(x, y, z)
        if px < -20 or px > w + 20 or py < -20 or py > h + 20:
            continue
        c, s = math.cos(heading), math.sin(heading)

        cp, sp = math.cos(plane), math.sin(plane)

        def corner(forward, side):
            # A wedge lies in its plane: the ground (x, y), a wall face (x, z), or a letter
            # part-way through its rise.
            ax = c * forward - s * side
            ay = s * forward + c * side
            return project(x + ax, y + ay * cp, z + ay * sp)

        if mode == "tiers":
            fill = TIER_COLOURS[tier]
        else:
            fill = SILVER_DIM if act in ("dormant", "reserve") else SILVER
            if act == "climb" or (plane < 0.1 and act not in rest and z > 0.1):
                fill = tuple(min(255, v + 30) for v in fill)
            if mode == "magnets" and magnet:
                fill = MAGNET_COLOURS[magnet]
            elif mode == "magnets":
                fill = (70, 74, 80) if act not in ("queue",) else (150, 150, 160)
        if scale < 3.0:
            draw.point((px, py), fill)
            continue
        if frame["tile"] > 0.0:
            # Tessellated: equilateral tiles (inset 4% so the joins read) that interlock.
            radius = frame["tile"] / math.sqrt(3.0) * 0.96
            third = 2.0 * math.pi / 3.0
            points = [corner(radius * math.cos(a), radius * math.sin(a)) for a in (0.0, third, -third)]
        else:
            points = [corner(0.55, 0.0), corner(-0.38, 0.36), corner(-0.38, -0.36)]
        draw.polygon(points, fill)
        if mode == "beauty" and scale >= 6.0:
            # Core: wake pulse decays to a steady glow; dormant cores are dark.
            if act == "dormant":
                core = CYAN_DIM
            elif act == "reserve":
                core = CYAN_DIM if t - since > 1.0 else CYAN
            else:
                core = CYAN
            r = max(1.0, 0.11 * scale)
            draw.ellipse([px - r, py - r, px + r, py + r], core)
    label = f"t={t:5.2f}s  half-width={frame['hw']:.1f}u"
    if mode == "tiers":
        counts = {k: 0 for k in TIER_NAMES}
        for row in frame["rows"]:
            counts[row[5]] += 1
        label += "  " + "  ".join(f"{TIER_NAMES[k]}={counts[k]}" for k in TIER_NAMES)
    draw.text((8, 6), label, (230, 230, 230))
    return draw


def nearest(frames, time):
    return min(frames, key=lambda f: abs(f["time"] - time))


def sheet(frames, times, mode, path):
    """Three-column contact sheet; an .svg path writes vectors, anything else a raster."""
    tiles = [draw_frame(nearest(frames, t), (640, 360), mode) for t in times]
    columns = 3
    rows = (len(tiles) + columns - 1) // columns
    width, height = columns * 640 + (columns - 1) * 4, rows * 360 + (rows - 1) * 4
    if path.suffix == ".svg":
        body = [tile.svg((i % columns) * 644, (i // columns) * 364) for i, tile in enumerate(tiles)]
        path.write_text(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
                        f'viewBox="0 0 {width} {height}"><rect width="100%" height="100%" fill="#282828"/>'
                        + "\n".join(body) + "</svg>\n")
        return
    out = Image.new("RGB", (width, height), (40, 40, 40))
    for i, tile in enumerate(tiles):
        out.paste(tile.image(), ((i % columns) * 644, (i // columns) * 364))
    out.save(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("frames", type=pathlib.Path)
    parser.add_argument("--sheet", type=pathlib.Path)
    parser.add_argument("--tiers", type=pathlib.Path)
    parser.add_argument("--gif", type=pathlib.Path)
    parser.add_argument("--gif-mode", default="beauty", choices=["beauty", "tiers", "magnets"])
    parser.add_argument("--magnets", type=pathlib.Path, help="sheet coloured by magnet mode: cyan lift, amber push, white seat")
    parser.add_argument("--times", default="1.0,3.0,5.0,6.8,8.4,11.9")
    parser.add_argument("--zoom", help="override the camera for sheets: cx,cy,half_width (view units)")
    args = parser.parse_args()
    frames = [load(p) for p in sorted(args.frames.glob("frame_*.csv"))]
    if not frames:
        raise SystemExit("no frame_*.csv captures found")
    times = [float(v) for v in args.times.split(",")]
    if args.zoom:
        cx, cy, hw = (float(v) for v in args.zoom.split(","))
        for f in frames:
            f["cx"], f["cy"], f["hw"] = cx, cy, hw
    if args.sheet:
        sheet(frames, times, "beauty", args.sheet)
    if args.tiers:
        sheet(frames, times, "tiers", args.tiers)
    if args.magnets:
        sheet(frames, times, "magnets", args.magnets)
    if args.gif:
        images = [draw_frame(f, (640, 360), args.gif_mode).image() for f in frames]
        step_ms = int(round((frames[1]["time"] - frames[0]["time"]) * 1000)) if len(frames) > 1 else 33
        images[0].save(args.gif, save_all=True, append_images=images[1:], duration=step_ms, loop=0, optimize=True)


if __name__ == "__main__":
    main()
