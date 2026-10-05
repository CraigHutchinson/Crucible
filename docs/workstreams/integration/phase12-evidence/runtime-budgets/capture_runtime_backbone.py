#!/usr/bin/env python3
"""Retain five or more serial, alternating production runtime process pairs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import time


def command_output(command, cwd=None):
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True, check=True)
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--pairs", type=int, default=5)
    parser.add_argument("--uncontended", action="store_true",
                        help="attest that builds, tests and other heavy workloads are stopped")
    parser.add_argument("--background-load", required=True,
                        help="describe observed background load and power settings")
    parser.add_argument("--cooldown-seconds", type=float, default=2)
    parser.add_argument("--skip-missions", action="store_true",
                        help="smoke only: omits complete production mission budgets")
    args = parser.parse_args()
    if args.pairs < 5:
        parser.error("at least five independent process pairs are required")
    if not args.uncontended:
        parser.error("stop competing heavy work, then explicitly pass --uncontended")
    if args.cooldown_seconds < 0 or args.cooldown_seconds > 60:
        parser.error("cooldown must be between zero and 60 seconds")
    binary = args.binary.resolve(strict=True)
    root = Path(__file__).resolve().parents[1]
    build = args.build_dir.resolve(strict=True)
    if not binary.is_file() or not binary.is_relative_to(build):
        parser.error("benchmark binary must be a file inside --build-dir")
    relative_binary = binary.relative_to(build)
    cache = (build / "CMakeCache.txt").read_text()
    cache_values = {line.split("=", 1)[0]: line.split("=", 1)[1]
                    for line in cache.splitlines() if "=" in line and not line.startswith(("#", "//"))}
    configurations = cache_values.get("CMAKE_CONFIGURATION_TYPES:STRING", "").split(";")
    if configurations != [""]:
        if "Release" not in configurations or "Release" not in relative_binary.parts[:-1]:
            parser.error("multi-configuration benchmark binary must be inside its Release folder")
    elif cache_values.get("CMAKE_BUILD_TYPE:STRING") != "Release":
        parser.error("single-configuration capture requires CMAKE_BUILD_TYPE=Release")
    git = ["git", "-c", f"safe.directory={root.as_posix()}"]
    source_ref = command_output(git + ["rev-parse", "HEAD"], root).strip()
    status = command_output(git + ["status", "--porcelain"], root)
    diff = command_output(git + ["diff", "HEAD", "--binary"], root)
    args.output.mkdir(parents=True, exist_ok=False)
    metadata = {
        "source_ref": source_ref, "git_status": status,
        "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "platform": platform.platform(), "processor": platform.processor(),
        "machine": platform.machine(), "logical_cpus": os.cpu_count(),
        "binary": str(binary), "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "build_dir": str(build), "configuration": "Release", "pairs": args.pairs,
        "background_load_and_power": args.background_load, "uncontended_attested": args.uncontended,
        "cooldown_seconds": args.cooldown_seconds, "skip_missions": args.skip_missions,
        "workers": 1, "telemetry": "disabled", "rendering": "excluded",
        "allocation_scope": "successful replaceable C++ new/new[] on coordinator thread; requested bytes",
        "limits": "No malloc/RSS/peak-live-memory coverage or full-game FPS claim. Timing advisory.",
        "workload": "production InspectorSession direct versus integrated; 64 and 2048 samples; 64x32 grid",
    }
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.exists():
        (args.output / "cpuinfo.txt").write_text(cpuinfo.read_text())
    elif platform.system() == "Windows":
        result = subprocess.run(["powershell", "-NoProfile", "-Command",
                                 "Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors | ConvertTo-Json"],
                                text=True, capture_output=True)
        (args.output / "cpuinfo.json").write_text(result.stdout)
        (args.output / "cpuinfo.stderr").write_text(result.stderr)
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    (args.output / "source.diff").write_text(diff)
    (args.output / "CMakeCache.txt").write_text(cache)
    (args.output / "DependencyPins.cmake").write_text((root / "cmake/DependencyPins.cmake").read_text())
    for index, source in enumerate(sorted(build.glob("CMakeFiles/*/CMakeCXXCompiler.cmake"))):
        (args.output / f"compiler-{index}.cmake").write_text(source.read_text())
    # Untracked sources are absent from git diff. Preserve the benchmark inputs verbatim.
    for source in (root / "benchmarks/runtime_backbone.cpp", Path(__file__).resolve()):
        (args.output / source.name).write_bytes(source.read_bytes())
    groups = {}
    signatures = {}
    invocations = []
    for pair in range(args.pairs):
        arms = ("integrated", "direct") if pair % 2 == 0 else ("direct", "integrated")
        for order, arm in enumerate(arms):
            command = [str(binary), "--arm", arm]
            if args.skip_missions:
                command.append("--skip-missions")
            started = time.monotonic()
            result = subprocess.run(command, text=True, capture_output=True, cwd=root)
            stem = f"pair-{pair:02d}-{order}-{arm}"
            (args.output / f"{stem}.jsonl").write_text(result.stdout)
            (args.output / f"{stem}.stderr").write_text(result.stderr)
            invocations.append({"pair": pair, "order": order, "arm": arm, "command": command,
                                "returncode": result.returncode, "wall_seconds": time.monotonic() - started})
            (args.output / "invocations.json").write_text(json.dumps(invocations, indent=2) + "\n")
            result.check_returncode()
            for line in result.stdout.splitlines():
                row = json.loads(line)
                if row["arm"] != arm:
                    raise RuntimeError("benchmark emitted wrong arm")
                key = (row["workload"], row["entities"])
                if "median_us" in row:
                    groups.setdefault((*key, arm), []).append(row)
                else:
                    # Observable summaries are a sanity check; full-state parity is a separate CTest gate.
                    signature = {k: v for k, v in row.items() if k != "arm"}
                    if key in signatures and signatures[key] != signature:
                        raise RuntimeError(f"observable receiving result changed: {key}")
                    signatures[key] = signature
            time.sleep(args.cooldown_seconds)
    summaries = []
    for (workload, entities, arm), rows in sorted(groups.items()):
        summary = {"workload": workload, "entities": entities, "arm": arm, "processes": len(rows)}
        for metric in ("median_us", "p95_us", "p99_us", "total_us", "new_calls", "new_bytes"):
            values = [row[metric] for row in rows]
            summary[metric] = {"median": statistics.median(values), "min": min(values), "max": max(values)}
        summaries.append(summary)
    (args.output / "summary.json").write_text(json.dumps(summaries, indent=2) + "\n")
    print(json.dumps(summaries, indent=2))


if __name__ == "__main__":
    main()
