# Benchmark workflow

Every optimization follows the [codified process](OPTIMIZATION_PROCESS.md), with
an [iteration record](optimization/iteration-template.md). It requires profiling,
vectorization/code-generation and aliasing review, repeated refinement, controlled
timing and combined consumer receiving. The initial integration loop below is one
advisory workload; it is not full-frame acceptance.

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

The production headless receiver below now covers spatial rebuild, complete
neighbor/field work, Blight/resources, command drain and snapshot publication;
separate complete missions cover structural fusion. Future receivers must cover
collision resolution and full native input/rendering service as those paths evolve.
Report whole-tick p95/p99, dropped commands/telemetry, peak memory, allocations,
worker count, grid sizes and occupancy. Rendering needs its own end-to-end frame
measurement. The goal is a 16.67 ms frame budget; current results do not prove it.

The [Phase14 major package](phases/phase14.md#measurement-and-handoff) now selects
that full workload as proposed core at 100K/150K. Its G0a/G0b checkpoints freeze
evolving scenarios, density classes, field-driven behavior, exact native completion
observation, sample minima and numerical/visual criteria before optimized acceptance.
Use the same production DesktopApp/Simulation consumer, retain raw timelines and
controlled pairs, and report CPU work/pacing/GPU completion separately. Scenario
and native lifecycle receiving is recorded in the Phase14 review; full-frame and
useful-parallelism targets remain unreceived.

The separate hosted `race` job builds headless Debug with actual ThreadSanitizer
flags, executes the configured compiler/runtime canary and receives Spatial,
Swarm, Scheduling and Pipeline-labelled ownership fixtures. It does not measure
timings or infer race freedom from a passed ordinary/sanitizer suite. Availability
and findings are receiving results; no local Windows ThreadSanitizer pass is claimed.


## Production headless pump and snapshot

The [production optimization record](optimization/production-gather-commit.md)
freezes source, dependency pins, correctness gates and control/A/B/A+B arms.
Build clean source checkouts with the same toolchain and settings:

```sh
cmake -S /path/to/arm -B /path/to/arm/build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DCRUCIBLE_BUILD_BENCHMARKS=ON \
  -DCRUCIBLE_BUILD_DESKTOP=OFF -DCRUCIBLE_BUILD_GPU_RECEIVER=OFF
cmake --build /path/to/arm/build --target crucible_runtime_backbone_bench --parallel 2
/path/to/arm/build/benchmarks/crucible_runtime_backbone_bench \
  --arm integrated --scale 100000 --workers 4 --warmup 120 --ticks 300 --route gather
```

`--scale` accepts 2048, 100000 or 150000, with respective 64×32, 400×250 and
500×300 worlds. `--route flow|gather` selects field intent; gather cycles FLOW,
repel and attraction every six measured ticks. `--partitions 0` (the default)
resolves to one partition for one worker and twice the worker count otherwise.
This uses real production steering and publication; it does not render or pace.
Admission is outside the timed pump. `--profile-stages` is an integrated-only
attribution diagnostic and must never enter qualifying timing cohorts.

For repeated comparisons, the owned Sub0ECS project supplies the collector:

```sh
python3 /path/to/Sub0ECS/bench/tools/crucible.py --source /path/to/control \
  --build control=/path/to/control/build --build candidate=/path/to/candidate/build \
  --scale 100000 --workers 4 --warmup 120 --ticks 300 --field-route gather \
  --arms integrated --rounds 5 --cooldown 2 --timeout 600 --out /path/to/new-capture
```

Repeat at all three scales and keep each cohort's raw process receipts, including
failures. Scale captures require identical benchmark source and dependency pins,
complete settings and command counts, matching initial/final semantic state and
trace digests, and zero observations, fallback or discarded work. Digests detect
drift; independent bitwise oracles and complete suites remain correctness gates.
Run without `--scale` or `--skip-missions` for existing reference/structural mission
receiving. Retained summaries cannot replace missing raw process receipts.

The completed campaign promotes checked commit mapping plus guarded sorting with
unchanged pins and public APIs: median pump/capture improves 11.18% at 100K and
13.23% at 150K, and the fresh 2048 and complete-mission median gates pass. The
[production record](optimization/production-gather-commit.md#final-small-world-and-complete-mission-receiving)
retains the mixed 100K/small-world tail results. This does not close native G3–G5.
