#!/usr/bin/env python3
"""Render crucible_swarm_intro frame captures into review images.

Concept2D/dev-test evidence only: flat top-down wedges through the scripted orthographic
camera. It shows choreography, spacing and LOD tiers; it is not production fidelity.

    crucible_swarm_intro --frames frames --every 2
    python3 render_swarm.py frames --sheet beats.svg --tiers tiers.svg --gif intro.gif

Sheets ending in .svg are written as vectors (the committed evidence); other extensions
are rasterised with Pillow. Sheet --times are choreography seconds; GIF and MP4 play at the
presentation pace recorded in each capture (see IntroSettings::pace).

    crucible_swarm_intro --build scaffold --frames frames --every 1
    python3 render_swarm.py frames --mp4 intro.mp4          # needs ffmpeg on PATH
"""
import argparse
import math
import pathlib
import re
import shutil
import subprocess

import numpy as np
from PIL import Image, ImageDraw

import nanite_mesh

BACKGROUND = (14, 16, 20)
SILVER = (196, 202, 210)
SILVER_DIM = (120, 126, 134)
CYAN = (60, 230, 255)
CYAN_DIM = (30, 90, 110)
CORE_SETTLED = (40, 160, 185)
TIER_COLOURS = {0: (255, 190, 60), 1: (80, 210, 255), 2: (150, 110, 255), 3: (90, 90, 90)}
TIER_NAMES = {0: "hero", 1: "near", 2: "far", 3: "hidden"}
MAGNET_COLOURS = {1: (60, 230, 255), 2: (255, 150, 40), 3: (250, 250, 250)}   # lift, push, seat
HEADER = re.compile(r"# time=([\d.]+) cx=([-\d.]+) cy=([-\d.]+) half_width=([\d.]+) aspect=([\d.]+)"
                    r"(?: view=(\w+) tilt=([\d.]+))?(?: tile=([\d.]+))?(?: pace=([\d.]+))?(?: body=([-\d.,]+))?")
MESH_SCALE = 6.0    # pixels per unit at which units are drawn as 3D bodies (below: silhouettes)
SEAM_SCALE = 14.0   # ... at which seams are outlined (below, they would turn letters to texture)
RING_SCALE = 10.0   # ... at which the core's dark recess ring is drawn
LIGHT = np.array([-0.55, -0.45, 0.70]) / np.linalg.norm([-0.55, -0.45, 0.70])   # cool grazing key


def load(path):
    lines = path.read_text().splitlines()
    groups = HEADER.match(lines[0]).groups()
    t, cx, cy, hw, aspect = (float(v) for v in groups[:5])
    view = groups[5] or "top"
    tilt = float(groups[6] or 0.4)
    tile = float(groups[7] or 0.0)
    pace = float(groups[8] or 1.0)   # choreography seconds per presentation second
    body = nanite_mesh.parse_body(groups[9])
    rows = []
    columns = lines[1].split(",")
    first_corner = columns.index("s0x") if "s0x" in columns else len(columns)
    layer_at = columns.index("layer") if "layer" in columns else None
    for line in lines[2:]:
        fields = line.split(",")
        x, y, z, h, act, tier, since = fields[:7]
        plane = float(fields[7]) if len(fields) > 7 else 0.0   # tilt about x: 0 ground, pi/2 wall
        magnet = int(fields[8]) if len(fields) > 8 else 0      # 1 lift, 2 push, 3 seat
        layer = int(fields[layer_at]) if layer_at is not None else 0   # depth layer (3D tessellation)
        # Locked tiles carry their morphed corners (letter plane: x along, y up the letter).
        shape = tuple(float(v) for v in fields[first_corner:first_corner + 6]) if len(fields) >= first_corner + 6 else None
        rows.append((float(x), float(y), float(z), float(h), act, int(tier), float(since), plane, magnet, shape, layer))
    return {"time": t, "cx": cx, "cy": cy, "hw": hw, "aspect": aspect, "view": view, "tilt": tilt, "tile": tile, "pace": pace, "body": body, "rows": rows}


class Canvas:
    """Records primitives so one frame can be rasterised (PNG/GIF) or written as SVG."""

    def __init__(self, size):
        self.size = size
        self.items = []

    def polygon(self, points, fill, outline=None):
        self.items.append(("polygon", points, fill, outline))

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
                draw.polygon(item[1], fill=item[2], outline=item[3])
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
                stroke = f' stroke="{colour(item[3])}" stroke-width="0.6"' if item[3] else ""
                parts.append(f'<polygon points="{pts}" fill="{colour(item[2])}"{stroke}/>')
            elif item[0] == "ellipse":
                x0, y0, x1, y1 = item[1]
                parts.append(f'<circle cx="{(x0 + x1) / 2:.1f}" cy="{(y0 + y1) / 2:.1f}" r="{(x1 - x0) / 2:.1f}" fill="{colour(item[2])}"/>')
            elif item[0] == "point":
                parts.append(f'<rect x="{item[1][0]:.1f}" y="{item[1][1]:.1f}" width="1" height="1" fill="{colour(item[2])}"/>')
            else:
                parts.append(f'<text x="{item[1][0] + 2}" y="{item[1][1] + 11}" font-family="monospace" font-size="11" fill="{colour(item[3])}">{item[2]}</text>')
        parts.append("</svg>")
        return "\n".join(parts)


def draw_frame(frame, size=(960, 540), mode="beauty", yaw=0.0):
    w, h = size
    draw = Canvas(size)
    scale = w / (2.0 * frame["hw"])
    cx, cy = frame["cx"], frame["cy"]
    t = frame["time"]

    oblique = frame["view"] == "oblique"
    tilt = frame["tilt"]

    cyaw, syaw = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))

    def turned(x, y):
        """Yaw about the vertical axis through the view centre: (across, depth)."""
        return (cx + (x - cx) * cyaw - y * syaw, (x - cx) * syaw + y * cyaw)

    def project(x, y, z):
        """World (z up) to screen. Top: looking down, lift nudges up. Oblique: front view,
        optionally yawed so the depth of a layered (3D tessellated) title shows."""
        if oblique:
            ax, depth = turned(x, y)
            vx, vy = ax, z + tilt * depth
        else:
            vx, vy = x, y + 0.35 * z
        return (w * 0.5 + (vx - cx) * scale, h * 0.5 - (vy - cy) * scale)

    rest = ("dormant", "locked", "moored", "reserve")
    if oblique:
        # Painter's order: farther (larger depth y) first; on the wall, the face over the slot.
        order = sorted(frame["rows"], key=lambda r: (-turned(r[0], r[1])[1], r[2]))
    else:
        order = sorted(frame["rows"], key=lambda r: (r[4] not in rest, r[2]))
    deepest = max((r[10] for r in frame["rows"]), default=0)
    bodies = []   # close enough for 3D bodies: drawn together, depth-sorted face by face
    for x, y, z, heading, act, tier, since, plane, magnet, shape, layer in order:
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
            if deepest > 0 and act in ("locked", "dock"):
                # Layered title: deeper layers darker, so the staggered bond reads in depth.
                shade = 1.0 - 0.22 * (deepest - layer) / max(1, deepest)
                fill = tuple(int(v * shade) for v in fill)
            if mode == "magnets" and magnet:
                fill = MAGNET_COLOURS[magnet]
            elif mode == "magnets":
                fill = (70, 74, 80) if act not in ("queue",) else (150, 150, 160)
        if scale < 3.0:
            draw.point((px, py), fill)
            continue
        # Core: wake pulse decays to a steady glow; dormant cores are dark.
        if act == "dormant":
            core = CYAN_DIM
        elif act == "reserve":
            core = CYAN_DIM if t - since > 1.0 else CYAN
        elif act in ("locked", "moored"):
            core = CORE_SETTLED   # settled: a steady glow, subordinate to the silver (brief)
        else:
            core = CYAN
        if scale >= MESH_SCALE:
            bodies.append((x, y, z, heading, plane, shape, fill, core if mode == "beauty" else None))
            continue
        if shape is not None:
            # Morphed tile: corners in the letter's plane, mapped through the plane tilt.
            inset = 0.97
            points = [project(x + shape[k] * inset, y + shape[k + 1] * inset * cp, z + shape[k + 1] * inset * sp) for k in (0, 2, 4)]
        elif frame["tile"] > 0.0:
            # Tessellated: equilateral tiles (inset 4% so the joins read) that interlock.
            radius = frame["tile"] / math.sqrt(3.0) * 0.96
            third = 2.0 * math.pi / 3.0
            points = [corner(radius * math.cos(a), radius * math.sin(a)) for a in (0.0, third, -third)]
        else:
            points = [corner(0.55, 0.0), corner(-0.38, 0.36), corner(-0.38, -0.36)]
        draw.polygon(points, fill)
    if bodies:
        draw_bodies(draw, bodies, frame, size, scale, oblique, tilt, cyaw, syaw)
    shown = t / frame["pace"]
    label = f"t={t:5.2f}s" + (f" ({shown:.2f} s shown)" if frame["pace"] != 1.0 else "") + f"  half-width={frame['hw']:.1f}u"
    if mode == "tiers":
        counts = {k: 0 for k in TIER_NAMES}
        for row in frame["rows"]:
            counts[row[5]] += 1
        label += "  " + "  ".join(f"{TIER_NAMES[k]}={counts[k]}" for k in TIER_NAMES)
    draw.text((8, 6), label, (230, 230, 230))
    return draw


def draw_bodies(draw, bodies, frame, size, scale, oblique, tilt, cyaw, syaw):
    """3D nanite bodies (nanite_mesh): lit faces, back faces culled, all faces of all bodies
    painted far to near, seams outlined, recessed cores on the top faces."""
    w, h = size
    cx, cy = frame["cx"], frame["cy"]
    body = frame["body"]
    side = frame["tile"] if frame["tile"] > 0.0 else math.sqrt(3.0)
    n = len(bodies)
    origin = np.array([[b[0], b[1], b[2]] for b in bodies])
    heading = np.array([b[3] for b in bodies])
    plane = np.array([b[4] for b in bodies])
    shaped = np.array([b[5] is not None for b in bodies])
    free = nanite_mesh.equilateral(side)
    corners = np.empty((n, 3, 2))
    for k, b in enumerate(bodies):
        corners[k] = np.array(b[5]).reshape(3, 2) if b[5] is not None else free
    # Free units point with corner 0 (their heading); a title tile with its apex (corner 2).
    nose = np.where(shaped, 2, 0)
    local = nanite_mesh.build(corners, nose, body)
    world = nanite_mesh.to_world(local, origin, heading, plane, ~shaped)
    disc, halo = nanite_mesh.core(local, nose, body)
    disc = nanite_mesh.to_world(disc, origin, heading, plane, ~shaped)
    halo = nanite_mesh.to_world(halo, origin, heading, plane, ~shaped)
    # View: the direction along which screen position is constant, pointing away from us.
    if oblique:
        away = np.array([syaw, cyaw, -tilt])
    else:
        away = np.array([0.0, 0.35, -1.0])
    away /= np.linalg.norm(away)
    toward = -away

    def screen(p):
        x, y, z = p[..., 0], p[..., 1], p[..., 2]
        if oblique:
            ax = cx + (x - cx) * cyaw - y * syaw
            depth = (x - cx) * syaw + y * cyaw
            vx, vy = ax, z + tilt * depth
        else:
            vx, vy = x, y + 0.35 * z
        return np.stack([w * 0.5 + (vx - cx) * scale, h * 0.5 - (vy - cy) * scale], axis=-1)

    pts = screen(world)
    disc_pts, halo_pts = screen(disc), screen(halo)
    nrm = nanite_mesh.normals(world)
    facing = nrm @ toward
    polys = []
    for k in range(n):
        base, core = bodies[k][6], bodies[k][7]
        for f, face in enumerate(nanite_mesh.FACES):
            if facing[k, f] <= 1e-4:
                continue
            centre = world[k, list(face)].mean(axis=0)
            colour = nanite_mesh.shade(base, nrm[k, f], LIGHT, toward)
            role = nanite_mesh.ROLE[f]
            seam = tuple(int(v * 0.5) for v in colour) if role != "wall" and scale >= SEAM_SCALE else None
            polys.append((float(centre @ away), [tuple(p) for p in pts[k, list(face)]], colour, seam))
            if role == "top" and core is not None:
                if scale >= RING_SCALE:
                    ring = tuple(int(v * 0.30) for v in base)
                    polys.append((float(centre @ away) - 1e-3, [tuple(p) for p in halo_pts[k]], ring, None))
                polys.append((float(centre @ away) - 2e-3, [tuple(p) for p in disc_pts[k]], core, None))
    polys.sort(key=lambda item: -item[0])
    for _, points, colour, seam in polys:
        draw.polygon(points, colour, seam)


def nearest(frames, time):
    return min(frames, key=lambda f: abs(f["time"] - time))


def sheet(frames, times, mode, path, yaw=0.0):
    """Three-column contact sheet; an .svg path writes vectors, anything else a raster."""
    tiles = [draw_frame(nearest(frames, t), (640, 360), mode, yaw) for t in times]
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


def presentation_step(frames):
    """Wall-clock seconds between captures: the choreography step over the pace."""
    if len(frames) < 2:
        return 1.0 / 30.0
    # Averaged over the whole capture: headers round each time to microseconds.
    return (frames[-1]["time"] - frames[0]["time"]) / (len(frames) - 1) / frames[0]["pace"]


def video(frames, path, mode, size, hold, yaw=0.0):
    """Pipes rasterised frames to ffmpeg as raw RGB; the final frame is held for `hold` s."""
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        raise SystemExit("--mp4 needs ffmpeg on PATH")
    w, h = (int(v) for v in size.lower().split("x"))
    if w % 2 or h % 2:
        raise SystemExit("--mp4-size needs even dimensions for yuv420p")
    fps = 1.0 / presentation_step(frames)
    command = [ffmpeg, "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{w}x{h}",
               "-r", f"{fps:.6f}", "-i", "-", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "18",
               "-preset", "medium", "-movflags", "+faststart", str(path)]
    with subprocess.Popen(command, stdin=subprocess.PIPE) as encoder:
        last = b""
        for f in frames:
            last = draw_frame(f, (w, h), mode, yaw).image().convert("RGB").tobytes()
            encoder.stdin.write(last)
        for _ in range(int(round(hold * fps))):
            encoder.stdin.write(last)
        encoder.stdin.close()
        if encoder.wait() != 0:
            raise SystemExit("ffmpeg failed")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("frames", type=pathlib.Path)
    parser.add_argument("--sheet", type=pathlib.Path)
    parser.add_argument("--tiers", type=pathlib.Path)
    parser.add_argument("--gif", type=pathlib.Path)
    parser.add_argument("--gif-mode", default="beauty", choices=["beauty", "tiers", "magnets"])
    parser.add_argument("--mp4", type=pathlib.Path, help="H.264 video at the capture's presentation rate (needs ffmpeg)")
    parser.add_argument("--mp4-mode", default="beauty", choices=["beauty", "tiers", "magnets"])
    parser.add_argument("--mp4-size", default="1280x720", help="video frame size, WIDTHxHEIGHT (even numbers)")
    parser.add_argument("--hold", type=float, default=2.0, help="seconds to hold the final frame in the video")
    parser.add_argument("--magnets", type=pathlib.Path, help="sheet coloured by magnet mode: cyan lift, amber push, white seat")
    parser.add_argument("--times", default="1.0,3.0,5.0,6.8,8.4,11.9")
    parser.add_argument("--yaw", type=float, default=0.0, help="oblique view: degrees of yaw about the vertical, to show depth layers")
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
        sheet(frames, times, "beauty", args.sheet, args.yaw)
    if args.tiers:
        sheet(frames, times, "tiers", args.tiers, args.yaw)
    if args.magnets:
        sheet(frames, times, "magnets", args.magnets, args.yaw)
    if args.gif:
        images = [draw_frame(f, (640, 360), args.gif_mode, args.yaw).image() for f in frames]
        step_ms = int(round(presentation_step(frames) * 1000))
        images[0].save(args.gif, save_all=True, append_images=images[1:], duration=step_ms, loop=0, optimize=True)
    if args.mp4:
        video(frames, args.mp4, args.mp4_mode, args.mp4_size, args.hold, args.yaw)


if __name__ == "__main__":
    main()
