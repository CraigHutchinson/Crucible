#!/usr/bin/env python3
"""Invariants of the nanite body mesh (nanite_mesh.py).

The tessellated title is gap-free only if a body's footprint is exactly its tile and its
walls are vertical; the renderer's culling and lighting need outward normals; the brief
needs a wedge (nose lower than the rear) with its core on the top face. The defaults must
match NaniteBody.hpp, the single source of the dimensions. Exit code 77 (skipped) when
numpy is unavailable.
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


def morphed():
    """An up tile whose outline corners were pulled in (area about 0.6 of equilateral)."""
    return np.array([[-0.62, -0.42], [0.95, -0.50], [0.10, 0.80]])


class NaniteMeshTest(unittest.TestCase):
    def bodies(self):
        corners = np.stack([nanite_mesh.equilateral(SIDE), morphed()])
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

    def test_footprint_is_the_tile_and_walls_are_vertical(self):
        corners, _, verts = self.bodies()
        np.testing.assert_allclose(verts[:, 0:3, :2], corners)
        np.testing.assert_allclose(verts[:, 3:6, :2], corners)   # shoulder straight above
        self.assertTrue(np.all(verts[:, 0:3, 2] == 0.0))

    def test_adjacent_tiles_share_their_wall(self):
        # Two tiles sharing an edge: the shared wall is the same quad, traversed both ways.
        up = np.array([[-SIDE / 2, -0.5], [SIDE / 2, -0.5], [0.0, 1.0]])
        down = np.array([[SIDE / 2 + SIDE / 2, 1.0], [0.0, 1.0], [SIDE / 2, -0.5]])
        verts = nanite_mesh.build(np.stack([up, down]), np.array([2, 2]), BODY)
        edge_up = {tuple(np.round(p, 6)) for p in verts[0][[1, 2], :2]}
        edge_down = {tuple(np.round(p, 6)) for p in verts[1][[1, 2], :2]}
        self.assertEqual(edge_up, edge_down)

    def test_normals_point_outward(self):
        _, _, verts = self.bodies()
        world = nanite_mesh.to_world(verts, np.zeros((2, 3)), np.zeros(2), np.zeros(2), np.array([False, False]))
        nrm = nanite_mesh.normals(world)
        centre = world.mean(axis=1)
        for f, face in enumerate(nanite_mesh.FACES):
            outward = world[:, list(face)].mean(axis=1) - centre
            self.assertTrue(np.all(np.einsum("ij,ij->i", nrm[:, f], outward) > 0.0), nanite_mesh.ROLE[f])

    def test_wedge_nose_is_lower_and_top_is_inset(self):
        corners, nose, verts = self.bodies()
        for k in range(2):
            top = verts[k, 6:9, 2]
            self.assertAlmostEqual(top[nose[k]], BODY["nose_height"])
            self.assertTrue(np.all(np.delete(top, nose[k]) > top[nose[k]]))
            # Top corners lie strictly inside the footprint (barycentric weights > 0).
            a, b, c = corners[k]
            m = np.array([[a[0] - c[0], b[0] - c[0]], [a[1] - c[1], b[1] - c[1]]])
            for p in verts[k, 6:9, :2]:
                l1, l2 = np.linalg.solve(m, p - c)
                self.assertTrue(l1 > 0.0 and l2 > 0.0 and l1 + l2 < 1.0)

    def test_core_lies_on_the_top_face(self):
        _, nose, verts = self.bodies()
        disc, halo = nanite_mesh.core(verts, nose, BODY)
        for k in range(2):
            top = verts[k, 6:9]
            normal = np.cross(top[1] - top[0], top[2] - top[0])
            normal /= np.linalg.norm(normal)
            a, b, c = top[:, :2]
            m = np.array([[a[0] - c[0], b[0] - c[0]], [a[1] - c[1], b[1] - c[1]]])
            for p in halo[k]:
                self.assertLess(abs(np.dot(p - top[0], normal)), 0.01)   # on the face plane
                l1, l2 = np.linalg.solve(m, p[:2] - c)
                self.assertTrue(l1 > 0.0 and l2 > 0.0 and l1 + l2 < 1.0, "core ring inside the top face")

    def test_upright_plane_turns_the_body_toward_the_camera(self):
        _, _, verts = self.bodies()
        world = nanite_mesh.to_world(verts[:1], np.zeros((1, 3)), np.zeros(1), np.array([math.pi / 2]), np.array([True]))
        top = nanite_mesh.normals(world)[0, -1]
        self.assertLess(top[1], -0.9)   # the sloped top faces the camera side (-y), leaning along the slope

    def test_header_round_trip(self):
        text = ",".join(f"{BODY[k]:.4f}" for k in nanite_mesh.BODY_KEYS)
        self.assertEqual(nanite_mesh.parse_body(text), {k: round(BODY[k], 4) for k in nanite_mesh.BODY_KEYS})


if __name__ == "__main__":
    unittest.main()
