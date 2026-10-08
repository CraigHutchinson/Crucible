"""Independent synthetic receipts exercise classification, never native performance."""
import copy
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("phase14_capture", ROOT / "scripts/capture_phase14_frames.py")
CAPTURE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CAPTURE)


def fixture():
    header = dict(type="capture", schema=1, source="a" * 40, source_tree="b" * 40, population=100000,
                  columns=400, route=1, completion_mode="joined-yield-zero",
                  pacing_mode="fixed-60hz-skip-whole-overdue-periods", duration_ns=30_000_000_000,
                  warmup_ticks=120, drained=True, failure="", bounded_out=False,
                  device_identity_received=True, driver_version=123, initial_tick=120,
                  dropped_frame_rows=0, dropped_input_rows=0, initial_discarded_scaled_ns=0)
    quality = dict(type="quality", compared=True, cpu_ns=1234, capture_cpu_ns=0,
                   initial_nonzero_field_samples=11000, signed_mean_dx=24)
    records = [header, quality]
    for index in range(1800):
        begin = index * 16_666_667
        records.append(dict(type="frame", run=1, frame=index + 1, tick=index + 121, draw_tick=index + 121,
                            advanced=1, authoritative_mobile=100000, individual=0, aggregated=100000,
                            hidden=0, marks=10000, begin_ns=begin, end_ns=begin + 10_000_000,
                            completion_observed_ns=begin + 9_000_000, marker_begin_ns=begin + 7_500_000,
                            marker_end_ns=begin + 8_000_000, poll_call_ns=1000, poll_calls=1,
                            handoff_observed_ns=begin + 7_000_000, native_status=0, marker_status=0,
                            visible=True, status=0, pacing_skipped=0, discarded_scaled_ns=0, width=1280, height=864))
    for index in range(300):
        frame = records[2 + index * 6]
        records.append(dict(type="trace", sequence=index + 1, tick=frame["tick"]))
        records.append(dict(type="input", run=1, sequence=index + 1, tick=frame["tick"],
                            frame=frame["frame"], event_begin_ns=frame["begin_ns"],
                            application_frame=frame["frame"],
                            admitted_ns=frame["begin_ns"] + 1_000_000,
                            applied_observed_ns=frame["begin_ns"] + 2_000_000,
                            completed_observed_ns=frame["completion_observed_ns"],
                            accepted=True, visible=True, status=0))
    return records


class ClassificationTests(unittest.TestCase):
    def setUp(self):
        self.records = fixture()

    def test_complete_receipt_uses_nearest_rank_targets(self):
        result = CAPTURE.summarize(self.records)
        self.assertTrue(result["acceptance_sample_eligible"])
        self.assertEqual(result["fullframe_p99_ns"], 10_000_000)
        self.assertEqual(result["input_p99_ns"], 9_000_000)
        self.assertTrue(result["g3_target_met"])
        self.assertTrue(result["g4_target_met"])
        self.assertTrue(result["quality_pass"])
        self.assertEqual(CAPTURE.nearest_rank([8, 2, 4], .5), 4)
        self.assertIsNone(CAPTURE.nearest_rank([], .99))

    def test_short_probe_has_no_acceptance_percentiles(self):
        self.records[0]["duration_ns"] = 29_999_999_999
        result = CAPTURE.summarize(self.records)
        self.assertFalse(result["acceptance_sample_eligible"])
        self.assertIsNone(result["fullframe_p99_ns"])
        self.assertFalse(result["g3_target_met"])

    def test_idle_redraws_cannot_receive_evolving_frame_gate(self):
        rows = [row for row in self.records if row["type"] not in ("input", "trace")]
        rows[0]["route"] = 0
        for row in rows:
            if row["type"] == "frame":
                row.update(advanced=0, tick=120, draw_tick=120)
        result = CAPTURE.summarize(rows)
        self.assertFalse(result["acceptance_sample_eligible"])
        self.assertEqual(result["zero_tick_frames"], 1800)
        self.assertIsNone(result["fullframe_p99_ns"])

    def test_corrupt_tick_or_backward_time_is_rejected(self):
        self.records[2]["tick"] += 1
        with self.assertRaisesRegex(ValueError, "Tick delta"):
            CAPTURE.summarize(self.records)
        self.records = fixture()
        self.records[3]["begin_ns"] = self.records[2]["end_ns"] - 1
        with self.assertRaisesRegex(ValueError, "backwards"):
            CAPTURE.summarize(self.records)

    def test_completion_order_and_missing_driver_are_not_accepted(self):
        self.records[2]["handoff_observed_ns"] = self.records[2]["marker_end_ns"] + 1
        with self.assertRaisesRegex(ValueError, "ordered"):
            CAPTURE.summarize(self.records)
        self.records = fixture()
        self.records[0]["driver_version"] = None
        self.assertFalse(CAPTURE.summarize(self.records)["acceptance_sample_eligible"])

    def test_insufficient_inputs_or_warmup_blocks(self):
        self.records[0]["warmup_ticks"] = 119
        self.assertFalse(CAPTURE.summarize(self.records)["acceptance_sample_eligible"])
        self.records[0]["warmup_ticks"] = 120
        self.records[-1]["accepted"] = False
        self.assertFalse(CAPTURE.summarize(self.records)["acceptance_sample_eligible"])

    def test_population_and_correlated_identity_are_checked(self):
        self.records[2]["hidden"] = 1
        with self.assertRaisesRegex(ValueError, "accounting"):
            CAPTURE.summarize(self.records)
        self.records = fixture()
        self.records[-1]["frame"] = 99999
        with self.assertRaisesRegex(ValueError, "checked frame"):
            CAPTURE.summarize(self.records)

    def test_occlusion_and_full_marker_are_visible_nonqualifying(self):
        self.records[100]["native_status"] = 2
        self.records[101]["marker_status"] = 1
        result = CAPTURE.summarize(self.records)
        self.assertFalse(result["acceptance_sample_eligible"])
        self.assertEqual(result["no_handoff_frames"], 1)
        self.assertEqual(result["marker_full_frames"], 1)

    def test_discarded_time_cannot_be_hidden_as_tick_rate(self):
        self.records[1801]["discarded_scaled_ns"] = 60
        result = CAPTURE.summarize(self.records)
        self.assertEqual(result["discarded_scaled_ns_delta"], 60)
        self.assertFalse(result["g4_target_met"])

    def test_one_outlier_cannot_receive_front_quality(self):
        self.records[1]["signed_mean_dx"] = 1
        self.records[1]["signed_max_dx"] = 300
        self.assertFalse(CAPTURE.summarize(self.records)["quality_pass"])
        self.records[1]["signed_mean_dx"] = 24
        self.records[1]["initial_nonzero_field_samples"] = 9999
        self.assertFalse(CAPTURE.summarize(self.records)["quality_pass"])

    def test_source_and_slow_outlier_receipts_are_retained(self):
        slow = copy.deepcopy(self.records)
        slow[1801]["end_ns"] += 1_000_000_000
        slow[0]["duration_ns"] = 31_000_000_000
        result = CAPTURE.summarize(slow)
        self.assertEqual(result["source"], "a" * 40)
        self.assertEqual(result["source_tree"], "b" * 40)
        self.assertTrue(result["acceptance_sample_eligible"])
        self.assertTrue(result["g3_target_met"])
        self.assertEqual(result["fullframe_max_ns"], 1_010_000_000)
        self.assertEqual(result["fullframe_over_20ms_frames"], 1)
        self.assertEqual(result["fullframe_p99_ns"], 10_000_000)


if __name__ == "__main__":
    unittest.main()
