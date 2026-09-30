#!/usr/bin/env python3
"""Capture independent Release process samples and reproducibility metadata."""
import argparse
import json
import platform
from pathlib import Path
import statistics
import subprocess

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, default=Path("build/bench/crucible_bench"))
    parser.add_argument("--build-dir", type=Path, default=Path("build/bench"))
    parser.add_argument("--output", type=Path, default=Path("bench-results/current"))
    parser.add_argument("--samples", type=int, default=5)
    args = parser.parse_args()
    if args.samples < 5:
        parser.error("at least five independent process samples are required")
    cache = args.build_dir / "CMakeCache.txt"
    cache_text = cache.read_text()
    if "CMAKE_BUILD_TYPE:STRING=Release" not in cache_text:
        parser.error("benchmark capture requires a Release CMake build")
    args.output.mkdir(parents=True, exist_ok=False)
    metadata = {
        "source_ref": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
        "git_status": subprocess.check_output(["git", "status", "--porcelain"], text=True),
        "platform": platform.platform(), "processor": platform.processor(),
        "machine": platform.machine(), "samples": args.samples,
        "binary": str(args.binary.resolve()), "workload": "ecs_integration",
        "limits": "Single-threaded ECS integration only; no full-game FPS claim."
    }
    if Path("/proc/cpuinfo").exists():
        metadata["cpuinfo"] = Path("/proc/cpuinfo").read_text()
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    (args.output / "CMakeCache.txt").write_text(cache_text)
    compiler_files = list(args.build_dir.glob("CMakeFiles/*/CMakeCXXCompiler.cmake"))
    for source in compiler_files:
        (args.output / source.name).write_text(source.read_text())
    pins = Path(__file__).resolve().parents[1] / "cmake/DependencyPins.cmake"
    (args.output / pins.name).write_text(pins.read_text())
    groups = {}
    for index in range(args.samples):
        result = subprocess.run([str(args.binary.resolve())], check=True, capture_output=True, text=True)
        (args.output / f"sample-{index:02d}.jsonl").write_text(result.stdout)
        (args.output / f"sample-{index:02d}.stderr").write_text(result.stderr)
        for line in result.stdout.splitlines():
            row = json.loads(line)
            groups.setdefault(row["entities"], []).append(row["median_us"])
    summary = {str(count): {"median_us": statistics.median(values),
                            "min_us": min(values), "max_us": max(values)}
               for count, values in groups.items()}
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))

if __name__ == "__main__":
    main()
