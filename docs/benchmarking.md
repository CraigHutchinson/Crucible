# Benchmark workflow

```sh
cmake --preset bench
cmake --build --preset bench --parallel 4
python scripts/run_tests.py --preset bench
python3 scripts/capture_benchmarks.py --output bench-results/current
```

The initial workload measures only single-threaded ECS position integration at
100,000 and 150,000 entities: 60 warm-up ticks, then 600 measured ticks. Setup,
checksum and stdout are outside measured intervals. The checksum keeps results
observable. Output is JSONL with median, p95 and observed min/max microseconds.

The capture script saves at least five independent process samples, raw stdout
and stderr, source revision and dirty status, CPU/OS details, dependency pins,
CMake cache and compiler configuration. Output directories must be new so a run
cannot silently overwrite evidence. Report process medians and observed ranges.

Compare baseline and current builds on the same machine with identical compilers,
flags and power settings. Run at least five alternating baseline/current process
pairs; retain separate capture directories and each invocation's raw results.
Record background load, worker count, telemetry settings and memory use separately.
Shared GitHub runners are advisory: no automatic regression or FPS threshold.

Future full-tick benchmarks must cover hash rebuild, neighbor/field work, Blight
automata, collision resolution, input handoff, telemetry and structural fusion.
Report whole-tick p95/p99, dropped commands/telemetry, peak memory, allocations,
worker count, grid sizes and occupancy. Rendering needs its own end-to-end frame
measurement. The goal is a 16.67 ms frame budget; current results do not prove it.

The [Phase14 major package](phases/phase14.md#measurement-and-handoff) now selects
that full workload as proposed core at100K/150K. Its G0a/G0b checkpoints freeze
evolving scenarios, density classes, field-driven behavior, exact native completion
observation, sample minima and numerical/visual criteria before optimized acceptance.
Use the same production DesktopApp/Simulation consumer, retain raw timelines and
controlled pairs, and report CPU work/pacing/GPU completion separately. No new
full-frame or scale result exists from this planning update.
