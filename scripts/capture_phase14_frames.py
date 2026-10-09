#!/usr/bin/env python3
"""Capture or classify production Phase14 native frame JSONL; timing remains advisory."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import subprocess
import time


COMPILED_PATHS = ["include", "src", "cmake", "CMakeLists.txt",
                  "benchmarks/CMakeLists.txt", "benchmarks/phase14_frame.cpp", "benchmarks/phase14_frame.cmake",
                  "scripts/capture_phase14_frames.py"]


def nearest_rank(values, percentile):
    """Nearest-rank quantile, with no invented value for an empty cohort."""
    if not values:
        return None
    return sorted(values)[max(0, math.ceil(len(values) * percentile) - 1)]


def execution_receipt(capture):
    """Validate copied Runtime policy and actual adapter bounds; never resolve settings for the app."""
    if capture.get("execution_schema") != 1:
        raise ValueError("Missing or unsupported row execution receipt")
    names = ("requested_workers", "requested_partitions", "resolved_workers", "resolved_partitions",
             "partition_query_scratch_capacity", "query_scratch_element_bytes", "partition_query_scratch_bytes",
             "row_task_capacity", "row_queue_capacity", "row_fallbacks_start", "row_fallbacks_end")
    if any(not isinstance(capture.get(name), int) or isinstance(capture[name], bool) or capture[name] < 0 for name in names):
        raise ValueError("Invalid row execution count or storage receipt")
    workers, partitions = capture["resolved_workers"], capture["resolved_partitions"]
    requested = capture["requested_partitions"]
    if (not 1 <= capture["requested_workers"] <= 32 or not 0 <= requested <= 128 or
            workers != capture["requested_workers"] or not 1 <= partitions <= min(128, capture["population"])):
        raise ValueError("Row execution settings exceed frozen Runtime policy")
    expected = requested if requested else 1 if workers == 1 else 2 * workers
    if partitions != expected:
        raise ValueError("Copied Runtime partition resolution differs from requested policy")
    capacity = capture["partition_query_scratch_capacity"]
    if capture["query_scratch_element_bytes"] != 8 or capture["partition_query_scratch_bytes"] != capacity * 8:
        raise ValueError("Partition query scratch byte accounting is inconsistent")
    backend = capture["row_execution_backend"]
    if backend == "pr29-sequential-control":
        if (workers != 1 or partitions != 1 or capacity or capture["row_task_capacity"] or capture["row_queue_capacity"]):
            raise ValueError("Sequential control reports unsupported axes or absent row storage")
        if capture["profile_stages"]:
            raise ValueError("Sequential control cannot receive simulation stage attribution")
    elif backend == "row-partitions":
        if (capacity != capture["population"] * partitions or capture["row_task_capacity"] != partitions or
                capture["row_queue_capacity"] != (partitions if workers > 1 else 0)):
            raise ValueError("Actual row adapter storage differs from resolved startup bounds")
    else:
        raise ValueError("Unknown row execution backend")
    start, end = capture["row_fallbacks_start"], capture["row_fallbacks_end"]
    if end < start or end > 2 ** 64 - 1:
        raise ValueError("Completed row fallback count moved backwards or exceeds uint64")
    if workers == 1 and (start or end):
        raise ValueError("One-worker or sequential control cannot report worker FP fallback")
    return {name: capture[name] for name in names + ("execution_schema", "row_execution_backend")} | {
        "row_fallbacks_delta": end - start, "parallel_rows_without_fallback": end == start}


def validate_comparison(comparison, arms):
    """Keep a preserved legacy control comparison distinct from one executable's worker arms."""
    if comparison == "same-source-workers":
        baseline, current = arms["baseline"], arms["current"]
        if any(baseline[name] != current[name] for name in ("source", "source_tree", "binary", "binary_sha256")):
            raise ValueError("Same-source worker comparison requires the identical frozen executable/source/tree")
        if baseline["requested_workers"] != 1 or current["requested_workers"] <= 1:
            raise ValueError("Same-source worker comparison requires one worker versus multiple workers")
    elif comparison == "pr29-control":
        if arms["baseline"]["requested_workers"] != 1 or arms["baseline"]["requested_partitions"] not in (0, 1):
            raise ValueError("Preserved sequential control rejects worker or partition axes")
    else:
        raise ValueError("Unknown comparison cohort")


def memory_receipt(capture):
    """Receive two cold process samples; process-lifetime peak is distinct from frame allocation."""
    if (capture.get("memory_schema") != 1 or capture["memory_api"] != "K32GetProcessMemoryInfo" or
            capture["memory_peak_scope"] != "process-lifetime-through-sample" or
            capture["memory_sample_scope"] != "after-native-setup-and-after-measured-drain-before-cold-quality"):
        raise ValueError("Unsupported process-memory observation scope")
    names = ("error", "working_set_bytes", "peak_working_set_bytes", "private_committed_bytes", "sample_call_ns")
    for sample in (capture["memory_startup"], capture["memory_end"]):
        if sample["received"] not in (False, True) or any(not isinstance(sample[name], int) or isinstance(sample[name], bool) or sample[name] < 0 for name in names):
            raise ValueError("Invalid process-memory counter")
        if sample["received"]:
            if (sample["error"] or sample["working_set_bytes"] <= 0 or sample["private_committed_bytes"] <= 0 or
                    sample["peak_working_set_bytes"] < sample["working_set_bytes"]):
                raise ValueError("Successful process-memory receipt has inconsistent counters")
        elif any(sample[name] for name in ("working_set_bytes", "peak_working_set_bytes", "private_committed_bytes")):
            raise ValueError("Failed process-memory sample cannot establish memory counters")
    received = bool(capture["memory_startup"]["received"] and capture["memory_end"]["received"])
    if received and capture["memory_end"]["peak_working_set_bytes"] < capture["memory_startup"]["peak_working_set_bytes"]:
        raise ValueError("Process-lifetime memory peak moved backwards")
    return {name: capture[name] for name in ("memory_schema", "memory_api", "memory_peak_scope", "memory_sample_scope", "memory_startup", "memory_end")} | {"process_memory_received": received}


def validate_timeouts(sample_timeout, process_timeout, probe):
    """Keep sampling finite while reserving a larger process budget for cold reference/capture."""
    if not 30 <= sample_timeout <= 3500 or not 30 <= process_timeout <= 3600:
        raise ValueError("Invalid sample or external process timeout")
    if (30 if probe else sample_timeout) >= process_timeout:
        raise ValueError("Internal sample timeout must leave external process headroom for cold work")


def summarize(records):
    """Validate identity/accounting first; short probes never receive percentile gates."""
    captures = [row for row in records if row.get("type") == "capture"]
    qualities = [row for row in records if row.get("type") == "quality"]
    if len(captures) != 1 or len(qualities) != 1:
        raise ValueError("Expected exactly one capture and quality receipt")
    capture, quality = captures[0], qualities[0]
    if capture["schema"] != 3 or capture["completion_mode"] != "joined-yield-zero" or capture["pacing_mode"] != "fixed-60hz-skip-whole-overdue-periods":
        raise ValueError("Unsupported schema or observation arm")
    execution = execution_receipt(capture)
    memory = memory_receipt(capture)
    if not isinstance(capture["internal_timeout_seconds"], int) or not 1 <= capture["internal_timeout_seconds"] <= 3600:
        raise ValueError("Invalid captured internal timeout budget")
    frames = [row for row in records if row.get("type") == "frame"]
    inputs = [row for row in records if row.get("type") == "input"]
    traces = [row for row in records if row.get("type") == "trace"]
    ticks = [row for row in records if row.get("type") == "tick"]
    identities = {}
    expected_ticks = {}
    previous_tick = capture["initial_tick"]
    previous_end = 0
    if (capture["camera_script"] != "overview-detail-pan-v1" or
            capture["camera_mode"] != ("fixed-overview" if capture["route"] == 0 else "six-frame-cycle")):
        raise ValueError("Unsupported camera script or route cohort")
    width, height = capture["columns"] * capture["cell_size"], capture["rows"] * capture["cell_size"]
    fit_scale = min(capture["camera_viewport_width"] / width, capture["camera_viewport_height"] / height)
    detail_scale = fit_scale * 1.2 ** 5
    if not math.isfinite(fit_scale) or fit_scale <= 0 or detail_scale < 4:
        raise ValueError("Camera geometry cannot receive the frozen detail workload")
    def same_camera(first, second):
        return (math.isclose(first[0], second[0], rel_tol=1e-9, abs_tol=1e-8) and
                all(abs(a - b) <= 1e-4 for a, b in zip(first[1:], second[1:])))
    fitted_camera = fit_scale, width / 2, height / 2
    expected_camera = fitted_camera
    camera_pan_frames = camera_detail_frames = camera_restore_frames = operator_camera_events = 0
    for index, frame in enumerate(frames):
        identity = frame["run"], frame["frame"]
        if identity in identities or min(identity) <= 0:
            raise ValueError("Duplicate or nonpositive frame identity")
        if frames and identities:
            previous = next(reversed(identities))
            if identity[0] != previous[0] or identity[1] != previous[1] + 1:
                raise ValueError("Mixed runs or noncontiguous frame identities")
        identities[identity] = frame
        counters = [frame[name] for name in ("advanced", "authoritative_mobile", "individual", "aggregated", "hidden", "marks",
                                             "begin_ns", "end_ns", "handoff_observed_ns", "marker_begin_ns", "marker_end_ns",
                                             "completion_observed_ns", "poll_call_ns", "poll_calls", "pacing_skipped", "pump_ns")]
        if any(value < 0 for value in counters):
            raise ValueError("Negative count or clock observation")
        if frame["tick"] - previous_tick != frame["advanced"]:
            raise ValueError("Tick delta differs from advanced tick count")
        for tick in range(previous_tick + 1, frame["tick"] + 1):
            expected_ticks[identity[0], tick] = frame
        if frame["begin_ns"] < previous_end:
            raise ValueError("Frame timeline moves backwards or overlaps in joined arm")
        previous_tick, previous_end = frame["tick"], frame["end_ns"]
        if frame["authoritative_mobile"] != frame["individual"] + frame["aggregated"] + frame["hidden"]:
            raise ValueError("Population count accounting is not closed")
        if frame["marks"] > frame["aggregated"] or frame["begin_ns"] > frame["end_ns"]:
            raise ValueError("Invalid geometry count or frame interval")
        completion = frame["completion_observed_ns"]
        if completion and not frame["begin_ns"] <= frame["handoff_observed_ns"] <= frame["marker_begin_ns"] <= frame["marker_end_ns"] <= completion <= frame["end_ns"]:
            raise ValueError("Completion is outside the ordered joined frame timeline")
        if completion and (frame["poll_calls"] <= 0 or frame["draw_tick"] != frame["tick"]):
            raise ValueError("Completed frame has no poll or mismatched drawn tick")
        camera_before = tuple(frame[name] for name in ("camera_scale_before", "camera_center_x_before", "camera_center_y_before"))
        camera_after = tuple(frame[name] for name in ("camera_scale", "camera_center_x", "camera_center_y"))
        if (not all(math.isfinite(value) for value in camera_before + camera_after) or
                min(camera_before[0], camera_after[0]) <= 0 or
                any(frame[name] < 0 for name in ("camera_events", "camera_transitions", "operator_camera_events"))):
            raise ValueError("Invalid camera observation")
        operator_camera_events += frame["operator_camera_events"]
        if not same_camera(camera_before, expected_camera):
            raise ValueError("Camera before-state differs from frozen measured trajectory")
        phase = -1 if capture["route"] == 0 else index % 6
        if frame["camera_phase"] != phase:
            raise ValueError("Camera phase differs from measured frame ordinal")
        if phase in (-1, 0, 4, 5):
            expected_camera = fitted_camera
        elif phase in (1, 3):
            expected_camera = detail_scale, width / 2, height / 2
        else:
            expected_camera = detail_scale, width / 2 - 24 / detail_scale, height / 2 - 12 / detail_scale
        expected_events = 0 if phase == -1 else 3 if phase in (2, 3) else 1
        if (not same_camera(camera_after, expected_camera) or frame["camera_events"] != expected_events or
                frame["camera_transitions"] != int(not same_camera(camera_before, camera_after))):
            raise ValueError("Camera actual transition does not receive the frozen production script")
        if phase in (1, 2, 3):
            if camera_after[0] < 4 or frame["individual"] <= 0 or frame["aggregated"]:
                raise ValueError("Camera detail frame did not submit actual individual geometry")
            camera_detail_frames += 1
        if phase in (2, 3):
            camera_pan_frames += 1
        if phase == 5:
            camera_restore_frames += 1
    applied = {row["sequence"]: row for row in traces}
    if len(applied) != len(traces):
        raise ValueError("Duplicate command sequence")
    if any(row["sequence"] != index + 1 or not capture["initial_tick"] < row["tick"] <= previous_tick
           for index, row in enumerate(traces)):
        raise ValueError("Trace sequence or tick is outside the frozen measured run")
    observation_counts = [capture[name] for name in ("tick_observation_capacity", "tick_observation_count",
                                                    "initial_observation_cursor", "dropped_tick_observations")]
    if (any(value < 0 for value in observation_counts) or not capture["tick_observation_capacity"] or
            not capture["initial_observation_cursor"] <= capture["tick_observation_count"] <= capture["tick_observation_capacity"]):
        raise ValueError("Invalid bounded tick observation accounting")
    received_ticks = {}
    frame_boundaries = {}
    commands_per_tick = {}
    for command in traces:
        commands_per_tick[command["tick"]] = commands_per_tick.get(command["tick"], 0) + 1
    last_observed_tick = capture["initial_tick"]
    for row in ticks:
        identity = row["run"], row["tick"]
        if identity in received_ticks:
            raise ValueError("Duplicate tick observation identity")
        if row["tick"] <= last_observed_tick or row["boundary_ns"] < 0 or row["applied_commands"] < 0:
            raise ValueError("Nonmonotonic or negative tick observation")
        last_observed_tick = row["tick"]
        frame = expected_ticks.get(identity)
        if frame is None or frame["frame"] != row["frame"]:
            raise ValueError("Tick observation has no exact run/frame boundary correlation")
        if row["applied_commands"] != commands_per_tick.get(row["tick"], 0):
            raise ValueError("Tick command count differs from applied trace")
        stages = row["simulation"]
        if bool(stages is not None) != bool(capture["profile_stages"]):
            raise ValueError("Tick phase presence differs from declared attribution arm")
        if stages is not None:
            durations = [stages[name] for name in ("gather_ns", "index_ns", "propose_ns", "commit_ns", "resources_ns", "rebuild_ns")]
            counts = [stages[name] for name in ("input_rows", "query_rows", "occupied_cells", "query_scratch_capacity", "workers", "partitions", "task_capacity")]
            if stages["tick"] != row["tick"] or any(value < 0 for value in durations + counts) or sum(durations) > row["boundary_ns"]:
                raise ValueError("Invalid copied simulation phase attribution")
            if (stages["task_capacity"] != execution["row_task_capacity"] or
                    stages["query_scratch_capacity"] != execution["partition_query_scratch_capacity"] or
                    stages["workers"] not in (1, execution["resolved_workers"]) or
                    stages["partitions"] > execution["resolved_partitions"] or
                    stages["input_rows"] > capture["population"] or stages["query_rows"] > stages["input_rows"]):
                raise ValueError("Simulation stage execution bounds differ from copied startup storage")
        received_ticks[identity] = row
        frame_identity = row["run"], row["frame"]
        frame_boundaries[frame_identity] = frame_boundaries.get(frame_identity, 0) + row["boundary_ns"]
    for identity, boundary in frame_boundaries.items():
        if boundary > identities[identity]["pump_ns"]:
            raise ValueError("Tick boundary intervals exceed correlated whole pump interval")
    tick_coverage = (set(received_ticks) == set(expected_ticks) and
                     capture["initial_observation_cursor"] == capture["initial_tick"] and
                     capture["tick_observation_count"] - capture["initial_observation_cursor"] == len(ticks) and
                     not capture["dropped_tick_observations"])
    accepted_inputs = []
    accepted_sequences = set()
    for row in inputs:
        if not row["accepted"]:
            continue
        if row["sequence"] in accepted_sequences:
            raise ValueError("Duplicate accepted input sequence")
        accepted_sequences.add(row["sequence"])
        event_frame = identities.get((row["run"], row["event_frame"]))
        if (event_frame is None or event_frame["camera_phase"] != 0 or
                not event_frame["begin_ns"] <= row["event_begin_ns"] <= row["admitted_ns"] <= event_frame["end_ns"]):
            raise ValueError("Field admission did not occur in its fitted overview event frame")
        if row["sequence"] not in applied:
            continue  # Accepted but uncompleted remains a visible failing cohort.
        command = applied[row["sequence"]]
        if command["tick"] != row["tick"]:
            raise ValueError("Input/application tick differs")
        application = identities.get((row["run"], row["application_frame"]))
        if application is None or application["tick"] < row["tick"] or not application["begin_ns"] <= row["applied_observed_ns"] <= application["end_ns"]:
            raise ValueError("Input application has no correlated frame interval")
        completed = row["completed_observed_ns"]
        if completed:
            frame = identities.get((row["run"], row["frame"]))
            if frame is None or frame["tick"] < row["tick"] or frame["native_status"] != 0 or frame["marker_status"] != 0:
                raise ValueError("Input completion has no checked frame")
            if completed != frame["completion_observed_ns"]:
                raise ValueError("Input completion differs from correlated frame receipt")
            if not row["event_begin_ns"] <= row["admitted_ns"] <= row["applied_observed_ns"] <= completed:
                raise ValueError("Input timeline is out of order")
            if row["visible"] and row["status"] == 0:
                accepted_inputs.append(completed - row["event_begin_ns"])
    handoffs = [row for row in frames if row["visible"] and row["status"] == 0 and
              row["native_status"] == 0 and row["marker_status"] == 0 and row["completion_observed_ns"] > 0]
    cohort = [row for row in handoffs if row["advanced"] > 0]
    duration = capture["duration_ns"]
    advanced = sum(row["advanced"] for row in frames)
    minimum = len(cohort) >= 1800 and duration >= 30_000_000_000 and capture["warmup_ticks"] >= 120
    stable_output = bool(frames and all((row["width"], row["height"]) == (frames[0]["width"], frames[0]["height"])
                                       and row["width"] > 0 and row["height"] > 0 for row in frames))
    eligible = (execution["parallel_rows_without_fallback"] and memory["process_memory_received"] and not operator_camera_events and tick_coverage and not capture["profile_stages"] and minimum and stable_output and len(handoffs) == len(frames) and capture["drained"] and capture["device_identity_received"] and capture["driver_version"] is not None and
                not capture["failure"] and not capture["bounded_out"] and
                not capture["dropped_frame_rows"] and not capture["dropped_input_rows"] and
                (capture["route"] == 0 or len(accepted_inputs) >= 300))
    result = {
        "schema": 3, "source": capture["source"], "source_tree": capture["source_tree"],
        "internal_timeout_seconds": capture["internal_timeout_seconds"],
        "population": capture["population"], "route": capture["route"],
        "acceptance_sample_eligible": bool(eligible), "frame_count": len(frames),
        "running_visible_completed_handoff_frames": len(handoffs), "evolving_completed_handoff_frames": len(cohort),
        "accepted_running_visible_completed_inputs": len(accepted_inputs),
        "active_ticks": advanced, "zero_tick_frames": sum(row["advanced"] == 0 for row in frames),
        "multiple_tick_frames": sum(row["advanced"] > 1 for row in frames),
        "tick_observation_count": len(ticks), "expected_measured_ticks": len(expected_ticks),
        "complete_tick_coverage": tick_coverage, "dropped_tick_observations": capture["dropped_tick_observations"],
        "profile_stages": bool(capture["profile_stages"]),
        "camera_script": capture["camera_script"], "camera_mode": capture["camera_mode"],
        "camera_detail_frames": camera_detail_frames, "camera_pan_frames": camera_pan_frames,
        "camera_restore_frames": camera_restore_frames, "operator_camera_events": operator_camera_events,
        "tick_boundary_p95_ns": None, "tick_boundary_p99_ns": None, "tick_boundary_max_ns": None,
        "pacing_skipped_periods": sum(row["pacing_skipped"] for row in frames),
        "stable_output_dimensions": stable_output,
        "no_handoff_frames": sum(row["native_status"] != 0 for row in frames),
        "marker_full_frames": sum(row["marker_status"] == 1 for row in frames),
        "paused_or_hidden_frames": sum(row["status"] != 0 or not row["visible"] for row in frames),
        "uncompleted_or_refused_inputs": len(inputs) - len(accepted_inputs),
        "tick_rate": advanced * 1e9 / duration if duration else None,
        "presentations_per_second": len(handoffs) * 1e9 / duration if duration else None,
        "discarded_scaled_ns_delta": frames[-1]["discarded_scaled_ns"] - capture.get("initial_discarded_scaled_ns", 0) if frames else 0,
        "quality_reference_cpu_ns": quality["cpu_ns"], "quality_capture_cpu_ns": quality["capture_cpu_ns"],
        "quality_influence_fraction": quality["initial_nonzero_field_samples"] / capture["population"],
        "quality_signed_mean_dx": quality["signed_mean_dx"],
        "quality_mean_front_fraction": quality["signed_mean_dx"] / capture["columns"],
        "quality_pass": bool(capture["route"] == 0 or (quality["compared"] and
                             quality["initial_nonzero_field_samples"] >= 0.1 * capture["population"] and
                             quality["signed_mean_dx"] >= 0.05 * capture["columns"])),
        "fullframe_p95_ns": None, "fullframe_p99_ns": None, "fullframe_max_ns": None,
        "fullframe_over_20ms_frames": None, "cadence_p95_ns": None,
        "cadence_p99_ns": None, "input_p99_ns": None, "g3_target_met": False, "g4_target_met": False,
        "scope": "Joined completion observation upper bounds; no scanout, participant or five-pair closure claim.",
    }
    result.update(execution)
    result.update(memory)
    if eligible:
        boundaries = [row["boundary_ns"] for row in ticks]
        result.update(tick_boundary_p95_ns=nearest_rank(boundaries, .95),
                      tick_boundary_p99_ns=nearest_rank(boundaries, .99), tick_boundary_max_ns=max(boundaries))
        services = [max(row["end_ns"], row["completion_observed_ns"]) - row["begin_ns"] for row in cohort]
        cadence = [b["handoff_observed_ns"] - a["handoff_observed_ns"] for a, b in zip(handoffs, handoffs[1:])]
        result.update(fullframe_p95_ns=nearest_rank(services, .95), fullframe_p99_ns=nearest_rank(services, .99),
                      fullframe_max_ns=max(services), fullframe_over_20ms_frames=sum(value > 20_000_000 for value in services),
                      cadence_p95_ns=nearest_rank(cadence, .95), cadence_p99_ns=nearest_rank(cadence, .99),
                      input_p99_ns=nearest_rank(accepted_inputs, .99))
        result["g3_target_met"] = result["fullframe_p95_ns"] <= 16_670_000 and result["fullframe_p99_ns"] <= 20_000_000
        repeated = sum(b["tick"] == a["tick"] for a, b in zip(handoffs, handoffs[1:])) / len(handoffs)
        result["repeated_tick_fraction"] = repeated
        # Conservative repeat/discard gates; no claim that every repeat is attributable to missed work.
        result["g4_target_met"] = bool(result["cadence_p95_ns"] <= 17_500_000 and
            result["cadence_p99_ns"] <= 33_340_000 and result["presentations_per_second"] >= 59 and
            59 <= result["tick_rate"] <= 61 and repeated <= .01 and not result["discarded_scaled_ns_delta"] and
            (capture["route"] == 0 or result["input_p99_ns"] <= 50_000_000))
    return result


def read_capture(path):
    with path.open(encoding="utf-8") as stream:
        return [json.loads(line) for line in stream if line.strip()]


def command_text(command, cwd=None):
    return subprocess.run(command, cwd=cwd, text=True, capture_output=True, check=True).stdout.strip()


def provenance(binary, build, source):
    """Bind actual executable bytes, configured source hash, source tree, pins and Release cache."""
    binary, build, source = binary.resolve(strict=True), build.resolve(strict=True), source.resolve(strict=True)
    if not binary.is_file() or not binary.is_relative_to(build):
        raise ValueError("Capture binary must be inside the declared build tree")
    cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
    values = {line.split("=", 1)[0]: line.split("=", 1)[1] for line in cache.splitlines()
              if "=" in line and not line.startswith(("#", "//"))}
    configurations = values.get("CMAKE_CONFIGURATION_TYPES:STRING", "")
    if configurations:
        if "Release" not in binary.relative_to(build).parts[:-1]:
            raise ValueError("Multi-config capture executable must be from Release")
    elif values.get("CMAKE_BUILD_TYPE:STRING") != "Release":
        raise ValueError("Native timing capture requires Release")
    if Path(values["CMAKE_HOME_DIRECTORY:INTERNAL"]).resolve() != source:
        raise ValueError("Configured source directory differs from supplied source")
    sha = command_text(["git", "rev-parse", "HEAD"], source)
    tree = command_text(["git", "rev-parse", "HEAD^{tree}"], source)
    status = command_text(["git", "status", "--porcelain", "--untracked-files=all", "--", *COMPILED_PATHS], source)
    if status:
        raise ValueError("Modified or uncommitted compiled sources cannot receive an exact-commit timing claim")
    return {"source": sha, "source_tree": tree, "compiled_source_status": status,
            "binary": str(binary), "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
            "build": str(build), "source_directory": str(source), "cmake_cache": cache,
            "dependency_pins": (source / "cmake/DependencyPins.cmake").read_text(encoding="utf-8")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--summarize", type=Path)
    for arm in ("baseline", "current"):
        parser.add_argument(f"--{arm}", type=Path)
        parser.add_argument(f"--{arm}-build", type=Path)
        parser.add_argument(f"--{arm}-source", type=Path)
        parser.add_argument(f"--{arm}-workers", type=int, default=1)
        parser.add_argument(f"--{arm}-partitions", type=int, default=0)
    parser.add_argument("--comparison", choices=["pr29-control", "same-source-workers"], default="pr29-control")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--pairs", type=int, default=5)
    parser.add_argument("--populations", nargs="+", type=int, default=[100000, 150000])
    parser.add_argument("--routes", nargs="+", choices=["none", "flow", "gather"], default=["none", "flow", "gather"])
    parser.add_argument("--probe", action="store_true", help="short receiving only; never percentile acceptance")
    parser.add_argument("--profile-stages", action="store_true", help="current-only stage attribution; never acceptance or baseline comparison")
    parser.add_argument("--uncontended", action="store_true")
    parser.add_argument("--background-power-thermal", help="record observed background load, power mode and thermal condition")
    parser.add_argument("--cooldown-seconds", type=float, default=10)
    parser.add_argument("--sample-timeout", type=int, default=1200)
    parser.add_argument("--process-timeout", type=int, default=3600)
    parser.add_argument("--quality-images", action="store_true", help="cold retained-frame pre-present BMP captures after measurement")
    args = parser.parse_args()
    if args.summarize:
        print(json.dumps(summarize(read_capture(args.summarize)), indent=2))
        return 0
    arms = ("current",) if args.profile_stages else ("baseline", "current")
    required = ["output"] + [arm + suffix for arm in arms for suffix in ("", "_build", "_source")]
    if any(getattr(args, name) is None for name in required):
        parser.error("capture requires each selected arm's binary/build/source and --output")
    if args.profile_stages and any(getattr(args, name) is not None for name in ("baseline", "baseline_build", "baseline_source")):
        parser.error("stage attribution is current-only; omit baseline arguments")
    if args.profile_stages and (args.baseline_workers != 1 or args.baseline_partitions != 0 or args.comparison != "pr29-control"):
        parser.error("stage attribution cannot consume baseline axes or a comparison cohort")
    if any(not 1 <= getattr(args, arm + "_workers") <= 32 or not 0 <= getattr(args, arm + "_partitions") <= 128 for arm in arms):
        parser.error("worker/partition axes exceed frozen Runtime startup bounds")
    if not args.uncontended or not args.background_power_thermal:
        parser.error("stop competing work and record --uncontended plus --background-power-thermal")
    if args.pairs < (1 if args.probe or args.profile_stages else 5) or args.pairs > 20 or not 0 <= args.cooldown_seconds <= 60:
        parser.error("invalid pair count or cooldown")
    try:
        validate_timeouts(args.sample_timeout, args.process_timeout, args.probe)
    except ValueError as error:
        parser.error(str(error))
    if any(value not in (100000, 150000) for value in args.populations) or len(set(args.populations)) != len(args.populations) or len(set(args.routes)) != len(args.routes):
        parser.error("invalid scale or external timeout")
    args.output.mkdir(parents=True, exist_ok=False)
    metadata = {"schema": 3, "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "platform": platform.platform(), "processor": platform.processor(), "logical_cpus": os.cpu_count(),
                "background_power_thermal": args.background_power_thermal, "uncontended_attested": args.uncontended,
                "probe": args.probe, "pairs": args.pairs, "cooldown_seconds": args.cooldown_seconds,
                "profile_stages": args.profile_stages, "study": "current-only-attribution" if args.profile_stages else "paired-boundary-only",
                "comparison": None if args.profile_stages else args.comparison,
                "process_timeout_seconds": args.process_timeout, "populations": args.populations, "routes": args.routes,
                "sample_timeout_seconds": 30 if args.probe else args.sample_timeout,
                "completion_mode": "joined-yield-zero", "camera_script": "overview-detail-pan-v1", "results": [], "arms": {}}
    metadata["wrapper_sha256"] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    metadata_path = args.output / "metadata.json"
    def save_metadata():
        metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    save_metadata()
    try:
        for arm in arms:
            metadata["arms"][arm] = provenance(getattr(args, arm), getattr(args, arm + "_build"), getattr(args, arm + "_source"))
            metadata["arms"][arm].update(requested_workers=getattr(args, arm + "_workers"),
                                         requested_partitions=getattr(args, arm + "_partitions"))
        if not args.profile_stages:
            validate_comparison(args.comparison, metadata["arms"])
        save_metadata()
        for population in args.populations:
            for route in args.routes:
                for pair in range(args.pairs):
                    order = arms if args.profile_stages or pair % 2 == 0 else tuple(reversed(arms))
                    for arm in order:
                        directory = args.output / f"{population}-{route}-pair{pair + 1}-{arm}"
                        directory.mkdir()
                        information = metadata["arms"][arm]
                        command = [information["binary"], "--source", information["source"], "--population", str(population),
                                   "--route", route, "--frames", "6" if args.probe else "1800",
                                   "--seconds", "0" if args.probe else "30", "--warmup-ticks", "0" if args.probe else "120",
                                   "--mutations", "1" if args.probe else "300", "--timeout-seconds", "30" if args.probe else str(args.sample_timeout),
                                   "--profile-stages", "true" if args.profile_stages else "false",
                                   "--workers", str(information["requested_workers"]),
                                   "--partitions", str(information["requested_partitions"])]
                        if args.quality_images:
                            command += ["--capture-dir", str(directory)]
                        receipt = {"arm": arm, "population": population, "route": route, "pair": pair + 1,
                                   "command": command, "returncode": None, "timed_out": False,
                                   "sample_timeout_seconds": 30 if args.probe else args.sample_timeout,
                                   "process_timeout_seconds": args.process_timeout}
                        began = time.monotonic()
                        with (directory / "raw.jsonl").open("wb") as output, (directory / "stderr.txt").open("wb") as errors:
                            try:
                                result = subprocess.run(command, stdout=output, stderr=errors, timeout=args.process_timeout, check=False)
                                receipt["returncode"] = result.returncode
                            except subprocess.TimeoutExpired:
                                receipt["timed_out"] = True
                        receipt["process_seconds"] = time.monotonic() - began
                        metadata["results"].append(receipt)
                        try:
                            summary = summarize(read_capture(directory / "raw.jsonl"))
                            if summary["source"] != information["source"] or summary["source_tree"] != information["source_tree"]:
                                raise ValueError("Binary source identity differs from captured checkout")
                            if any(summary[name] != information[name] for name in ("requested_workers", "requested_partitions")):
                                raise ValueError("Captured worker/partition requests differ from launched arm")
                            if summary["internal_timeout_seconds"] != receipt["sample_timeout_seconds"]:
                                raise ValueError("Captured timeout budget differs from launched arm")
                            expected_backend = "pr29-sequential-control" if not args.profile_stages and args.comparison == "pr29-control" and arm == "baseline" else "row-partitions"
                            if summary["row_execution_backend"] != expected_backend:
                                raise ValueError("Captured execution backend differs from declared comparison cohort")
                            (directory / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
                            receipt["sample_eligible"] = summary["acceptance_sample_eligible"] and not args.probe and not args.profile_stages
                            receipt["g3_target_met"] = summary["g3_target_met"]
                            receipt["g4_target_met"] = summary["g4_target_met"]
                            receipt["quality_pass"] = summary["quality_pass"]
                            receipt["execution"] = {name: summary[name] for name in ("resolved_workers", "resolved_partitions", "row_execution_backend",
                                "partition_query_scratch_capacity", "partition_query_scratch_bytes", "row_task_capacity", "row_queue_capacity",
                                "row_fallbacks_start", "row_fallbacks_end", "row_fallbacks_delta", "parallel_rows_without_fallback")}
                        except (ValueError, KeyError) as error:
                            receipt["classification_error"] = str(error)
                        save_metadata()
                        if receipt["returncode"] != 0 or receipt["timed_out"] or receipt.get("classification_error"):
                            return 1  # Preserve the exact failed process; no retry or outlier deletion.
                        time.sleep(args.cooldown_seconds)
        # Individual receipts remain separate; neither a median nor a probe hides a slower/outlier arm.
        metadata["all_samples_eligible"] = bool(not args.probe and not args.profile_stages and all(row.get("sample_eligible") for row in metadata["results"]))
        save_metadata()
        return 0 if args.probe or args.profile_stages or metadata["all_samples_eligible"] else 1
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        metadata["failure"] = str(error)
        save_metadata()
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
