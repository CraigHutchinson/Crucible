# Production runtime backbone budgets

Status: receiving harness implemented; measured values and disposition pending
the architect's uncontended Release capture. No performance or allocation budget
has passed merely by adding this harness.

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
outside all reported intervals. Diagnostics are omitted.

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
python scripts/capture_runtime_backbone.py --binary build/bench/benchmarks/crucible_runtime_bench.exe --build-dir build/bench --output bench-results/phase12-runtime-NEW --pairs 5 --uncontended --background-load "Dedicated machine; heavy work stopped; record actual power mode here"
```

The output directory must be new. Five serial pairs alternate integrated/direct
and direct/integrated ordering, with a configurable two-second default cooldown.
Each arm is an independent process. Capture retains raw stdout/stderr, invocation
order/status/wall time, source HEAD/status/diff, benchmark sources, binary SHA256,
dependency pins, CMake cache, compiler configuration and CPU/OS observations.
Multi-configuration captures require the supplied binary to be the Release
artifact; the script records the configuration but cannot prove artifact provenance
from a multi-configuration cache alone. Stop competing workloads before attesting
`--uncontended`; the script does not control machine power state or detect contention.

Summaries report medians and observed ranges across process samples, including
per-process pump p95/p99. They are advisory; an isolated tick is not a whole-game
frame and these measurements do not establish 60 FPS. `--skip-missions` is a smoke
capture and cannot receive complete mission budgets.

Pending measured table:

| Gate | Direct | Integrated | Disposition |
| --- | --- | --- | --- |
| Startup calls/bytes/cost, 64 and 2,048 | pending | pending | startup storage reviewed separately |
| Pub admission calls/bytes/cost | pending | pending | allocation-free steady admission desired |
| First complete pump calls/bytes/cost | pending | pending | expose lazy graph bookkeeping |
| Warm complete pump calls/bytes/p95/p99 | pending | pending | measured overhead and exact allocation source |
| Complete reference/structural mission | pending | pending | terminal gates plus whole-loop overhead |

The architect will attach the exact capture path, measured values and accepted
budget/disposition after the paired run. If run storage is necessary, evolve and
test the neutral solution upstream, then receive its full dependency pin.

## Exact pinned Pipeline source assessment

Inspected `Sub0Pipeline` commit
`f6f54c623908649e8daac3613545062cf08b3822` using `git show`, not the mutable sibling
working tree. In `src/sub0pipeline.cpp`, `run_inline` at lines 959-970 constructs
the inline executor and delegates to ordinary `run`/`runImpl` at lines 663-675.
The production graph's untimed jobs use no worker threads or deadline path.

`runImpl` resets every node's `stopSource_` with `std::stop_source{}` at line 710
on every call. This is the primary steady bookkeeping allocation candidate for
three-node production graphs; the precise requested sizes/counts depend on the
actual standard library and are to be confirmed by the probe. Graph construction
does not preclude these per-run stop-state allocations.

Initial validation uses `validationDegrees_`/`validationReady_` at lines 632-636;
the root cache pushes into `roots_` at lines 696-704. Their lazy first-run growth is
distinguished by the first-pump row and subsequent warmed rows. Dispatch lambdas
at lines 834-849 use `DispatchContext` pointer/index captures intended to fit
`std::function` small-object storage; whether the actual toolchain allocates is
measured rather than inferred from a source comment. Failure-only `skipped_`
reserve at line 763 is not exercised by the successful workload. Timed-job
completion/packaged-task/thread allocations belong to deadline execution and are
not attributed to this production graph. No upstream files were changed here.
