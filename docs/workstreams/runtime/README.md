# Runtime workstream — W2

The compatibility `run_ticks` path remains. Bounded ingress and sequential
pause/boundary/trace/replay are ready for architect integration; see
[design](design.md) and [validation](validation.md). Clock/Pipeline orchestration
remain pending.

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
