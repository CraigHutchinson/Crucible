# Phase14 bounded row execution

`RowPartitions` is the Scheduling provider for Simulation's immutable steering
proposal ranges. Runtime selects resolved startup `ExecutionSettings`; Simulation
owns complete input, pending output, fields/index and full query scratch per
partition. Scheduling owns its callback, fixed partition metadata, untimed graph
and optional exclusive `Sub0Pipeline::Priority` pool. It does not depend on Core,
Runtime, Swarm, ECS, Pub or desktop APIs.

The [frozen scale contract](../../decisions/phase14-scale-contracts.md#c2-scheduling-and-failure-joins)
controls this increment. Integration and native/performance acceptance remain
separate from these provider fixtures.

## Execution and storage

The constructor validates positive resolved counts, task/count representation,
partition count at most `max(rowCapacity,1)` and the received 65536-node index domain, storage representation and a nonempty
callback before constructing owned graph storage or launching workers. Allocation
and launch failure propagate; cleanup joins previously started workers.

The graph contains one root per startup partition, with no dependencies, timed
jobs, recursive submission or per-entity jobs. One worker runs this same graph
inline; selected multiple workers use a private pool whose queued capacity is the
partition count. Constructor priming runs an explicitly inactive inline epoch:
it populates graph caches without invoking the row callback.

Each run partitions the actual rows by quotient/remainder into contiguous ranges.
At most `min(rows,partitions)` callbacks run; remaining tasks are empty. Zero rows
return complete without dispatch. Metadata and task count never grow during a
run. Simulation's separate query storage is `P*N*sizeof(SampleId)`; each worker
mutates only its supplied pending range and scratch slot.

Calls and destruction require exclusive, non-reentrant coordinator access. The
startup callback may be invoked concurrently; captured input, fields/index and
other state must remain immutable until joined return. The callback must not
restart, replace, destroy or recursively execute the run.

## Status and lifetime

Only `complete` allows publication. Excess rows return `invalidRows` without
callbacks. False/throwing callbacks and graph/submission errors return `failed`
after accepted work joins. Normal joined failures permit provider reuse; Runtime
owns its stricter fail-stop policy. The graph receives bodies, completion callbacks
and accepted callable-target destruction before returning.

An unsupported floating-point installation invokes no row callback for that
partition. It returns graph success with private unsupported metadata, allowing
`unsupportedFloatingPoint` only when the entire graph joins successfully. Other
partitions may have written partial staging: the consumer must discard all ranges
and recompute the entire result sequentially before publication. Submission or
body failure always dominates unsupported FP.

Pool jobs snapshot the worker environment, install/verify the coordinator fenv
and supported x86 MXCSR control bits, then restore the worker before Pipeline
completion. Checked normal restoration and an unwinding guard cover both returns
and throws. Restoration failure returns `failed` and poisons the adapter: later
calls, including zero rows, fail without further dispatch. Inline execution
preserves the coordinator's ordinary arithmetic exception-flag effects.

Destruction drains/stops the pool before releasing graph, metadata or callback
state. No hard wall-clock guarantee is implied for a noncooperative callback.
Legal restart/close and complete-state publication require root integration tests;
concurrent destruction during `tryRun` violates this provider's precondition.

## Provider receiving

Three fixtures are registered under Scheduling:

- `scheduling_row_partitions`: startup callback suppression, exact/uneven/empty
  coverage at 0/1/23/257 rows, 1/2/4 workers and 1/2/7 partitions; prelaunch invalid
  bounds; held-partition false/throw failure joins, valid reuse and callback capture
  teardown after joined use.
- `scheduling_row_floating_point`: actual Steering full-oracle bitwise comparisons
  at 23/257 rows, four standard rounding modes and supported x86 FTZ/DAZ combinations;
  coordinator flags, pool isolation and throwing-callback recovery. The test reads
  the FXSAVE MXCSR_MASK before writing DAZ, following the
  [Intel SDM](https://cdrdv2-public.intel.com/868137/325462-089-sdm-vol-1-2abcd-3abcd-4.pdf).
- `scheduling_row_allocations`: ordinary replaceable `new/new[]` on all pool threads,
  including the first actual run, steady runs, invalid/empty rows, false callback
  failures and recovery. This does not observe direct malloc, aligned/private/OS
  allocation, live/peak memory or a universal allocator guarantee.

Provider receiving passed on 2026-10-08 in the isolated `phase14-scheduling`
worktree based on `97152dc93f2aa3b7d4b391bc5a65f729f75c70ed`. Exact received
Pipeline source is `f730c4ec2973a449c45fbf9a74595414b9bf30e1`; other full pins
remain unchanged. Builds use MSVC 19.51.36246, Ninja, Embedded `/Z7` debug
information, checksum-verified CPM0.42.1, absolute read-only dependency overrides
and the repository prerequisite-aware runner. Debug 3/3 and Release 3/3 passed.
Both configurations actually received all four rounding modes and supported
FTZ/DAZ combinations; this host reported MXCSR denormal mask `0x8040`.

```text
python scripts/run_tests.py --preset debug --test-dir build/phase14-scheduling-debug --prerequisite-build-dir build/phase14-scheduling-debug --prerequisite-report build/phase14-scheduling-debug/scheduling-prerequisites.json -R "^scheduling_row_" -V
python scripts/run_tests.py --preset release --test-dir build/phase14-scheduling-release --prerequisite-build-dir build/phase14-scheduling-release --prerequisite-report build/phase14-scheduling-release/scheduling-prerequisites.json -R "^scheduling_row_" -V
```

Release first actual runs and 32 repetitions of success/invalid/empty/failure/recovery
observed 0 ordinary allocation calls/bytes at 1, 2 and 4 workers. The first computation
was included in the audit after inactive constructor priming. This is a receiving
count, not performance evidence.

Checked MSVC Debug orchestration is qualified separately under the frozen G2
contract. Observed first actual calls were 7/9/7 and 112/144/112 requested bytes;
the full audit observed 480/485/483 calls and 7680/7760/7728 requested bytes at 1/2/4
workers. A retained auxiliary Debug stack capture attributes the first 16-byte
allocation to `std::_Container_base12::_Alloc_proxy`, the `std::string` move
constructor and Pipeline's per-node `jobErrorCtx` consumption. Counts vary with
worker initialization and failure scheduling. The Debug fixture reports these
orchestration costs and retains correctness/output-address checks; unchecked STL
and Release retain the strict zero-allocation assertion. Existing separate
kernel/query allocation fixtures are unchanged. No claim of zero Debug scheduler
or universal allocator overhead is made.

Raw final logs/preflight JSON remain under both owned build directories.
`build/allocation-trace.log` retains the requested-size/call-stack attribution.
Initial Release callback-throw fail-fast logs are retained: Pipeline's untimed body
contract required adapter containment inside the worker job, corrected with a
nonthrowing function-try-block and restored-before-catch RAII. The initial Debug
allocation failure is also retained before its measured orchestration qualification.
Independent source review received both corrections; final regression fixtures pass.

The public tests establish requested FP controls, coordinator effects, bitwise
math and throwing-callback recovery. They do not directly observe each persistent
worker's original rounding/sticky flags between jobs. Naturally unavailable FP
installation/restoration failures remain source-audited branches; no production
test injection surface was added. Root receiving still requires integrated 1/2/N
full-state parity, supported sanitizers/race detection, legal restart/close and
physical frame budget evidence at both target scales.