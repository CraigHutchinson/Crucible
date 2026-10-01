# Workstream groundwork validation — 2026-10-01

This validates folder/build boundaries and the small existing-workload extractions;
it does not complete W0–W10 gameplay or concurrency packages.

## Implemented foundations

- Dedicated briefs for twelve workstreams; ten domain header/source/test areas.
- Central source/test inventories with explicit stream-owned manifests. Reserved
  streams are INTERFACE targets without dummy code or unimplemented success APIs.
- Position/Velocity identities and the 1/60 float interval retained in Contracts.
- Swarm integration stays inline with unchanged arithmetic and a real Simulation caller.
- Runtime run_ticks drives the existing headless executable, with zero/empty/exact-tick tests.
- Core no longer publicly links Pub/Pipeline/Log. Integration tests explicitly link
  their stack requirements; benchmark output location remains capture-compatible.

## Checks and limits

Windows: MSVC 19.51.36246.0 / VS 18 Community, Ninja and CMake presets. Debug,
Release and bench configurations/builds passed; each unfiltered suite has three
tests (simulation, stack_round_trip, runtime_headless). The runtime label selects
one test. Debug and Release executable checks report 225000 for 150K entities/60 ticks.
The benchmark executable was built, not measured; no performance claim is made.

Linux: Ubuntu 24.04 WSL, GCC 15.2.0 and CMake 4.2.3. The sanitize preset builds and
passes all three tests with ASan/UBSan, ASAN_OPTIONS=detect_leaks=1 and
UBSAN_OPTIONS=halt_on_error=1. The corrected stack
test also passed three consecutive repetitions on the mounted Windows filesystem.
This is sequential integration coverage, not a race check or threaded workload.

An initial WSL stack test failed opening the decoded segment while its writer mapping
was still alive. Decoding now starts after Logger/ScopedBind/executor teardown; that
ordering resolved the failure. Diagnostics also reject unreadable/empty input before
constructing a byte range. No logger-library or filesystem policy was changed.

Pinned Pipeline emits a padding warning with MSVC and missing-field-initializers for
DispatchContext::skipMutex with GCC. These dependency warnings remain unsuppressed.
No new dependency revisions or graphics packages were added. Independent read-only
cpp-review checked ownership, API compatibility, inline math and manifest boundaries.
Groundwork Markdown links and git diff whitespace checks pass.

## Remaining work

Audit full pinned dependency identity/capacity/thread contracts; define scenario and
command values; implement bounded ingress/replay, gameplay, partition execution,
snapshots and rendering in their respective streams. Structural commit exception/
allocation guarantees and whole-workload performance remain unproven. See the
[stream map](../README.md) and [backlog](../../work-breakdown.md).
