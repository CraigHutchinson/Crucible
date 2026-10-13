#!/usr/bin/env python3
"""Invariants of the nanite body mesh (nanite_mesh.py), the tetrahedral proxy.

The docked letter's footprint is gap-free only if a shell's base is exactly its tile; the
stacking layer is the apex height; the renderer's culling and lighting need outward
normals; the design space asks for ONE regular tetrahedron core, protected inside the shell
and clear of the surface; the shell's parts must cover each face around one window. The
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


def up_tile():
    return np.array([[-SIDE / 2, -0.5], [SIDE / 2, -0.5], [0.0, 1.0]])


def inside(point, triangle):
    a, b, c = triangle
    m = np.array([[a[0] - c[0], b[0] - c[0]], [a[1] - c[1], b[1] - c[1]]])
    l1, l2 = np.linalg.solve(m, point - c)
    return l1 > -1e-9 and l2 > -1e-9 and l1 + l2 < 1.0 + 1e-9


def area(poly):
    """Area of a planar polygon in 3D."""
    total = np.zeros(3)
    for i in range(len(poly)):
        total += np.cross(poly[i], poly[(i + 1) % len(poly)])
    return 0.5 * np.linalg.norm(total)


class NaniteMeshTest(unittest.TestCase):
    def bodies(self):
        corners = np.stack([nanite_mesh.footprint(BODY), morphed_tile(), up_tile()])
        nose = np.array([0, 2, 2])
        return corners, nose, nanite_mesh.shell(corners, nose, BODY)

    def test_defaults_match_the_cpp_body(self):
        text = (pathlib.Path(__file__).resolve().parent / "NaniteBody.hpp").read_text()
        cpp = {}
        for name, value in re.findall(r"float (\w+) = ([\d.]+)F;", text):
            cpp[re.sub(r"([A-Z])", lambda m: "_" + m.group(1).lower(), name)] = float(value)
        self.assertEqual(set(cpp), set(nanite_mesh.BODY_KEYS))
        for key, value in cpp.items():
            self.assertAlmostEqual(BODY[key], value, places=6, msg=key)

    def test_free_footprint_is_equilateral(self):
        d = nanite_mesh.footprint(BODY)
        np.testing.assert_allclose(d.mean(axis=0), [0.0, 0.0], atol=1e-12)
        for i in range(3):
            self.assertAlmostEqual(np.linalg.norm(d[i] - d[(i + 1) % 3]), BODY["span"])
        self.assertAlmostEqual(d[0, 1], 0.0)   # nose along the heading
        e1, e2 = d[1] - d[0], d[2] - d[1]
        self.assertGreater(e1[0] * e2[1] - e1[1] * e2[0], 0.0)   # counter-clockwise

    def test_base_is_the_tile_and_apex_is_one_layer_up(self):
        corners, nose, verts = self.bodies()
        for k in range(len(corners)):
            self.assertEqual({tuple(np.round(p, 9)) for p in verts[k, 0:3, :2]}, {tuple(np.round(p, 9)) for p in corners[k]})
            np.testing.assert_allclose(verts[k, 0, :2], corners[k, nose[k]])   # vertex 0 is the nose
            self.assertAlmostEqual(verts[k, 3, 2], BODY["height"])
            self.assertTrue(inside(verts[k, 3, :2], verts[k, 0:3, :2]))
            centroid = verts[k, 0:3, :2].mean(axis=0)
            lean = np.dot(verts[k, 3, :2] - centroid, verts[k, 0, :2] - centroid)
            self.assertGreater(lean, 0.0)   # leaned toward the nose: the heading reads
        self.assertTrue(np.all(verts[:, 0:3, 2] == 0.0))

    def test_adjacent_tiles_share_their_base_edge(self):
        down = np.array([[SIDE, 1.0], [0.0, 1.0], [SIDE / 2, -0.5]])
        verts = nanite_mesh.shell(np.stack([up_tile(), down]), np.array([2, 2]), BODY)
        shared = {(round(SIDE / 2, 6), -0.5), (0.0, 1.0)}
        for k in range(2):
            self.assertTrue(shared <= {tuple(np.round(p, 6)) for p in verts[k, 0:3, :2]})

    def test_normals_point_outward(self):
        _, _, verts = self.bodies()
        n = len(verts)
        world = nanite_mesh.to_world(verts, np.zeros((n, 3)), np.zeros(n), np.zeros(n), np.zeros(n, dtype=bool))
        nrm = nanite_mesh.normals(world)
        centre = world.mean(axis=1)
        for f, face in enumerate(nanite_mesh.SHELL_FACES):
            outward = world[:, list(face)].mean(axis=1) - centre
            self.assertTrue(np.all(np.einsum("ij,ij->i", nrm[:, f], outward) > 0.0), f)
        self.assertTrue(np.all(nrm[:, 0, 2] < 0.0))   # face 0 is the base, facing down

    def test_one_regular_core_protected_inside_the_shell(self):
        _, _, verts = self.bodies()
        heart = nanite_mesh.core(verts, BODY)
        self.assertEqual(heart.shape, (len(verts), 4, 3))   # four vertices: one tetrahedron
        nrm = nanite_mesh.normals(verts)
        for k in range(len(verts)):
            edges = [np.linalg.norm(heart[k, i] - heart[k, j]) for i in range(4) for j in range(i + 1, 4)]
            self.assertAlmostEqual(min(edges), max(edges), places=9)   # regular: six equal edges
            base = verts[k, 0:3, :2]
            mean_edge = np.mean([np.linalg.norm(base[i] - base[(i + 1) % 3]) for i in range(3)])
            self.assertLessEqual(edges[0], BODY["core"] * mean_edge + 1e-9)
            self.assertGreater(edges[0], 0.25 * mean_edge)   # substantial, not a speck
            self.assertTrue(np.all(heart[k, :, 2] > 0.0))      # clear of the surface
            for f, face in enumerate(nanite_mesh.SHELL_FACES):
                reach = (heart[k] - verts[k, face[0]]) @ nrm[k, f]
                self.assertTrue(np.all(reach <= -nanite_mesh.CLEARANCE + 1e-9), (k, f))
        world = nanite_mesh.to_world(heart, np.zeros((len(verts), 3)), np.zeros(len(verts)), np.zeros(len(verts)), np.zeros(len(verts), dtype=bool))
        cn = nanite_mesh.normals(world, nanite_mesh.CORE_FACES)
        centre = world.mean(axis=1)
        for f, face in enumerate(nanite_mesh.CORE_FACES):
            outward = world[:, list(face)].mean(axis=1) - centre
            self.assertTrue(np.all(np.einsum("ij,ij->i", cn[:, f], outward) > 0.0), f)

    def test_parts_cover_each_face_around_one_window(self):
        _, _, verts = self.bodies()
        for f, face in enumerate(nanite_mesh.SHELL_FACES):
            whole = np.array([area(verts[k, list(face)]) for k in range(len(verts))])
            window = whole * (1.0 - nanite_mesh.WINDOW) ** 2
            for detail in (False, True):
                parts = nanite_mesh.face_parts(verts, f, detail)
                if detail:
                    self.assertEqual(sorted(role for role, _ in parts), ["hub"] * 3 + ["rail"] * 3 + ["shield"] * 3)
                covered = np.array([sum(area(quad[k]) for _, quad in parts) for k in range(len(verts))])
                np.testing.assert_allclose(covered + window, whole, rtol=1e-9)
            # Shields sit inside the face plane (recessed); rails and hubs on it.
            nrm = nanite_mesh.normals(verts)[:, f]
            for role, quad in nanite_mesh.face_parts(verts, f, True):
                depth = np.einsum("ikj,ij->ik", quad - verts[:, face[0]][:, None], nrm)
                expected = -nanite_mesh.RECESS if role == "shield" else 0.0
                np.testing.assert_allclose(depth, expected, atol=1e-9)

    def test_upright_plane_turns_the_sides_toward_the_camera(self):
        _, _, verts = self.bodies()
        world = nanite_mesh.to_world(verts[:1], np.zeros((1, 3)), np.zeros(1), np.array([math.pi / 2]), np.array([True]))
        sides = nanite_mesh.normals(world)[0, 1:]
        self.assertTrue(np.all(sides[:, 1] < 0.0))   # every side faces the camera side (-y)

    def test_header_round_trip(self):
        text = ",".join(f"{BODY[k]:.4f}" for k in nanite_mesh.BODY_KEYS)
        self.assertEqual(nanite_mesh.parse_body(text), {k: round(BODY[k], 4) for k in nanite_mesh.BODY_KEYS})
        self.assertEqual(nanite_mesh.parse_body("0.55,0.22,0.40,0.28,0.14,1.90,0.90,0.36"), BODY)   # S0j dart layout: defaults


if __name__ == "__main__":
    unittest.main()
