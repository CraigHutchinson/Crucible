"""The nanite's 3D body as a mesh, built in batches for the swarm renderer.

Mirrors NaniteBody.hpp (the single source of the dimensions; frame captures carry them in
their header). The concepts (docs/concepts/units-v1.png, the phase 15 hero) show a slender
arrowhead dart: a raised spine from the rear to the nose, faceted plates falling away from
it, a steep rear facet and a cyan core set into the spine. The director brief: an
engineered low asymmetrical triangular body, beveled planes, a recessed protected core,
dark seams; letter bodies and moving units share one construction.

Construction, per unit, in its plane's coordinates (a, b along the plane, h along its
normal), for any triangular footprint with a nose corner: vertical walls rise from the
footprint to a low shoulder; a spine runs from just behind the nose (A, low) to just inside
the rear edge (R, high); five roof facets join the shoulder ring to the spine. A free unit's
footprint is a dart; a locked unit's is its (morphed) title tile, so the walls of
neighbouring tiles coincide and the letter stays gap-free.

Vertex layout per body: 0-2 footprint (h = 0), 3-5 shoulder ring, 6 spine front A, 7 spine
rear R. Corner 0 is the nose; corners run counter-clockwise.
"""
import math

import numpy as np

DEFAULT_BODY = {"height": 0.55, "nose_height": 0.22, "shoulder": 0.40, "spine_front": 0.28, "spine_rear": 0.14,
                "dart_length": 1.90, "dart_width": 0.90, "core_length": 0.36}
BODY_KEYS = ("height", "nose_height", "shoulder", "spine_front", "spine_rear", "dart_length", "dart_width", "core_length")

WALLS = [(0, 1, 4, 3), (1, 2, 5, 4), (2, 0, 3, 5)]
# Roof: nose-side and rear-side facets either side of the spine, and the rear facet.
ROOF = [(3, 4, 6), (4, 7, 6), (4, 5, 7), (5, 6, 7), (5, 3, 6)]
ROLE = ["wall"] * 3 + ["roof"] * 5
# The core lies on the two rear-side facets either side of the spine.
CORE_FACES = (4, 6)


def _orient(faces, verts):
    """Order each face counter-clockwise seen from outside, using a reference body."""
    centre = verts.mean(axis=0)
    out = []
    for face in faces:
        p = verts[list(face)]
        n = np.cross(p[1] - p[0], p[2] - p[1])
        out.append(face if np.dot(n, p.mean(axis=0) - centre) > 0 else tuple(reversed(face)))
    return out


def parse_body(text):
    """Header value (comma-separated, BODY_KEYS order) to a body dict."""
    if not text:
        return dict(DEFAULT_BODY)
    values = [float(v) for v in text.split(",")]
    if len(values) != len(BODY_KEYS):
        return dict(DEFAULT_BODY)   # a capture from an older body layout
    return dict(zip(BODY_KEYS, values))


def dart(body):
    """Footprint of a free unit in its heading frame: nose (corner 0) along +a, CCW, the
    centroid at the origin."""
    length, half = body["dart_length"], body["dart_width"] * 0.5
    corners = np.array([[length, 0.0], [0.0, half], [0.0, -half]])
    return corners - corners.mean(axis=0)


def build(corners, nose, body):
    """Bodies for N footprints.

    corners: (N, 3, 2) CCW footprint corners relative to the unit's origin.
    nose: (N,) index of the nose corner.
    Returns (N, 8, 3) vertices (a, b, h), reordered so vertex 0 is the nose.
    """
    corners = np.asarray(corners, dtype=float)
    nose = np.asarray(nose)
    n = corners.shape[0]
    # Rotate the corner order so the nose comes first (keeps counter-clockwise order).
    order = (np.arange(3)[None, :] + nose[:, None]) % 3
    c = np.take_along_axis(corners, order[:, :, None], axis=1)
    centroid = c.mean(axis=1)
    rear_mid = 0.5 * (c[:, 1] + c[:, 2])
    front = c[:, 0] + (centroid - c[:, 0]) * body["spine_front"]
    rear = rear_mid + (centroid - rear_mid) * body["spine_rear"]
    verts = np.zeros((n, 8, 3))
    verts[:, 0:3, :2] = c
    verts[:, 3:6, :2] = c
    shoulder = body["shoulder"] * body["height"]
    verts[:, 3:6, 2] = [shoulder * body["nose_height"] / body["height"], shoulder, shoulder]
    verts[:, 6, :2], verts[:, 6, 2] = front, body["nose_height"]
    verts[:, 7, :2], verts[:, 7, 2] = rear, body["height"]
    return verts


FACES = _orient(WALLS + ROOF, build(dart(DEFAULT_BODY)[None], np.array([0]), DEFAULT_BODY)[0])


def core(verts, body, ring=1.35):
    """Core diamond set into the spine near the rear, lying on the two rear-side roof facets.

    Returns two (N, 4, 3) outlines: the cyan core and its dark recess ring. Every vertex is a
    convex combination of a facet's own vertices, so it lies exactly on that facet."""
    a, r = verts[:, 6], verts[:, 7]
    left, right = verts[:, 4], verts[:, 5]
    spine = np.linalg.norm(r - a, axis=1, keepdims=True)
    t = np.clip(body["core_length"] / np.maximum(spine, 1e-9), 0.05, 0.8)

    def diamond(scale):
        back = r + (a - r) * (0.10 * scale)
        tip = r + (a - r) * (t * scale + 0.10 * scale)
        mid = r + (a - r) * (0.10 * scale + 0.5 * t * scale)
        side = 0.22 * scale
        return np.stack([back, mid + (left - mid) * side, tip, mid + (right - mid) * side], axis=1)

    lift = 0.004
    up = np.array([0.0, 0.0, lift])
    return diamond(1.0) + up, diamond(ring) + up * 0.5


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
