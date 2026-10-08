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


def summarize(records):
    """Validate identity/accounting first; short probes never receive percentile gates."""
    captures = [row for row in records if row.get("type") == "capture"]
    qualities = [row for row in records if row.get("type") == "quality"]
    if len(captures) != 1 or len(qualities) != 1:
        raise ValueError("Expected exactly one capture and quality receipt")
    capture, quality = captures[0], qualities[0]
    if capture["schema"] != 2 or capture["completion_mode"] != "joined-yield-zero" or capture["pacing_mode"] != "fixed-60hz-skip-whole-overdue-periods":
        raise ValueError("Unsupported schema or observation arm")
    frames = [row for row in records if row.get("type") == "frame"]
    inputs = [row for row in records if row.get("type") == "input"]
    traces = [row for row in records if row.get("type") == "trace"]
    ticks = [row for row in records if row.get("type") == "tick"]
    identities = {}
    expected_ticks = {}
    previous_tick = capture["initial_tick"]
    previous_end = 0
    for frame in frames:
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
    eligible = (tick_coverage and not capture["profile_stages"] and minimum and stable_output and len(handoffs) == len(frames) and capture["drained"] and capture["device_identity_received"] and capture["driver_version"] is not None and
                not capture["failure"] and not capture["bounded_out"] and
                not capture["dropped_frame_rows"] and not capture["dropped_input_rows"] and
                (capture["route"] == 0 or len(accepted_inputs) >= 300))
    result = {
        "schema": 2, "source": capture["source"], "source_tree": capture["source_tree"],
        "population": capture["population"], "route": capture["route"],
        "acceptance_sample_eligible": bool(eligible), "frame_count": len(frames),
        "running_visible_completed_handoff_frames": len(handoffs), "evolving_completed_handoff_frames": len(cohort),
        "accepted_running_visible_completed_inputs": len(accepted_inputs),
        "active_ticks": advanced, "zero_tick_frames": sum(row["advanced"] == 0 for row in frames),
        "multiple_tick_frames": sum(row["advanced"] > 1 for row in frames),
        "tick_observation_count": len(ticks), "expected_measured_ticks": len(expected_ticks),
        "complete_tick_coverage": tick_coverage, "dropped_tick_observations": capture["dropped_tick_observations"],
        "profile_stages": bool(capture["profile_stages"]),
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
    parser.add_argument("--output", type=Path)
    parser.add_argument("--pairs", type=int, default=5)
    parser.add_argument("--populations", nargs="+", type=int, default=[100000, 150000])
    parser.add_argument("--routes", nargs="+", choices=["none", "flow", "gather"], default=["none", "flow", "gather"])
    parser.add_argument("--probe", action="store_true", help="short receiving only; never percentile acceptance")
    parser.add_argument("--profile-stages", action="store_true", help="current-only stage attribution; never acceptance or baseline comparison")
    parser.add_argument("--uncontended", action="store_true")
    parser.add_argument("--background-power-thermal", help="record observed background load, power mode and thermal condition")
    parser.add_argument("--cooldown-seconds", type=float, default=10)
    parser.add_argument("--process-timeout", type=int, default=1800)
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
    if not args.uncontended or not args.background_power_thermal:
        parser.error("stop competing work and record --uncontended plus --background-power-thermal")
    if args.pairs < (1 if args.probe or args.profile_stages else 5) or args.pairs > 20 or not 0 <= args.cooldown_seconds <= 60:
        parser.error("invalid pair count or cooldown")
    if any(value not in (100000, 150000) for value in args.populations) or not 30 <= args.process_timeout <= 3600 or len(set(args.populations)) != len(args.populations) or len(set(args.routes)) != len(args.routes):
        parser.error("invalid scale or external timeout")
    args.output.mkdir(parents=True, exist_ok=False)
    metadata = {"schema": 2, "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "platform": platform.platform(), "processor": platform.processor(), "logical_cpus": os.cpu_count(),
                "background_power_thermal": args.background_power_thermal, "uncontended_attested": args.uncontended,
                "probe": args.probe, "pairs": args.pairs, "cooldown_seconds": args.cooldown_seconds,
                "profile_stages": args.profile_stages, "study": "current-only-attribution" if args.profile_stages else "paired-boundary-only",
                "process_timeout_seconds": args.process_timeout, "populations": args.populations, "routes": args.routes,
                "completion_mode": "joined-yield-zero", "results": [], "arms": {}}
    metadata["wrapper_sha256"] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    metadata_path = args.output / "metadata.json"
    def save_metadata():
        metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    save_metadata()
    try:
        for arm in arms:
            metadata["arms"][arm] = provenance(getattr(args, arm), getattr(args, arm + "_build"), getattr(args, arm + "_source"))
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
                                   "--route", route, "--frames", "3" if args.probe else "1800",
                                   "--seconds", "0" if args.probe else "30", "--warmup-ticks", "0" if args.probe else "120",
                                   "--mutations", "1" if args.probe else "300", "--timeout-seconds", "30" if args.probe else "300",
                                   "--profile-stages", "true" if args.profile_stages else "false"]
                        if args.quality_images:
                            command += ["--capture-dir", str(directory)]
                        receipt = {"arm": arm, "population": population, "route": route, "pair": pair + 1,
                                   "command": command, "returncode": None, "timed_out": False}
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
                            (directory / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
                            receipt["sample_eligible"] = summary["acceptance_sample_eligible"] and not args.probe and not args.profile_stages
                            receipt["g3_target_met"] = summary["g3_target_met"]
                            receipt["g4_target_met"] = summary["g4_target_met"]
                            receipt["quality_pass"] = summary["quality_pass"]
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
