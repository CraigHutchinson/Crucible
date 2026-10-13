#!/usr/bin/env python3
"""Invariants of the nanite body mesh (nanite_mesh.py).

The tessellated title is gap-free only if a body's footprint is exactly its tile and its
walls are vertical; the renderer's culling and lighting need outward normals; the concept
dart needs its spine low at the nose and high at the rear, and its core on the roof. The
defaults must match NaniteBody.hpp, the single source of the dimensions. Exit code 77
(skipped) when numpy is unavailable.
"""
import math
import pathlib
import re
import sys
import unittest

try:
    import numpy as np
except ImportError:   # pragma: no cover - environment dependent
    print("numpy unavailable: skipped")
    sys.exit(77)

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import nanite_mesh  # noqa: E402

BODY = nanite_mesh.DEFAULT_BODY
SIDE = math.sqrt(3.0)


def morphed_tile():
    """An up tile (apex = corner 2) whose outline corners were pulled in."""
    return np.array([[-0.62, -0.42], [0.95, -0.50], [0.10, 0.80]])


def inside(point, triangle):
    a, b, c = triangle
    m = np.array([[a[0] - c[0], b[0] - c[0]], [a[1] - c[1], b[1] - c[1]]])
    l1, l2 = np.linalg.solve(m, point - c)
    return l1 > -1e-9 and l2 > -1e-9 and l1 + l2 < 1.0 + 1e-9


class NaniteMeshTest(unittest.TestCase):
    def bodies(self):
        corners = np.stack([nanite_mesh.dart(BODY), morphed_tile()])
        nose = np.array([0, 2])
        return corners, nose, nanite_mesh.build(corners, nose, BODY)

    def test_defaults_match_the_cpp_body(self):
        text = (pathlib.Path(__file__).resolve().parent / "NaniteBody.hpp").read_text()
        cpp = {}
        for name, value in re.findall(r"float (\w+) = ([\d.]+)F;", text):
            cpp[re.sub(r"([A-Z])", lambda m: "_" + m.group(1).lower(), name)] = float(value)
        self.assertEqual(set(cpp), set(nanite_mesh.BODY_KEYS))
        for key, value in cpp.items():
            self.assertAlmostEqual(BODY[key], value, places=6, msg=key)

    def test_dart_footprint(self):
        d = nanite_mesh.dart(BODY)
        np.testing.assert_allclose(d.mean(axis=0), [0.0, 0.0], atol=1e-12)
        self.assertAlmostEqual(d[0, 0] - d[1, 0], BODY["dart_length"])
        self.assertAlmostEqual(d[1, 1] - d[2, 1], BODY["dart_width"])
        e1, e2 = d[1] - d[0], d[2] - d[1]
        self.assertGreater(e1[0] * e2[1] - e1[1] * e2[0], 0.0)   # counter-clockwise

    def test_footprint_is_the_tile_and_walls_are_vertical(self):
        corners, nose, verts = self.bodies()
        for k in range(2):
            self.assertEqual({tuple(np.round(p, 9)) for p in verts[k, 0:3, :2]}, {tuple(np.round(p, 9)) for p in corners[k]})
            np.testing.assert_allclose(verts[k, 3:6, :2], verts[k, 0:3, :2])   # shoulder straight above
            np.testing.assert_allclose(verts[k, 0, :2], corners[k, nose[k]])   # vertex 0 is the nose
        self.assertTrue(np.all(verts[:, 0:3, 2] == 0.0))

    def test_adjacent_tiles_share_their_wall(self):
        up = np.array([[-SIDE / 2, -0.5], [SIDE / 2, -0.5], [0.0, 1.0]])
        down = np.array([[SIDE, 1.0], [0.0, 1.0], [SIDE / 2, -0.5]])
        verts = nanite_mesh.build(np.stack([up, down]), np.array([2, 2]), BODY)
        shared = {(round(SIDE / 2, 6), -0.5), (0.0, 1.0)}
        for k in range(2):
            footprint = {tuple(np.round(p, 6)) for p in verts[k, 0:3, :2]}
            self.assertTrue(shared <= footprint)

    def test_normals_point_outward(self):
        _, _, verts = self.bodies()
        world = nanite_mesh.to_world(verts, np.zeros((2, 3)), np.zeros(2), np.zeros(2), np.array([False, False]))
        nrm = nanite_mesh.normals(world)
        centre = world.mean(axis=1)
        for f, face in enumerate(nanite_mesh.FACES):
            outward = world[:, list(face)].mean(axis=1) - centre
            self.assertTrue(np.all(np.einsum("ij,ij->i", nrm[:, f], outward) > 0.0), f)

    def test_spine_rises_from_nose_to_rear_inside_the_footprint(self):
        corners, _, verts = self.bodies()
        for k in range(2):
            front, rear = verts[k, 6], verts[k, 7]
            self.assertAlmostEqual(front[2], BODY["nose_height"])
            self.assertAlmostEqual(rear[2], BODY["height"])
            self.assertTrue(np.all(verts[k, 3:6, 2] < front[2] + 1e-9))   # shoulders below the spine
            for p in (front[:2], rear[:2]):
                self.assertTrue(inside(p, verts[k, 0:3, :2]))

    def test_core_lies_on_the_rear_facets(self):
        _, _, verts = self.bodies()
        disc, halo = nanite_mesh.core(verts, BODY)
        for k in range(2):
            planes = []
            for f in nanite_mesh.CORE_FACES:
                p = verts[k, list(nanite_mesh.FACES[f])]
                n = np.cross(p[1] - p[0], p[2] - p[0])
                planes.append((p, n / np.linalg.norm(n)))
            for outline in (disc[k], halo[k]):
                for point in outline:
                    on = [abs(np.dot(point - p[0], n)) < 0.01 and inside(point[:2], p[:, :2]) for p, n in planes]
                    self.assertTrue(any(on), "core vertex on a rear facet")

    def test_upright_plane_turns_the_roof_toward_the_camera(self):
        _, _, verts = self.bodies()
        world = nanite_mesh.to_world(verts[:1], np.zeros((1, 3)), np.zeros(1), np.array([math.pi / 2]), np.array([True]))
        roof = nanite_mesh.normals(world)[0, 3:]
        self.assertTrue(np.all(roof[:, 1] < 0.0))   # every roof facet faces the camera side (-y)

    def test_header_round_trip(self):
        text = ",".join(f"{BODY[k]:.4f}" for k in nanite_mesh.BODY_KEYS)
        self.assertEqual(nanite_mesh.parse_body(text), {k: round(BODY[k], 4) for k in nanite_mesh.BODY_KEYS})
        self.assertEqual(nanite_mesh.parse_body("0.5,0.3"), BODY)   # older layout: defaults


if __name__ == "__main__":
    unittest.main()
