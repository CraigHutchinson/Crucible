# Production runtime backbone budgets

Status: received on the dedicated Windows machine, 5 October 2026. Five alternating
Release process pairs establish the bounded allocation budgets below. Timings are
advisory; these results do not establish a speedup or a full-game frame rate.

The benchmark uses the production `InspectorSession::ExecutionPath` comparator.
Both arms execute the same simulation, ingress, clock, scenario capture and mission
rules. Integrated adds synchronous `IntentDelivery` and the three-node
`BoundaryPipeline`; direct uses the production direct coordinator. There is no
benchmark copy of a scheduler, simulation or mission transition.

## Workload and scope

Each process runs admission-only delivery through a one-entry production queue
(one first request reported separately, then 10,000 steady accepted requests,
draining outside the admission measurement), then the
complete finite-resource inspector at 64 and 2,048 samples. Both use a 64x32 grid,
four field slots, 64 pending-command slots and 4,096 trace slots. Initial startup,
first pump, 15 further warm-up ticks and 120 observed steady pumps are distinct.
Steady production admission is timed independently before the corresponding pump.
The complete pump includes the actual boundary, simulation, owned frame capture
and publication; it does not include rendering.

Complete missions run at 2,048 samples using `GetReferenceMissionRouteEdit`.
The reference route must close with WON at tick 267. The structural route switches
to the receiving gathering field after quota, admits fusion when actual eligible
mobile mass permits it, and must close with WON at tick 448. Terminal states,
sample checksum and complete applied command traces are emitted. The correctness
gate remains `BackboneParity` full-state/mission/trace acceptance, separately from
timing. Observable benchmark summaries are an additional receiving sanity check.

Startup reports requested allocation calls/bytes and wall cost. Steady admission,
first pump, warmed complete pumps, mission admissions, mission pumps and whole
mission loops each report allocations and timings. Mission pump observations start
at tick zero and include first-run bookkeeping. Whole mission scopes include the
nested admission and pump scopes once: never add the whole-mission counts to their
component rows. Whole mission timing also includes route observations and probe
overhead. Startup destruction, sorting, checksum calculation and stdout occur
outside all reported intervals. Ordinary and mission workloads omit diagnostics.
A separate 64-sample paused session enables bounded diagnostics, fills its command
queue, measures the first refusal and 2,000 further refusals through exhaustion,
and observes the unchanged completed tick and counted drops.

The allocation probe replaces the ordinary, array, aligned and nothrow C++ `new`
families and matching sized/unsized deletes. Aligned allocations pair
`_aligned_malloc/_aligned_free` on Windows and `posix_memalign/free` on POSIX.
Counts cover successful coordinator-thread requests; bytes are requested volume,
not peak live memory. Direct malloc, library-private allocation, other threads,
GPU allocations and RSS are not intercepted. Clock/probe overhead is included in
the timings, especially noticeable for the narrow admission microbenchmark.

## Capture and acceptance

Build the benchmark in Release and pass its exact executable and build directory:

```powershell
python scripts/capture_runtime_backbone.py --binary build/phase12-release/benchmarks/crucible_runtime_backbone_bench.exe --build-dir build/phase12-release --output bench-results/phase12-runtime-NEW --pairs 5 --uncontended --background-load "Dedicated machine; heavy work stopped; record actual power mode here"
```

The output directory must be new. Five serial pairs alternate integrated/direct
and direct/integrated ordering, with a configurable two-second default cooldown.
Each arm is an independent process. Capture retains raw stdout/stderr, invocation
order/status/wall time, source HEAD/status/diff, benchmark sources, binary SHA256,
dependency pins, CMake cache, compiler configuration and CPU/OS observations.
The binary must reside inside the supplied build directory. Single-configuration
captures require `CMAKE_BUILD_TYPE=Release`; multi-configuration captures require
an artifact inside the actual Release folder. Stop competing workloads before attesting
`--uncontended`; the script does not control machine power state or detect contention.

Summaries report medians and observed ranges across process samples, including
per-process pump p95/p99. They are advisory; an isolated tick is not a whole-game
frame and these measurements do not establish 60 FPS. `--skip-missions` is a smoke
capture and cannot receive complete mission budgets.

The preserved [capture evidence](../integration/phase12-evidence/runtime-budgets/)
contains all ten raw JSONL/stderr pairs, invocations, source/pins, cache/compiler
configuration, benchmark/script inputs, CPU observations and summary. All raw files
are byte-identical to `bench-results/phase12-windows-pairs`; only metadata's absolute
workspace binary/build paths become relative. [sha256.json](../integration/phase12-evidence/runtime-budgets/sha256.json)
records the preserved file hashes. The exact clean source is
`1eef613c08d72535975aac9c7ce01c31f98efb9c`; the measured binary SHA256 is
`e327a232730e86b78fc8411ef79592da5dd90c9e84da584fb2d34b4bda2233d7`.

The host was Windows 11 build 26220, Intel Core Ultra 9 275HX (24 logical CPUs),
MSVC 19.51.36246.0 x64, Ninja, Release `/O2 /Ob2 /DNDEBUG`, with the diagnostics
feature compiled ON. Capture started at 09:21:35 UTC with three-second cooldowns.
The coordinator used one worker. No agent builds, tests, GPU or desktop runs
overlapped; AC power and unchanged OS power policy were recorded. VS Code, chat
and OS background processes remained, and no thermal telemetry was captured.
All ten process exits were zero, and observable terminal/checksum/trace signatures
agreed across both arms and all runs. Full-state acceptance is a separate test gate.

## Accepted allocation budgets

Counts below were identical across all five processes per arm. Bytes are requested
C++ allocation volume, under the probe's limits above. Startup values describe this
compiler/configuration, not a universal portable allocation ceiling.

| Gate | Direct | Integrated | Disposition |
| --- | --- | --- | --- |
| Ordinary startup, 64 samples | 85 calls / 523,599 bytes | 102 / 547,438 | startup storage received |
| Ordinary startup, 2,048 samples | 124 / 1,216,240 | 141 / 1,303,606 | startup storage received |
| Pub route startup | 0 / 0 | 2 / 232 | startup-only delivery owner |
| Pub first/steady admission; production and mission admission | 0 / 0 | 0 / 0 | accept zero per request |
| First complete pump, either population | 0 / 0 | 6 / 124 | accept bounded initial graph bookkeeping |
| Warm complete pump, either population | 0 / 0 | 3 / 96 per tick | accept explicit steady stop-state cost |
| Complete reference mission, WON at tick 267 | 0 / 0 | 804 / 25,660 | terminal received; graph cost matches formula |
| Complete structural mission, WON at tick 448 | 0 / 0 | 1,347 / 43,036 | terminal received; graph cost matches formula |
| Diagnostic first emission and 2,000 refusals through exhaustion | 0 / 0 | 0 / 0 | accept zero emission allocation |

The 120 warmed pumps total 360 calls / 11,520 bytes in each integrated process.
For an integrated mission of `N` ticks including the first pump, the observed
budget is `3*N + 3` calls and `96*N + 28` bytes. Diagnostic exhaustion counted
1,463 drops after 2,001 refused attempts, with completed tick still zero; loss
does not change queue refusal or grow the backing. This allocation gate covers
the measured refusal path, while summary encoding and state parity are covered
by receiving fixtures.

Retain Pipeline pin `f6f54c623908649e8daac3613545062cf08b3822`. The three small
steady allocations are a known, bounded receiving cost of its stop-state reset;
Phase12 accepts that budget rather than claiming an allocation-free graph. The
evidence does not justify speculative upstream run-storage changes. A future
consumed workload requiring zero run allocations must receive its neutral upstream
solution with repeat-run, failure/cancellation and package fixtures before changing
the full pin.

## Advisory timing observations

Each entry is the median of five process medians, with the observed process-median
range in parentheses. Units are microseconds except complete missions in
milliseconds. These are descriptive observations with background/thermal limits
above, and do not support a speedup conclusion. Full percentile/range data remains
in [summary.json](../integration/phase12-evidence/runtime-budgets/summary.json).

| Workload | Direct | Integrated |
| --- | --- | --- |
| Startup, 64 | 206.9 (204.0–225.0) | 218.9 (210.7–261.3) |
| Startup, 2,048 | 478.2 (446.8–577.7) | 497.7 (474.0–850.4) |
| Pub steady admission | 0.0 (0.0–0.0) | 0.1 (0.1–0.1) |
| Production admission, 2,048 | 0.55 (0.35–0.90) | 1.50 (0.90–1.75) |
| Warm complete pump, 64 | 45.90 (44.00–57.05) | 45.95 (44.60–56.05) |
| Warm complete pump, 2,048 | 2,271.80 (1,963.20–2,418.70) | 2,176.75 (1,942.75–2,468.30) |
| Reference complete mission, ms | 720.206 (657.719–776.725) | 706.334 (621.157–764.085) |
| Structural complete mission, ms | 1,345.869 (1,239.298–1,367.675) | 1,281.245 (1,163.602–1,332.652) |

The zero admission median reflects the timer's granularity, not zero work. For
2,048-sample warmed pumps, the medians of process p95/p99 are 2,639.3/2,699.2
direct and 2,544.3/2,602.6 integrated microseconds. Complete mission pump p95/p99
are 3,346.6/3,744.5 direct and 3,221.1/3,335.3 integrated for reference; structural
values are 3,827.6/4,464.8 and 3,651.3/3,886.1 respectively. Those pumps include
production work/capture, while rendering, presentation and end-to-end frame pacing
are excluded.

## Exact pinned Pipeline source assessment

Inspected `Sub0Pipeline` commit
`f6f54c623908649e8daac3613545062cf08b3822` using `git show`, not the mutable sibling
working tree. In `src/sub0pipeline.cpp`, `run_inline` at lines 959-970 constructs
the inline executor and delegates to ordinary `run`/`runImpl` at lines 663-675.
The production graph's untimed jobs use no worker threads or deadline path.

`runImpl` resets every node's `stopSource_` with `std::stop_source{}` at line 710
on every call. The warmed probe reports three calls / 96 bytes for the three-node
production graph, consistent with the actual MSVC standard library's three
32-byte stop states. Graph construction does not preclude these per-run
allocations; no allocation-stack attribution is claimed from the counting probe.

Initial validation uses `validationDegrees_`/`validationReady_` at lines 632-636;
the root cache pushes into `roots_` at lines 696-704. Their lazy first-run growth is
distinguished by the first-pump row and subsequent warmed rows. Dispatch lambdas
at lines 834-849 use `DispatchContext` pointer/index captures intended to fit
`std::function` small-object storage; whether the actual toolchain allocates is
measured rather than inferred from a source comment. Failure-only `skipped_`
reserve at line 763 is not exercised by the successful workload. Timed-job
completion/packaged-task/thread allocations belong to deadline execution and are
not attributed to this production graph. No upstream files were changed here.
