# Runtime workstream — W2

Phase14 proposal retains coordinator-only ingress/clock/outer BoundaryPipeline.
Root resolves startup execution policy into Contracts-owned values passed to
Simulation; Core/Scheduling must not depend on Runtime. All partition bodies and
callbacks join before capture/restart. See [scale contracts](../../decisions/phase14-scale-contracts.md)
and [all-stream audit](../hierarchy-boundaries.md#phase14-proposed-scale-assignment).

## Hierarchy and acceleration boundary

Own admission, replay/pause, deadline/catch-up and when resumable domain work runs. Scheduling owns executor joins; Spatial owns its cursor. Partial hierarchy work cannot silently complete a tick or change snapshot freshness policy.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners,
read/write sets, lifetimes, bounded progress and cross-project handoffs.

The compatibility `run_ticks` path remains. Bounded ingress and sequential
pause/boundary/trace/replay are integrated and validated; see [combined evidence](../integration/wave1-validation.md),
[design](design.md) and [validation](validation.md). The bounded clock and owned
summary increment is [review-ready](phase2-clock.md); combined verification and
production wiring are architect-owned. Pipeline orchestration remains pending.

## Scope and first task

Command ingress, fixed-step orchestration and lifecycle. Prerequisites: W0/W1.

Implement bounded ingress and cutoff drain before clocked/Pub execution.

## Exclusive ownership

- [Headers](../../../include/crucible/runtime/README.md): include/crucible/runtime/.
- [Sources/build manifest](../../../src/runtime/CMakeLists.txt): src/runtime/.
- [Tests/registration](../../../tests/runtime/CMakeLists.txt): tests/runtime/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Runtime. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Queue full, atomic batches, cutoff timing, replay, pause, shutdown and startup failure.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.
