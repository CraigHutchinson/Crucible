#!/usr/bin/env python3
"""Invariants of presentation-rate playback (playback.py): interpolation reproduces the
captures at their own times and blends between them on the shorter arc; extrapolation
dead-reckons ground movers only; resampling covers the captured span at the asked rate;
reconstruction error is zero for straight constant-speed motion."""
import math
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import playback  # noqa: E402


def frame(shown, rows, pace=0.5):
    return {"time": shown * pace, "shown": shown, "pace": pace, "cx": 0.0, "cy": 0.0, "hw": 10.0, "yaw": 0.0, "rows": rows}


def row(x, y, heading=0.0, act="lane", plane=0.0, speed=0.0, z=0.0):
    return (x, y, z, heading, act, 2, 0.0, plane, 0, None, 0, speed)


class PlaybackTest(unittest.TestCase):
    def test_angle_takes_the_shorter_arc(self):
        a, b = math.radians(170), math.radians(-170)
        mid = playback.lerp_angle(a, b, 0.5)
        self.assertAlmostEqual(math.cos(mid), -1.0, places=6)

    def test_interpolation_hits_captures_and_blends(self):
        a = frame(1.0, [row(0.0, 0.0, act="lane"), row(5.0, 5.0, act="dock")])
        b = frame(2.0, [row(2.0, 4.0, act="lane"), row(5.0, 5.0, act="locked")])
        self.assertEqual(playback.blend_frame(a, b, 0.0)["rows"][0][:2], (0.0, 0.0))
        self.assertEqual(playback.blend_frame(a, b, 1.0)["rows"][0][:2], (2.0, 4.0))
        mid = playback.blend_frame(a, b, 0.5)
        self.assertEqual(mid["rows"][0][:2], (1.0, 2.0))
        self.assertAlmostEqual(mid["shown"], 1.5)
        self.assertEqual(playback.blend_frame(a, b, 0.4)["rows"][1][playback.ACT], "dock")     # discrete: nearer capture
        self.assertEqual(playback.blend_frame(a, b, 0.6)["rows"][1][playback.ACT], "locked")

    def test_extrapolation_moves_ground_movers_only(self):
        a = frame(1.0, [row(0.0, 0.0, heading=0.0, speed=10.0),
                        row(0.0, 0.0, heading=0.0, speed=10.0, act="locked"),
                        row(0.0, 0.0, heading=0.0, speed=10.0, plane=math.pi / 2)], pace=0.5)
        ahead = playback.predict_frame(a, 1.2)   # 0.2 s shown = 0.1 s of choreography = 1 d
        self.assertAlmostEqual(ahead["rows"][0][0], 1.0)
        self.assertEqual(ahead["rows"][1][0], 0.0)
        self.assertEqual(ahead["rows"][2][0], 0.0)
        self.assertAlmostEqual(ahead["time"], a["time"] + 0.1)

    def test_resample_rate_and_span(self):
        frames = [frame(t / 10.0, [row(t, 0.0, speed=10.0)]) for t in range(11)]   # 0..1 s shown
        out = playback.resample(frames, 60.0)
        self.assertEqual(len(out), 61)
        self.assertAlmostEqual(out[-1]["shown"], 1.0)
        self.assertAlmostEqual(out[30]["rows"][0][0], 5.0, places=6)

    def test_straight_motion_reconstructs_exactly(self):
        # 10 d per choreography second at pace 0.5: 5 d per shown second along +x.
        frames = [frame(k / 60.0, [row(5.0 * k / 60.0, 1.0, heading=0.0, speed=10.0)]) for k in range(61)]
        for mode, (mean, p95, worst) in playback.reconstruction_error(frames, 4).items():
            self.assertLess(worst, 1e-9, mode)


if __name__ == "__main__":
    unittest.main()
