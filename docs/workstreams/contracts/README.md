# Contracts workstream — W1

Phase14 proposed assignment: root defines consumed scenario/execution/observation
values, epoch/scratch bounds, result/failure and numerical policy with affected
callers. Runtime resolves policy; Core and Scheduling never depend on Runtime.
G0a/G0b freeze before dependent adoption. See [scale contracts](../../decisions/phase14-scale-contracts.md)
and [all-stream audit](../hierarchy-boundaries.md#phase14-proposed-scale-assignment).

## Hierarchy and acceleration boundary

Own consumed shared snapshot/request/result values and failure/capacity compatibility. Keep epochs, snapshot rows, SampleId and ECS handles distinct; geometry, reducer algorithms and tree storage stay with their providers.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners,
read/write sets, lifetimes, bounded progress and cross-project handoffs.

Grid geometry, owned field edits and stable sample IDs join Position/Velocity and
fixed timing. The consumed first contracts are integrated and validated; broader
snapshot/resource contracts remain pending. See [combined evidence](../integration/wave1-validation.md).

## Scope and first task

Shared value contracts and compatibility. Prerequisites: W0.

Validate scenario limits and stable identity against pinned ECS.

## Exclusive ownership

- [Headers](../../../include/crucible/contracts/README.md): include/crucible/contracts/.
- [Sources/build manifest](../../../src/contracts/CMakeLists.txt): src/contracts/.
- [Tests/registration](../../../tests/contracts/CMakeLists.txt): tests/contracts/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Contracts. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Overflow/nonfinite inputs, owned values and named consumers.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.
