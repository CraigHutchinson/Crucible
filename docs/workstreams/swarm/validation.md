# Phase 2 swarm handoff

Worktree: `.worktrees/phase2-swarm`, branch `phase2-swarm`, supplied base `a4e83f0`.
Exclusive edits: swarm headers/source/tests/docs only. Root owns Git, shared
Simulation integration, central wiring, builds and CPU reservations. No worker
builds, benchmarks, installs, dependency edits or Git writes occurred.

Implementation follows the exact constructor and TryCompute API in the execution
ADR. Root must construct the provider only for the optional steering path, gather
immutable ID-sorted tick-start values, rebuild the matching Grid, invoke TryCompute
before committing ECS state, and rebuild committed-position diagnostics afterward.
Failure must preserve committed Simulation state. Absence of steering retains the
legacy scenario path. Shared headers were consumed without changes.

`swarm_steering` is registered locally with CTest label `swarm`. Fixtures include
an independent all-pairs oracle with independently derived radial field math; empty,
isolated, pair, antisymmetric coincident, inclusive zero-weight border, world corners,
radial attract/repel, Euclidean caps, saturated fields and subnormal radius cases.
The 2048-sample fixture mixes 1024 coincident neighbors and distributed samples.
Reversed spatial gather must preserve exact output; unsorted input is rejected.
Repeated calls reuse the provider, aliasing is checked, output tails are preserved,
and insufficient capacity/invalid state/late unknown query ID failures must leave
the whole destination untouched. Startup geometry/settings/oversized capacity
validation has explicit fixtures.

Validation status: **REQUESTED**. Root should build `crucible_swarm_tests`, run
`ctest --preset debug -L swarm --no-tests=error`, then the combined unfiltered
Debug/Release/ASan/UBSan gate after integration. The architect additionally owns
tiny and 2048-entity integrated owned-state/replay acceptance. No executable result
or performance conclusion is claimed here. Scratch reuse is exercised by repeated
calls; allocation freedom is established by static inspection of the hot path,
not a runtime allocator interception fixture.

Static checks: `git diff --check` passed for tracked edits. New source/header/test
files were read directly during the review because they are intentionally untracked
until root performs the Git handoff. Git status emitted an inaccessible global-ignore
warning; it did not prevent worktree status inspection.
