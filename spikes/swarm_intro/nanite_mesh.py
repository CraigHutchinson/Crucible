"""The nanite's 3D body as a mesh, built in batches for the swarm renderer.

A prototype proxy of the tetrahedral direction in docs/concepts/nanite/ (the authority on
the unit's form): a compact silver tetrahedral shell around ONE rigid cyan tetrahedron core.
The shell has four graphite vertex hubs, six silver edge rails and four recessed face
shields, each shield open onto the same core. NaniteBody.hpp is the single source of the
dimensions the simulation shares (the stacking height) and that captures carry in their
header; the part breakdown below (rails, hubs, windows, recess) is view-owned decoration.

Construction, per unit, in its plane's coordinates (a, b along the plane, h along its
normal), for any triangular footprint with a nose corner: the shell's base is the footprint
(h = 0) and its apex stands `height` above a point leaned from the centroid toward the
nose, so the low oblique body reads its heading. A free unit's footprint is an equilateral
triangle `span` on a side; a locked unit's is its (morphed) title tile, so neighbouring
shells share base edges and the letter's footprint stays gap-free. That docked envelope is
a disclosed proxy: the design space does not claim the articulated unit tessellates.

A moving unit's shell elongates along its heading with speed (`stretch`, `stretch_speed`):
the nose reaches forward while the rear edge and the apex stay, a long low body leading with
its nose. The core is computed from the compact shell and never changes; the stretched base
contains the compact base, so the stretched shell contains the compact shell and its core.

The core is a regular tetrahedron (edge `core` x the shell's mean base edge), base down,
centred under the apex, lifted clear of the surface and shrunk where a squat docked shell
would not hold it; tests hold it inside the shell.

Vertex layout per shell: 0-2 base (corner 0 the nose, counter-clockwise), 3 apex.
"""
import math

import numpy as np

DEFAULT_BODY = {"height": 0.55, "span": 1.10, "lean": 0.16, "core": 0.40, "stretch": 0.90, "stretch_speed": 24.0}
BODY_KEYS = ("height", "span", "lean", "core", "stretch", "stretch_speed")

# View-owned part breakdown of every shell face, as fractions of the way from each corner
# toward the face centroid (rail, window) or along an edge (hub).
RAIL = 0.20      # edge rail band
HUB = 0.24       # vertex hub length along each edge
WINDOW = 0.66    # shield inner edge: the opening onto the core
RECESS = 0.035   # shields sit this far (diameters) inside the face plane
CORE_LIFT = 0.12  # core base above the shell base, as a fraction of `height`
CLEARANCE = 0.04  # least distance (diameters) from the core to the shell's faces

SHELL_FACES = [(0, 2, 1), (0, 1, 3), (1, 2, 3), (2, 0, 3)]   # base, then the three sides
CORE_FACES = SHELL_FACES   # same layout: 0-2 base, 3 apex


def parse_body(text):
    """Header value (comma-separated, BODY_KEYS order) to a body dict."""
    if not text:
        return dict(DEFAULT_BODY)
    values = [float(v) for v in text.split(",")]
    if len(values) != len(BODY_KEYS):
        return dict(DEFAULT_BODY)   # a capture from an older body layout
    return dict(zip(BODY_KEYS, values))


def footprint(body):
    """Footprint of a free unit in its heading frame: an equilateral triangle, nose (corner 0)
    along +a, counter-clockwise, the centroid at the origin."""
    r = body["span"] / math.sqrt(3.0)
    third = 2.0 * math.pi / 3.0
    return np.array([[r * math.cos(a), r * math.sin(a)] for a in (0.0, third, -third)])


def stretch_amount(speed, body):
    """0 at rest to 1 at `stretch_speed`, eased (smoothstep) so cruising reads as one shape."""
    t = np.clip(np.asarray(speed, dtype=float) / max(body["stretch_speed"], 1e-6), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def elongate(verts, amount, body):
    """Stretch shells (N, 4, 3), vertex 0 the nose, along their nose direction by `amount`
    (N,) in [0, 1]. Only the nose moves, so the result contains the compact shell."""
    out = np.array(verts, dtype=float, copy=True)
    centroid = out[:, 0:3, :2].mean(axis=1)
    forward = out[:, 0, :2] - centroid
    forward /= np.maximum(np.linalg.norm(forward, axis=1, keepdims=True), 1e-12)
    reach = (np.asarray(amount, dtype=float) * body["stretch"] * body["span"])[:, None]
    out[:, 0, :2] += forward * reach
    return out


def outline(body, amount):
    """A free unit's silhouette (its stretched base) in its heading frame, for one unit."""
    verts = np.zeros((1, 4, 3))
    verts[0, 0:3, :2] = footprint(body)
    return elongate(verts, np.array([amount]), body)[0, 0:3, :2]


def shell(corners, nose, body):
    """Shells for N footprints.

    corners: (N, 3, 2) CCW footprint corners relative to the unit's origin.
    nose: (N,) index of the nose corner.
    Returns (N, 4, 3) vertices (a, b, h), reordered so vertex 0 is the nose.
    """
    corners = np.asarray(corners, dtype=float)
    nose = np.asarray(nose)
    order = (np.arange(3)[None, :] + nose[:, None]) % 3
    c = np.take_along_axis(corners, order[:, :, None], axis=1)
    centroid = c.mean(axis=1)
    verts = np.zeros((corners.shape[0], 4, 3))
    verts[:, 0:3, :2] = c
    verts[:, 3, :2] = centroid + (c[:, 0] - centroid) * body["lean"]
    verts[:, 3, 2] = body["height"]
    return verts


def core(verts, body):
    """The single core: a regular tetrahedron, base down, under the shell's apex, its corner 0
    toward the nose, edge `core` x the shell's mean base edge but never closer than CLEARANCE
    to a shell face (a squat docked shell on a large tile bounds it). Returns (N, 4, 3) in
    the shell's coordinates."""
    base = verts[:, 0:3, :2]
    edges = np.linalg.norm(base - np.roll(base, -1, axis=1), axis=2).mean(axis=1)
    centre = verts[:, 3, :2]
    toward = base[:, 0] - centre
    toward /= np.maximum(np.linalg.norm(toward, axis=1, keepdims=True), 1e-12)
    # Unit-edge tetrahedron about its base centroid (the anchor), then scaled to fit.
    unit = np.zeros((verts.shape[0], 4, 3))
    for i, turn in enumerate((0.0, 2.0 * math.pi / 3.0, -2.0 * math.pi / 3.0)):
        c, s = math.cos(turn), math.sin(turn)
        unit[:, i, 0] = (c * toward[:, 0] - s * toward[:, 1]) / math.sqrt(3.0)
        unit[:, i, 1] = (s * toward[:, 0] + c * toward[:, 1]) / math.sqrt(3.0)
    unit[:, 3, 2] = math.sqrt(2.0 / 3.0)
    anchor = np.zeros((verts.shape[0], 1, 3))
    anchor[:, 0, :2] = centre
    anchor[:, 0, 2] = CORE_LIFT * body["height"]
    edge = body["core"] * edges
    nrm = normals(verts)
    for f, face in enumerate(SHELL_FACES):
        n = nrm[:, f]
        gap = np.einsum("ij,ij->i", anchor[:, 0] - verts[:, face[0]], n)     # anchor's (negative) distance
        reach = np.einsum("ikj,ij->ik", unit, n).max(axis=1)                  # per unit edge
        limit = np.where(reach > 1e-9, (-CLEARANCE - gap) / np.maximum(reach, 1e-9), np.inf)
        edge = np.minimum(edge, limit)
    return anchor + unit * edge[:, None, None]


def to_world(local, origin, heading, plane, rotate):
    """Local (a, b, h) to world (x, y, z): heading turns (a, b) within the plane (when
    `rotate`), the plane tilts about x (0 ground, pi/2 an upright face), h is the normal."""
    shape = local.shape
    flat = local.reshape(shape[0], -1, 3)
    a, b, h = flat[..., 0], flat[..., 1], flat[..., 2]
    c, s = np.cos(heading)[:, None], np.sin(heading)[:, None]
    pa = np.where(rotate[:, None], c * a - s * b, a)
    pb = np.where(rotate[:, None], s * a + c * b, b)
    cp, sp = np.cos(plane)[:, None], np.sin(plane)[:, None]
    x = origin[:, 0, None] + pa
    y = origin[:, 1, None] + pb * cp - h * sp
    z = origin[:, 2, None] + pb * sp + h * cp
    return np.stack([x, y, z], axis=-1).reshape(shape)


def normals(world, faces=SHELL_FACES):
    """Outward unit normal of every face: (N, len(faces), 3)."""
    out = []
    for face in faces:
        p0, p1, p2 = world[:, face[0]], world[:, face[1]], world[:, face[2]]
        nrm = np.cross(p1 - p0, p2 - p1)
        out.append(nrm / np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-12))
    return np.stack(out, axis=1)


def face_parts(world, f, detail):
    """Parts of shell face f for every body, as a list of (role, (N, 4, 3) quads).

    detail False: three trapezoids from the face's edges to its window (a shell with an
    opening onto the core). detail True: per corner a hub, per edge a rail, and three
    shield segments recessed below the face plane between the rails and the window."""
    i, j, k = SHELL_FACES[f]
    p = world[:, [i, j, k]]
    centre = p.mean(axis=1, keepdims=True)
    window = p + (centre - p) * WINDOW
    pairs = ((0, 1), (1, 2), (2, 0))
    if not detail:
        return [("shield", np.stack([p[:, a], p[:, b], window[:, b], window[:, a]], axis=1)) for a, b in pairs]
    nrm = normals(world, [SHELL_FACES[f]])[:, 0]
    inner = p + (centre - p) * RAIL
    sunk = -nrm[:, None, :] * RECESS
    parts = []
    for a, b in pairs:
        ha = p[:, a] + (p[:, b] - p[:, a]) * HUB
        hb = p[:, b] + (p[:, a] - p[:, b]) * HUB
        parts.append(("rail", np.stack([ha, hb, inner[:, b], inner[:, a]], axis=1)))
        parts.append(("shield", np.stack([inner[:, a], inner[:, b], window[:, b], window[:, a]], axis=1) + sunk))
    for a in range(3):
        b, c = (a + 1) % 3, (a + 2) % 3
        parts.append(("hub", np.stack([p[:, a], p[:, a] + (p[:, b] - p[:, a]) * HUB, inner[:, a],
                                       p[:, a] + (p[:, c] - p[:, a]) * HUB], axis=1)))
    return parts


def shade(base, normal, light, view, emissive=0.0):
    """Lambert plus a controlled Blinn highlight (silver: smooth, not glittering)."""
    half = light + view
    half /= np.linalg.norm(half)
    diffuse = max(0.0, float(np.dot(normal, light)))
    spec = max(0.0, float(np.dot(normal, half))) ** 28
    k = 0.48 + 0.62 * diffuse
    return tuple(int(min(255, v * k + 255 * 0.42 * spec + emissive)) for v in base)


def shade_core(base, normal, light):
    """Flat emissive core: each face one uniform tone, restrained (no clip to white)."""
    k = 0.62 + 0.38 * max(0.0, float(np.dot(normal, light)))
    return tuple(int(min(255, v * k)) for v in base)
