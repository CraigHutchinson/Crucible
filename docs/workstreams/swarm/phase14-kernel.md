# Phase14 P14-01 immutable query and row provider

Source provider based on `0bfb0d299de38ef6eada740911d6d8f308739b4d`, branch
`phase14-kernel`, isolated `.worktrees/phase14-kernel`. Spatial/Swarm source,
headers, local fixtures/manifests and this handoff are the exclusive provider paths.
Implementation follows the G0a freeze in the
[scale contracts](../../decisions/phase14-scale-contracts.md).

## Consumed boundaries

`Grid::tryQuery` reads the committed index into a caller-owned result span.
Scratch fits every committed sample; finite/input/capacity validation precedes
writes. Exact inclusive filtering, clamping, H2 candidates/exact-scan fallback and
ascending IDs share the existing query implementation. The exclusive `TryQuery`
wrapper retains its existing owned scratch. A query result borrows caller storage,
so another task's scratch or a legacy query cannot overwrite it.

`Steering::tryComputeRows` reads the complete ordered tick-start input and immutable
Fields/Grid, and writes only the assigned pending range. The pending span size
defines range length. Full-input validation, subtraction-first range bounds,
input/output overlap and scratch capacity are checked before writes. Each task
owns disjoint pending/query spans. Unexpected missing/unknown membership may leave
staging partial; no caller may publish any range until every task succeeds and all
bodies/callbacks join. No worker calls ECS or rebuilds the index.

Both steering entry points use one private row kernel with the same ascending-ID
accumulation, coincidence direction, hypot, force/speed caps and integration/clamp
expressions. The legacy caller stages in its original startup-owned buffer before
copying, retaining overlap support and whole-destination preservation on failure.
No extra legacy startup storage is introduced. The new row entry point initially
validates complete input per partition; its cost belongs in subsequent receiving.

## Root integration requests

Root wires the frozen Simulation/ScenarioSettings path, first through the sequential
provider and later through independently received Scheduling partitions. Root owns
all authoritative state, frame publication, runtime/desktop/central inventories,
dependency promotion and serialized receiving CPU. The provider uses no new target,
dependency pin, executor implementation or public generic wrapper.

Before dispatch root supplies complete immutable input, a matching committed index,
per-task full-result scratch, disjoint pending ranges and matching floating-point
mode. After joins root validates full range coverage and all success signals before
the existing serial movement/resource/frame publication. Parallel execution remains
blocked by the Pipeline queue/submission/startup failure prerequisite, separate from
this source provider.

## Verification and limits

Source self-review covered L0 callers (legacy production Simulation through existing
TryCompute/TryQuery and the shared algorithms), L1 existing module homes/dependencies, L2 documented storage,
capacity/failure/borrow contracts and L3 unchanged arithmetic/no hot allocations.
New declarations use the sub0 profile. Existing class/file/member names remain the
established API; no repository-wide style migration is claimed.
The public row entry point has fixture callers at provider handoff; root must wire
its production Simulation/Scheduling caller before integrated completion. New
fixtures retain Crucible's standalone receiving harness without adding a test
framework dependency; filenames follow the profile's `test_<topic>.cpp` convention.

Registered receiving:

- `spatial_caller_scratch`: independent complete scan, sorted dense/coincident/edge
  queries, retained independent results, invalid/undersized scratch preservation,
  all four rounding modes and empty index.
- `swarm_partition_rows`: exact bitwise 1/2/7-task comparisons at 0/1/23/257 samples,
  sparse IDs, cross-range neighbors/coincidence, worker FP setup, empty/uneven ranges,
  whole-input validation, overflow offsets, overlap, late partial staging failure,
  legacy destination preservation and in-place recovery.
- `swarm_partition_allocations` and existing `spatial_allocation_reuse`: repeated
  success/rejection using startup storage, observing ordinary replaceable new/new[]
  only. They do not measure direct malloc, aligned/private allocations or RSS.
- Existing `swarm_steering` retains its independent arithmetic/property oracle;
  existing spatial/hex receiving retains independent geometry and extreme fixtures.

Provider receiving passed on 2026-10-08 with the root-granted sole CPU reservation:
fresh MSVC19.51.36246 Debug and Release builds, Ninja, `/Z7` Embedded debug
information, checksum-verified CPM0.42.1 and explicit read-only exact dependency
sources. Both configurations built only the seven registered Spatial/Swarm targets.
The repository `scripts/run_tests.py` runner passed all seven selected fixtures in
each configuration: Release7/7 and Debug7/7. Logs and preflight receipts remain in
`build/phase14-kernel-{release,debug}/kernel-tests.log` and
`kernel-prerequisites.json` in the retained worktree. No timing benchmark ran;
fixture durations are not throughput evidence. All native build/test processes
completed and the CPU reservation was released.

Receiving commands, with the matching isolated build directory supplied:

```text
python scripts/run_tests.py --preset release --test-dir build/phase14-kernel-release --prerequisite-build-dir build/phase14-kernel-release -R "^(spatial_(grid|hex_queries|allocation_reuse|caller_scratch)|swarm_(steering|partition_rows|partition_allocations))$"
python scripts/run_tests.py --preset debug --test-dir build/phase14-kernel-debug --prerequisite-build-dir build/phase14-kernel-debug -R "^(spatial_(grid|hex_queries|allocation_reuse|caller_scratch)|swarm_(steering|partition_rows|partition_allocations))$"
```

Unfiltered combined suites, supported sanitizer/race receiving, exact-head CI,
production row wiring, native scale execution and performance gates remain root
integration work. Concurrent native fixtures establish their observed behavior;
they do not replace a capable race-detector run.
