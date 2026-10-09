"""Independent synthetic receipts exercise classification, never native performance."""
import copy
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("phase14_capture", ROOT / "scripts/capture_phase14_frames.py")
CAPTURE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CAPTURE)


def camera_fixture(records):
    """Supply a known complete trajectory; rejected receipts below deliberately corrupt its actual values."""
    header = records[0]
    header.update(camera_script="overview-detail-pan-v1", camera_mode="fixed-overview" if header["route"] == 0 else "six-frame-cycle",
                  camera_viewport_width=1232, camera_viewport_height=520)
    scale, detail = (2.08, 5.1757056) if header["population"] == 100000 else (520 / 300, 4.313088)
    center = header["columns"] / 2, header["rows"] / 2
    fit = scale, *center
    close = detail, *center
    pan = detail, center[0] - 24 / detail, center[1] - 12 / detail
    trajectory = (fit, close, pan, close, fit, fit)
    previous = fit
    for index, frame in enumerate(row for row in records if row["type"] == "frame"):
        phase = -1 if header["route"] == 0 else index % 6
        after = fit if phase == -1 else trajectory[phase]
        frame.update(camera_phase=phase, camera_events=0 if phase == -1 else 3 if phase in (2, 3) else 1,
                     camera_transitions=int(previous != after), operator_camera_events=0,
                     camera_scale_before=previous[0], camera_center_x_before=previous[1], camera_center_y_before=previous[2],
                     camera_scale=after[0], camera_center_x=after[1], camera_center_y=after[2])
        if phase in (1, 2, 3):
            frame.update(individual=50000, aggregated=0, hidden=header["population"] - 50000, marks=0)
        else:
            frame.update(individual=0, aggregated=header["population"], hidden=0, marks=10000)
        previous = after


def fixture():
    header = dict(type="capture", schema=3, source="a" * 40, source_tree="b" * 40, population=100000,
                  columns=400, rows=250, cell_size=1, route=1, completion_mode="joined-yield-zero",
                  pacing_mode="fixed-60hz-skip-whole-overdue-periods", duration_ns=30_000_000_000,
                  warmup_ticks=120, drained=True, failure="", bounded_out=False,
                  device_identity_received=True, driver_version=123, initial_tick=120,
                  dropped_frame_rows=0, dropped_input_rows=0, initial_discarded_scaled_ns=0,
                  internal_timeout_seconds=1200, tick_observation_capacity=40124, tick_observation_count=1920,
                  initial_observation_cursor=120, dropped_tick_observations=0, profile_stages=False)
    header.update(execution_schema=1, requested_workers=1, requested_partitions=0,
                  resolved_workers=1, resolved_partitions=1, row_execution_backend="row-partitions",
                  partition_query_scratch_capacity=100000, query_scratch_element_bytes=8,
                  partition_query_scratch_bytes=800000, row_task_capacity=1, row_queue_capacity=0,
                  row_fallbacks_start=0, row_fallbacks_end=0)
    header.update(memory_schema=1, memory_api="K32GetProcessMemoryInfo",
                  memory_peak_scope="process-lifetime-through-sample",
                  memory_sample_scope="after-native-setup-and-after-measured-drain-before-cold-quality",
                  memory_startup=dict(received=True, error=0, working_set_bytes=1000000,
                                      peak_working_set_bytes=1500000, private_committed_bytes=900000, sample_call_ns=10),
                  memory_end=dict(received=True, error=0, working_set_bytes=2000000,
                                  peak_working_set_bytes=2500000, private_committed_bytes=1900000, sample_call_ns=12))
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
                            visible=True, status=0, pacing_skipped=0, discarded_scaled_ns=0, width=1280, height=864,
                            pump_ns=6_000_000))
    for index in range(1800):
        records.append(dict(type="tick", run=1, frame=index + 1, tick=index + 121,
                            boundary_ns=5_000_000, applied_commands=int(index % 6 == 0), simulation=None))
    for index in range(300):
        frame = records[2 + index * 6]
        records.append(dict(type="trace", sequence=index + 1, tick=frame["tick"]))
        records.append(dict(type="input", run=1, sequence=index + 1, tick=frame["tick"],
                            frame=frame["frame"], event_begin_ns=frame["begin_ns"],
                            application_frame=frame["frame"],
                            event_frame=frame["frame"],
                            admitted_ns=frame["begin_ns"] + 1_000_000,
                            applied_observed_ns=frame["begin_ns"] + 2_000_000,
                            completed_observed_ns=frame["completion_observed_ns"],
                            accepted=True, visible=True, status=0))
    camera_fixture(records)
    return records


class ClassificationTests(unittest.TestCase):
    def setUp(self):
        self.records = fixture()

    def test_complete_receipt_uses_nearest_rank_targets(self):
        result = CAPTURE.summarize(self.records)
        self.assertTrue(result["acceptance_sample_eligible"])
        self.assertEqual(result["fullframe_p99_ns"], 10_000_000)
        self.assertEqual(result["tick_boundary_p95_ns"], 5_000_000)
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
        rows = [row for row in self.records if row["type"] not in ("input", "trace", "tick")]
        rows[0]["route"] = 0
        camera_fixture(rows)
        rows[0]["tick_observation_count"] = 120
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

    def test_four_ticks_in_one_pump_keep_actual_boundary_distribution(self):
        frames = [row for row in self.records if row["type"] == "frame"]
        self.records = [row for row in self.records if row["type"] != "tick"]
        for index, frame in enumerate(frames):
            frame.update(advanced=4, tick=120 + (index + 1) * 4, draw_tick=120 + (index + 1) * 4)
            for offset, duration in enumerate((1_000_000, 1_000_000, 1_000_000, 2_000_000)):
                self.records.append(dict(type="tick", run=1, frame=frame["frame"], tick=121 + index * 4 + offset,
                                         boundary_ns=duration, applied_commands=int(index % 6 == 0 and offset == 3), simulation=None))
        for row in self.records:
            if row["type"] == "input":
                row["tick"] = frames[row["application_frame"] - 1]["tick"]
            elif row["type"] == "trace":
                row["tick"] = frames[(row["sequence"] - 1) * 6]["tick"]
        self.records[0]["tick_observation_count"] = 7320
        result = CAPTURE.summarize(self.records)
        self.assertEqual(result["tick_observation_count"], 7200)
        self.assertEqual(result["tick_boundary_p95_ns"], 2_000_000)
        self.assertNotEqual(result["tick_boundary_p95_ns"], 6_000_000 // 4)

    def test_missing_tick_or_drop_blocks_all_acceptance_percentiles(self):
        rows = [row for row in self.records if not (row["type"] == "tick" and row["tick"] == 122)]
        result = CAPTURE.summarize(rows)
        self.assertFalse(result["complete_tick_coverage"])
        self.assertFalse(result["acceptance_sample_eligible"])
        self.assertIsNone(result["tick_boundary_p95_ns"])
        self.assertIsNone(result["fullframe_p95_ns"])
        self.records[0]["dropped_tick_observations"] = 1
        self.assertFalse(CAPTURE.summarize(self.records)["acceptance_sample_eligible"])

    def test_duplicate_mixed_run_or_wrong_frame_tick_is_rejected(self):
        tick = next(row for row in self.records if row["type"] == "tick")
        self.records.append(copy.deepcopy(tick))
        with self.assertRaisesRegex(ValueError, "Duplicate tick"):
            CAPTURE.summarize(self.records)
        self.records.pop()
        tick["run"] = 2
        with self.assertRaisesRegex(ValueError, "exact run/frame"):
            CAPTURE.summarize(self.records)
        tick["run"] = 1
        tick["frame"] = 2
        with self.assertRaisesRegex(ValueError, "exact run/frame"):
            CAPTURE.summarize(self.records)

    def test_tick_command_and_pump_accounting_cannot_be_forged(self):
        tick = next(row for row in self.records if row["type"] == "tick")
        tick["applied_commands"] = 0
        with self.assertRaisesRegex(ValueError, "command count"):
            CAPTURE.summarize(self.records)
        tick["applied_commands"] = 1
        tick["boundary_ns"] = 7_000_000
        with self.assertRaisesRegex(ValueError, "whole pump"):
            CAPTURE.summarize(self.records)

    def test_bounded_prefix_cursor_cannot_exceed_received_storage(self):
        self.records[0]["initial_observation_cursor"] = 1921
        with self.assertRaisesRegex(ValueError, "bounded tick observation"):
            CAPTURE.summarize(self.records)

    def test_stage_attribution_is_never_budget_acceptance(self):
        self.records[0]["profile_stages"] = True
        for row in self.records:
            if row["type"] == "tick":
                row["simulation"] = dict(tick=row["tick"], gather_ns=1, index_ns=2, propose_ns=3,
                                         commit_ns=4, resources_ns=5, rebuild_ns=6, input_rows=100000,
                                         query_rows=100000, occupied_cells=50000, query_scratch_capacity=100000,
                                         workers=1, partitions=1, task_capacity=1)
        result = CAPTURE.summarize(self.records)
        self.assertTrue(result["complete_tick_coverage"])
        self.assertFalse(result["acceptance_sample_eligible"])
        self.assertIsNone(result["tick_boundary_p95_ns"])
        self.assertFalse(result["g3_target_met"])
        self.assertFalse(result["g4_target_met"])
        next(row for row in self.records if row["type"] == "tick")["simulation"]["tick"] += 1
        with self.assertRaisesRegex(ValueError, "phase attribution"):
            CAPTURE.summarize(self.records)

    def test_both_scales_receive_detail_pan_and_fitted_restoration(self):
        for population, columns, rows in ((100000, 400, 250), (150000, 500, 300)):
            self.records[0].update(population=population, columns=columns, rows=rows)
            self.records[0].update(partition_query_scratch_capacity=population, partition_query_scratch_bytes=population * 8)
            for frame in self.records:
                if frame["type"] == "frame":
                    frame["authoritative_mobile"] = population
            camera_fixture(self.records)
            result = CAPTURE.summarize(self.records)
            self.assertTrue(result["acceptance_sample_eligible"])
            self.assertEqual(result["camera_detail_frames"], 900)
            self.assertEqual(result["camera_pan_frames"], 600)
            self.assertEqual(result["camera_restore_frames"], 300)

    def test_camera_events_cannot_substitute_for_actual_zoom_or_pan(self):
        frames = [row for row in self.records if row["type"] == "frame"]
        frames[1]["camera_scale"] = frames[1]["camera_scale_before"]
        with self.assertRaisesRegex(ValueError, "actual transition"):
            CAPTURE.summarize(self.records)
        camera_fixture(self.records)
        frames[2]["camera_center_x"] = frames[2]["camera_center_x_before"]
        frames[2]["camera_center_y"] = frames[2]["camera_center_y_before"]
        with self.assertRaisesRegex(ValueError, "actual transition"):
            CAPTURE.summarize(self.records)

    def test_detail_geometry_and_operator_cohorts_are_received_separately(self):
        frames = [row for row in self.records if row["type"] == "frame"]
        frames[1].update(individual=0, aggregated=50000, marks=100)
        with self.assertRaisesRegex(ValueError, "actual individual geometry"):
            CAPTURE.summarize(self.records)
        camera_fixture(self.records)
        frames[1]["operator_camera_events"] = 1
        result = CAPTURE.summarize(self.records)
        self.assertFalse(result["acceptance_sample_eligible"])
        self.assertEqual(result["operator_camera_events"], 1)

    def test_mutation_must_be_admitted_from_its_fitted_event_frame(self):
        event = next(row for row in self.records if row["type"] == "input")
        event["event_frame"] = 2
        with self.assertRaisesRegex(ValueError, "fitted overview event frame"):
            CAPTURE.summarize(self.records)

    def test_fitted_restoration_cannot_be_assumed_from_phase_or_events(self):
        restored = [row for row in self.records if row["type"] == "frame"][5]
        restored["camera_center_x"] += 2
        with self.assertRaisesRegex(ValueError, "actual transition"):
            CAPTURE.summarize(self.records)

    def test_none_camera_cohort_is_explicitly_fixed_overview(self):
        self.records = [row for row in self.records if row["type"] not in ("input", "trace")]
        self.records[0]["route"] = 0
        for tick in self.records:
            if tick["type"] == "tick":
                tick["applied_commands"] = 0
        camera_fixture(self.records)
        result = CAPTURE.summarize(self.records)
        self.assertTrue(result["acceptance_sample_eligible"])
        self.assertEqual(result["camera_detail_frames"], 0)
        self.records[0]["camera_mode"] = "six-frame-cycle"
        with self.assertRaisesRegex(ValueError, "route cohort"):
            CAPTURE.summarize(self.records)

    def test_copied_worker_resolution_and_storage_are_validated(self):
        header = self.records[0]
        header.update(requested_workers=4, requested_partitions=0, resolved_workers=4, resolved_partitions=8,
                      partition_query_scratch_capacity=800000, partition_query_scratch_bytes=6400000,
                      row_task_capacity=8, row_queue_capacity=8)
        result = CAPTURE.summarize(self.records)
        self.assertEqual(result["resolved_partitions"], 8)
        self.assertEqual(result["partition_query_scratch_bytes"], 6400000)
        header["resolved_partitions"] = 7
        with self.assertRaisesRegex(ValueError, "partition resolution"):
            CAPTURE.summarize(self.records)
        header["resolved_partitions"] = 8
        header["row_queue_capacity"] = 7
        with self.assertRaisesRegex(ValueError, "Actual row adapter storage"):
            CAPTURE.summarize(self.records)

    def test_stage_execution_bounds_must_match_copied_startup_storage(self):
        self.records[0]["profile_stages"] = True
        for row in self.records:
            if row["type"] == "tick":
                row["simulation"] = dict(tick=row["tick"], gather_ns=1, index_ns=2, propose_ns=3,
                                         commit_ns=4, resources_ns=5, rebuild_ns=6, input_rows=100000,
                                         query_rows=100000, occupied_cells=50000, query_scratch_capacity=100000,
                                         workers=1, partitions=1, task_capacity=1)
        self.assertFalse(CAPTURE.summarize(self.records)["acceptance_sample_eligible"])
        next(row for row in self.records if row["type"] == "tick")["simulation"]["task_capacity"] = 2
        with self.assertRaisesRegex(ValueError, "execution bounds differ"):
            CAPTURE.summarize(self.records)

    def test_parallel_fallback_is_reported_without_performance_acceptance(self):
        header = self.records[0]
        header.update(requested_workers=4, resolved_workers=4, resolved_partitions=8,
                      partition_query_scratch_capacity=800000, partition_query_scratch_bytes=6400000,
                      row_task_capacity=8, row_queue_capacity=8, row_fallbacks_start=2, row_fallbacks_end=2)
        self.assertTrue(CAPTURE.summarize(self.records)["acceptance_sample_eligible"])
        header["row_fallbacks_end"] = 3
        result = CAPTURE.summarize(self.records)
        self.assertEqual(result["row_fallbacks_delta"], 1)
        self.assertFalse(result["parallel_rows_without_fallback"])
        self.assertFalse(result["acceptance_sample_eligible"])
        self.assertIsNone(result["tick_boundary_p95_ns"])
        self.assertIsNone(result["fullframe_p95_ns"])
        self.assertFalse(result["g3_target_met"])
        header["row_fallbacks_end"] = 1
        with self.assertRaisesRegex(ValueError, "moved backwards"):
            CAPTURE.summarize(self.records)

    def test_forged_inline_and_missing_fallback_receipts_are_rejected(self):
        self.records[0]["row_fallbacks_end"] = 1
        with self.assertRaisesRegex(ValueError, "cannot report worker FP fallback"):
            CAPTURE.summarize(self.records)
        header = self.records[0]
        header.update(row_execution_backend="pr29-sequential-control", partition_query_scratch_capacity=0,
                      partition_query_scratch_bytes=0, row_task_capacity=0)
        with self.assertRaisesRegex(ValueError, "cannot report worker FP fallback"):
            CAPTURE.summarize(self.records)
        del header["row_fallbacks_end"]
        with self.assertRaisesRegex(ValueError, "Invalid row execution count"):
            CAPTURE.summarize(self.records)

    def test_forged_scratch_bytes_and_inline_queue_are_rejected(self):
        self.records[0]["partition_query_scratch_bytes"] += 8
        with self.assertRaisesRegex(ValueError, "scratch byte accounting"):
            CAPTURE.summarize(self.records)
        self.records[0]["partition_query_scratch_bytes"] -= 8
        self.records[0]["row_queue_capacity"] = 1
        with self.assertRaisesRegex(ValueError, "Actual row adapter storage"):
            CAPTURE.summarize(self.records)

    def test_legacy_control_cannot_receive_unconsumed_axes(self):
        header = self.records[0]
        header.update(row_execution_backend="pr29-sequential-control", partition_query_scratch_capacity=0,
                      partition_query_scratch_bytes=0, row_task_capacity=0)
        self.assertEqual(CAPTURE.summarize(self.records)["row_execution_backend"], "pr29-sequential-control")
        header.update(requested_partitions=2, resolved_partitions=2)
        with self.assertRaisesRegex(ValueError, "Sequential control"):
            CAPTURE.summarize(self.records)
        header.update(requested_partitions=1, resolved_partitions=1, profile_stages=True)
        with self.assertRaisesRegex(ValueError, "stage attribution"):
            CAPTURE.summarize(self.records)

    def test_same_source_worker_pairs_require_one_frozen_executable(self):
        baseline = dict(source="a" * 40, source_tree="b" * 40, binary="same.exe", binary_sha256="c" * 64,
                        requested_workers=1, requested_partitions=0)
        current = dict(baseline, requested_workers=4)
        arms = dict(baseline=baseline, current=current)
        CAPTURE.validate_comparison("same-source-workers", arms)
        current["binary_sha256"] = "d" * 64
        with self.assertRaisesRegex(ValueError, "identical frozen executable"):
            CAPTURE.validate_comparison("same-source-workers", arms)
        current["binary_sha256"] = baseline["binary_sha256"]
        current["requested_workers"] = 1
        with self.assertRaisesRegex(ValueError, "one worker versus multiple"):
            CAPTURE.validate_comparison("same-source-workers", arms)
        baseline["requested_partitions"] = 2
        with self.assertRaisesRegex(ValueError, "sequential control rejects"):
            CAPTURE.validate_comparison("pr29-control", arms)

    def test_memory_samples_keep_process_lifetime_and_configured_bounds_distinct(self):
        result = CAPTURE.summarize(self.records)
        self.assertTrue(result["process_memory_received"])
        self.assertEqual(result["memory_end"]["peak_working_set_bytes"], 2500000)
        self.assertEqual(result["partition_query_scratch_bytes"], 800000)
        self.records[0]["memory_peak_scope"] = "frame-allocation-peak"
        with self.assertRaisesRegex(ValueError, "observation scope"):
            CAPTURE.summarize(self.records)

    def test_missing_memory_blocks_acceptance_and_inconsistent_peak_is_rejected(self):
        self.records[0]["memory_end"].update(received=False, error=5, working_set_bytes=0,
                                             peak_working_set_bytes=0, private_committed_bytes=0)
        result = CAPTURE.summarize(self.records)
        self.assertFalse(result["process_memory_received"])
        self.assertFalse(result["acceptance_sample_eligible"])
        self.assertIsNone(result["tick_boundary_p95_ns"])
        self.records[0]["memory_end"].update(received=True, error=0, working_set_bytes=1000000,
                                             peak_working_set_bytes=1200000, private_committed_bytes=1900000)
        with self.assertRaisesRegex(ValueError, "peak moved backwards"):
            CAPTURE.summarize(self.records)

    def test_sample_timeout_retains_cold_process_headroom_without_changing_minima(self):
        CAPTURE.validate_timeouts(1200, 3600, False)
        CAPTURE.validate_timeouts(1200, 31, True)
        for sample, process, probe in ((1200, 1200, False), (29, 3600, False), (3501, 3600, False),
                                       (1200, 3601, False), (1200, 30, True)):
            with self.assertRaises(ValueError):
                CAPTURE.validate_timeouts(sample, process, probe)
        self.records[0]["duration_ns"] = 29_999_999_999
        self.assertFalse(CAPTURE.summarize(self.records)["acceptance_sample_eligible"])


if __name__ == "__main__":
    unittest.main()
