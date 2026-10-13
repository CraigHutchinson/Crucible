"""The nanite's 3D body as a mesh, built in batches for the swarm renderer.

Mirrors NaniteBody.hpp (the single source of the dimensions; frame captures carry them in
their header). Director brief: an engineered low asymmetrical triangular body with distinct
beveled planes, a recessed cyan core and dark mechanical seams; letter bodies and moving
units share one construction.

Construction, per unit, in its plane's coordinates (a, b along the plane, h along its
normal): vertical walls rise from the triangular footprint to a shoulder, a chamfer runs
inward to the top face, and the top slopes from the rear edge down to the nose. The nose's
chamfer is wider than the rear ones. Because the walls are vertical, any tiling of
footprints (the title's morphed tiles) stays flush in 3D.

Vertex layout per body: 0-2 footprint (h = 0), 3-5 shoulder ring, 6-8 top face.
"""
import math

import numpy as np

DEFAULT_BODY = {"height": 0.55, "nose_height": 0.30, "shoulder": 0.55, "bevel": 0.16, "nose_bevel": 0.34,
                "core_radius": 0.15, "core_offset": 0.12}
BODY_KEYS = ("height", "nose_height", "shoulder", "bevel", "nose_bevel", "core_radius", "core_offset")

# Faces as vertex indices, counter-clockwise seen from outside (footprints are CCW).
WALLS = [(i, (i + 1) % 3, 3 + (i + 1) % 3, 3 + i) for i in range(3)]
BEVELS = [(3 + i, 3 + (i + 1) % 3, 6 + (i + 1) % 3, 6 + i) for i in range(3)]
TOP = [(6, 7, 8)]
FACES = WALLS + BEVELS + TOP
ROLE = ["wall"] * 3 + ["bevel"] * 3 + ["top"]


def parse_body(text):
    """Header value 'h,nose,shoulder,bevel,nose_bevel,core,offset' to a body dict."""
    if not text:
        return dict(DEFAULT_BODY)
    return dict(zip(BODY_KEYS, (float(v) for v in text.split(","))))


def equilateral(side):
    """Footprint of a free unit in its heading frame: nose (corner 0) along +a, CCW."""
    r = side / math.sqrt(3.0)
    return np.array([[r * math.cos(a), r * math.sin(a)] for a in (0.0, 2.0 * math.pi / 3.0, -2.0 * math.pi / 3.0)])


def build(corners, nose, body):
    """Bodies for N footprints.

    corners: (N, 3, 2) CCW footprint corners relative to the unit's origin.
    nose: (N,) index of the nose corner.
    Returns (N, 9, 3) vertices (a, b, h).
    """
    corners = np.asarray(corners, dtype=float)
    n = corners.shape[0]
    centroid = corners.mean(axis=1, keepdims=True)
    is_nose = np.arange(3)[None, :] == np.asarray(nose)[:, None]
    top_h = np.where(is_nose, body["nose_height"], body["height"])
    inset = np.where(is_nose, body["nose_bevel"], body["bevel"])
    top_xy = corners + (centroid - corners) * inset[:, :, None]
    verts = np.zeros((n, 9, 3))
    verts[:, 0:3, :2] = corners
    verts[:, 3:6, :2] = corners
    verts[:, 3:6, 2] = top_h * body["shoulder"]
    verts[:, 6:9, :2] = top_xy
    verts[:, 6:9, 2] = top_h
    return verts


def core(verts, nose, body, sides=6, ring=1.45):
    """Recessed core on each top face, toward the rear: (N, sides, 3) disc and ring outlines."""
    top = verts[:, 6:9, :]
    tip = top[np.arange(len(top)), nose]
    centre = top.mean(axis=1)
    normal = np.cross(top[:, 1] - top[:, 0], top[:, 2] - top[:, 0])
    normal /= np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-9)
    rear = centre - tip   # along the (sloped) top face, away from the nose
    rear -= normal * np.einsum("ij,ij->i", rear, normal)[:, None]
    rear /= np.maximum(np.linalg.norm(rear, axis=1, keepdims=True), 1e-9)
    c = centre + rear * body["core_offset"]
    # In-plane basis on the (sloped) top face.
    u = np.cross(normal, np.array([0.0, 0.0, 1.0]))
    flat = np.linalg.norm(u, axis=1) < 1e-6
    u[flat] = [1.0, 0.0, 0.0]
    u /= np.linalg.norm(u, axis=1, keepdims=True)
    v = np.cross(normal, u)
    angles = np.linspace(0.0, 2.0 * math.pi, sides, endpoint=False)
    offsets = np.cos(angles)[None, :, None] * u[:, None, :] + np.sin(angles)[None, :, None] * v[:, None, :]
    lift = normal[:, None, :] * 0.005   # a hair above the face, so it sorts after it
    disc = c[:, None, :] + offsets * body["core_radius"] + lift
    halo = c[:, None, :] + offsets * body["core_radius"] * ring + lift * 0.5
    return disc, halo


def to_world(local, origin, heading, plane, rotate):
    """Local (a, b, h) to world (x, y, z): heading turns (a, b) within the plane (when
    `rotate`), the plane tilts about x (0 ground, pi/2 an upright face), h is the normal."""
    a, b, h = local[..., 0], local[..., 1], local[..., 2]
    c, s = np.cos(heading)[:, None], np.sin(heading)[:, None]
    pa = np.where(rotate[:, None], c * a - s * b, a)
    pb = np.where(rotate[:, None], s * a + c * b, b)
    cp, sp = np.cos(plane)[:, None], np.sin(plane)[:, None]
    x = origin[:, 0, None] + pa
    y = origin[:, 1, None] + pb * cp - h * sp
    z = origin[:, 2, None] + pb * sp + h * cp
    return np.stack([x, y, z], axis=-1)


def normals(world):
    """Outward unit normal of every face: (N, len(FACES), 3)."""
    out = []
    for face in FACES:
        p0, p1, p2 = world[:, face[0]], world[:, face[1]], world[:, face[2]]
        nrm = np.cross(p1 - p0, p2 - p1)
        out.append(nrm / np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-12))
    return np.stack(out, axis=1)


def shade(base, normal, light, view, emissive=0.0):
    """Lambert plus a controlled Blinn highlight (silver: smooth, not glittering)."""
    half = light + view
    half /= np.linalg.norm(half)
    diffuse = max(0.0, float(np.dot(normal, light)))
    spec = max(0.0, float(np.dot(normal, half))) ** 28
    k = 0.48 + 0.62 * diffuse
    return tuple(int(min(255, v * k + 255 * 0.42 * spec + emissive)) for v in base)
