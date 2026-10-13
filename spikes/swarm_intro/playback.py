"""Presentation-rate playback of frame captures: interpolation and extrapolation.

The simulation ticks at its own rate; a renderer draws at the display's. Between two
captures a frame can be interpolated (smooth, but one capture behind) or extrapolated from
the latest capture (current, but it guesses). Captures list units in identity order, so row
k is the same unit in every frame.

Continuous fields blend (position, heading on the shortest arc, plane tilt, speed, camera);
discrete ones (act, tier, magnet, layer, the morphed shape) come from the nearer capture.
Extrapolation dead-reckons a ground mover along its heading at its speed (choreography
distance per presented second is speed x pace); units on the wall or resting hold. This is a
review tool and the reference behaviour for S1's renderer, not the renderer itself.
"""
import bisect
import math

# Row layout (render_swarm.load): x, y, z, heading, act, tier, since, plane, magnet, shape, layer, speed.
X, Y, Z, HEADING, ACT, TIER, SINCE, PLANE, MAGNET, SHAPE, LAYER, SPEED = range(12)
RESTING = ("dormant", "locked", "moored", "reserve")


def lerp(a, b, f):
    return a + (b - a) * f


def lerp_angle(a, b, f):
    """Interpolate an angle along the shorter arc."""
    d = (b - a + math.pi) % (2.0 * math.pi) - math.pi
    return a + d * f


def blend_row(a, b, f):
    near = a if f < 0.5 else b
    return (lerp(a[X], b[X], f), lerp(a[Y], b[Y], f), lerp(a[Z], b[Z], f), lerp_angle(a[HEADING], b[HEADING], f),
            near[ACT], near[TIER], near[SINCE], lerp(a[PLANE], b[PLANE], f), near[MAGNET], near[SHAPE], near[LAYER],
            lerp(a[SPEED], b[SPEED], f))


def blend_frame(a, b, f):
    """A frame f of the way from capture a to capture b (same units, same order)."""
    out = dict(a if f < 0.5 else b)
    out["time"] = lerp(a["time"], b["time"], f)
    out["shown"] = lerp(a["shown"], b["shown"], f)
    out["cx"] = lerp(a["cx"], b["cx"], f)
    out["cy"] = lerp(a["cy"], b["cy"], f)
    out["hw"] = math.exp(lerp(math.log(a["hw"]), math.log(b["hw"]), f))
    out["yaw"] = lerp(a["yaw"], b["yaw"], f)
    out["rows"] = [blend_row(ra, rb, f) for ra, rb in zip(a["rows"], b["rows"])]
    return out


def predict_frame(a, shown):
    """Dead-reckon capture a forward to presentation time `shown`."""
    ahead = max(0.0, shown - a["shown"])
    travel = ahead * a["pace"]   # choreography seconds
    rows = []
    for r in a["rows"]:
        if r[ACT] in RESTING or r[PLANE] > 0.1:
            rows.append(r)
            continue
        step = r[SPEED] * travel
        rows.append((r[X] + math.cos(r[HEADING]) * step, r[Y] + math.sin(r[HEADING]) * step) + tuple(r[Z:]))
    out = dict(a)
    out["time"] = a["time"] + travel
    out["shown"] = shown
    out["rows"] = rows
    return out


def resample(frames, fps, extrapolate=False):
    """Frames at `fps` presentation frames per second across the captures."""
    shown = [f["shown"] for f in frames]
    out = []
    t = shown[0]
    step = 1.0 / fps
    while t <= shown[-1] + 1e-9:
        i = max(0, bisect.bisect_right(shown, t + 1e-9) - 1)
        if extrapolate or i + 1 >= len(frames):
            out.append(predict_frame(frames[i], t))
        else:
            a, b = frames[i], frames[i + 1]
            span = b["shown"] - a["shown"]
            out.append(blend_frame(a, b, 0.0 if span <= 0.0 else (t - a["shown"]) / span))
        t += step
    return out


def reconstruction_error(frames, stride):
    """Position error (diameters) when only every `stride`-th capture is kept and the frames
    between are rebuilt by interpolation or by extrapolation, against the real captures.
    Ground movers only. Returns {"interpolate": (mean, p95, max), "extrapolate": (...)}."""
    kept = frames[::stride]
    errors = {"interpolate": [], "extrapolate": []}
    for k in range(len(kept) - 1):
        a, b = kept[k], kept[k + 1]
        for j in range(1, stride):
            truth = frames[k * stride + j]
            span = b["shown"] - a["shown"]
            f = (truth["shown"] - a["shown"]) / span if span > 0.0 else 0.0
            guesses = {"interpolate": blend_frame(a, b, f), "extrapolate": predict_frame(a, truth["shown"])}
            for mode, guess in guesses.items():
                for g, r in zip(guess["rows"], truth["rows"]):
                    if r[ACT] in RESTING or r[PLANE] > 0.1:
                        continue
                    errors[mode].append(math.hypot(g[X] - r[X], g[Y] - r[Y]))
    out = {}
    for mode, values in errors.items():
        values.sort()
        if not values:
            out[mode] = (0.0, 0.0, 0.0)
            continue
        out[mode] = (sum(values) / len(values), values[int(0.95 * (len(values) - 1))], values[-1])
    return out
