"""Cold exact-source oracle generation; missing NumPy is a failure, never a skip.

Run from any directory. Outputs are checked in; C++ tests consume these bytes
without importing Python or fetching dependencies. No production code is used.
"""
import ast
import hashlib
import json
import math
import pathlib
import re
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[4]
SOURCE = ROOT / "spikes/swarm_intro"
sys.path.insert(0, str(SOURCE))
import nanite_mesh as reference  # noqa: E402

ROLE = {"shield": 0, "rail": 1, "hub": 2, "core": 3}
DEFAULT_SHAPE = [-0.62, -0.42, 0.95, -0.50, 0.10, 0.80]


def f32(value):
    return float(np.float32(value))


def row(**changes):
    values = dict(x=0., y=0., z=0., heading=0., plane=0., act=2, tier=0,
                  since=0., magnet=0, layer=0, shaped=False, shape=[0.] * 6, speed=0.)
    values.update(changes)
    for name in ("x", "y", "z", "heading", "plane", "since", "speed"):
        values[name] = f32(values[name])
    values["shape"] = [f32(v) for v in values["shape"]]
    return values


def case(name, rows, detail=False, time=1., yaw=0.):
    return dict(name=name, rows=rows, time=f32(time), tilt=f32(.4), diameter=1.,
                camera=[0., 0., 10., f32(16 / 9), 1920. if detail else 100., f32(yaw)])


def build_cases():
    cases = [case("free_rest_low", [row()]),
             case("free_full_speed_low", [row(speed=24.)]),
             case("free_rest_detail", [row()], True),
             case("free_full_speed_detail", [row(speed=24., heading=.37)], True),
             case("free_mid_speed_hinge", [row(speed=12., heading=-.52, plane=math.pi / 4)], True, yaw=23.)]
    for plane, label in ((0., "ground"), (math.pi / 4, "hinged"), (math.pi / 2, "upright")):
        for detail in (False, True):
            cases.append(case(f"morphed_{label}_{'detail' if detail else 'low'}",
                              [row(shaped=True, shape=DEFAULT_SHAPE, heading=1.2, plane=plane,
                                   act=6, speed=24.)], detail, time=12., yaw=-18.))
    for act in range(11):
        cases.append(case(f"act_{act}", [row(act=act, plane=0., z=.2)], True, time=1.))
    for time, label in ((.999999, "before"), (1., "exact"), (1.000001, "after")):
        cases.append(case(f"reserve_boundary_{label}", [row(act=9)], True, time=time))
    cases.append(case("reserve_nonzero_since", [row(act=9, since=.1)], True, time=1.1))
    cases.append(case("layered_locked", [row(act=6, layer=0), row(x=2., act=5, layer=2)], True, time=12.))
    return cases


def main():
    body_text = (SOURCE / "NaniteBody.hpp").read_text(encoding="utf-8")
    body = {re.sub(r"([A-Z])", lambda m: "_" + m.group(1).lower(), name): f32(float(value))
            for name, value in re.findall(r"float (\w+) = ([\d.]+)F;", body_text)}
    render_text = (SOURCE / "render_swarm.py").read_text(encoding="utf-8")
    colours = {}
    for name in ("SILVER", "SILVER_DIM", "CYAN", "CYAN_DIM", "CORE_SETTLED"):
        colours[name] = np.array(ast.literal_eval(re.search(rf"^{name} = (.+)$", render_text, re.M).group(1)), dtype=float)
    luminance = lambda value: float(np.dot(value, [.2126, .7152, .0722]))
    lines = ["CRUCIBLE_NANITE_PROXY_ORACLE 1"]
    cases = build_cases()
    lines.append(str(len(cases)))
    for item in cases:
        rows = item["rows"]
        n = len(rows)
        body_values = [body[key] for key in reference.BODY_KEYS]
        lines.extend([f"{item['name']} {n}", " ".join(format(v, ".17g") for v in body_values),
                      " ".join(format(v, ".17g") for v in item["camera"]),
                      " ".join(format(item[k], ".17g") for k in ("tilt", "time", "diameter"))])
        for r in rows:
            values = [r[k] for k in ("x", "y", "z", "heading", "plane", "act", "tier", "since", "magnet", "layer", "shaped")]
            values += r["shape"] + [r["speed"]]
            lines.append(" ".join(format(float(v), ".17g") for v in values))
        corners = np.array([r["shape"] if r["shaped"] else reference.footprint(body).reshape(-1) for r in rows]).reshape(n, 3, 2)
        shaped = np.array([r["shaped"] for r in rows])
        local = reference.shell(corners, np.where(shaped, 2, 0), body)
        origin = np.array([[r[k] for k in ("x", "y", "z")] for r in rows])
        heading = np.array([r["heading"] for r in rows])
        plane = np.array([r["plane"] for r in rows])
        heart = reference.to_world(reference.core(local, body), origin, heading, plane, ~shaped)
        amount = np.where(shaped, 0., reference.stretch_amount([r["speed"] for r in rows], body))
        world = reference.to_world(reference.elongate(local, amount, body), origin, heading, plane, ~shaped)
        cx, cy, hw, aspect, pixels, yaw = item["camera"]
        c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        candidates = []
        for index, r in enumerate(rows):
            dx = (r["x"] - cx) * c - r["y"] * s
            dy = r["z"] + item["tilt"] * ((r["x"] - cx) * s + r["y"] * c) - cy
            if pixels / (2 * hw) * item["diameter"] >= 14 and abs(dx) <= hw + 2 * item["diameter"] and abs(dy) <= hw / aspect + 2 * item["diameter"]:
                candidates.append(((dx / hw)**2 + (dy / (hw / aspect))**2, index))
        detailed = {index for _, index in sorted(candidates)[:128]}
        vertices = []

        def emit(points, normal, role, intensity):
            for p in points:
                converted = np.asarray([p[0], p[2], -p[1]], dtype=np.float32)
                converted_normal = np.asarray([normal[0], normal[2], -normal[1]], dtype=np.float32)
                dx = (float(converted[0]) - cx) * c + float(converted[2]) * s
                dy = float(converted[1]) + item["tilt"] * ((float(converted[0]) - cx) * s - float(converted[2]) * c) - cy
                vertices.append([*converted, *converted_normal, ROLE[role], f32(intensity), dx / hw, dy / (hw / aspect)])

        deepest = max(r["layer"] for r in rows)
        for k, r in enumerate(rows):
            colour = colours["SILVER_DIM"].copy() if r["act"] in (0, 9) else colours["SILVER"].copy()
            if r["act"] == 4 or (r["plane"] < .1 and r["act"] not in (0, 6, 8, 9) and r["z"] > .1):
                colour = np.minimum(colour + 30., 255.)
            shell_intensity = luminance(colour) / luminance(colours["SILVER"])
            if deepest and r["act"] in (6, 5):
                shell_intensity *= 1 - .22 * (deepest - r["layer"]) / deepest
            core_colour = colours["CYAN_DIM"] if r["act"] == 0 or (r["act"] == 9 and item["time"] - r["since"] > 1.) else (
                colours["CORE_SETTLED"] if r["act"] in (6, 8) else colours["CYAN"])
            core_intensity = luminance(core_colour) / luminance(colours["CYAN"])
            nrm = reference.normals(world)
            for f in range(4):
                for role, batch in reference.face_parts(world, f, k in detailed):
                    quad = batch[k]
                    emit(quad[[0, 1, 2]], nrm[k, f], role, shell_intensity)
                    emit(quad[[0, 2, 3]], nrm[k, f], role, shell_intensity)
            core_normals = reference.normals(heart)
            for f, face in enumerate(reference.CORE_FACES):
                emit(heart[k, list(face)], core_normals[k, f], "core", core_intensity)
        lines.append(f"{len(vertices)} {len(detailed)}")
        lines.extend(" ".join(format(float(v), ".17g") for v in vertex) for vertex in vertices)
    destination = pathlib.Path(__file__).with_name("nanite_proxy_reference.txt")
    destination.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    digest = lambda path: hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()
    sources = [SOURCE / name for name in ("NaniteBody.hpp", "nanite_mesh.py", "render_swarm.py")]
    manifest = dict(format=1, text_hash_format="UTF-8 with canonical LF newlines", cases=len(cases), generator_sha256=digest(pathlib.Path(__file__)),
                    sources={str(path.relative_to(ROOT)).replace("\\", "/"): digest(path) for path in sources},
                    oracle_sha256=digest(destination), numpy_version=np.__version__, runtime_numpy_required=False)
    destination.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"Generated {len(cases)} exact-source cases: {destination}")


if __name__ == "__main__":
    main()
