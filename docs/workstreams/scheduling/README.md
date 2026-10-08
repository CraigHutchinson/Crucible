# Scheduling workstream — W7

Phase14 proposed first consumer: Simulation synchronously executes an untimed
partition graph over immutable input; workerA owns this adapter and its independent
join/dispatch/failure tests. Root retains outer Runtime graph and shared wiring.
Priority pool storage/failure guarantees require G0b receiving; fixed workers
alone do not bound the queue. See [scale contracts](../../decisions/phase14-scale-contracts.md)
and [all-stream audit](../hierarchy-boundaries.md#phase14-proposed-scale-assignment).

## Hierarchy and acceleration boundary

Own executor adaptation, phase access sets, work partitioning and CPU/device completion/failure joins. Execute Runtime policy; do not choose index semantics, view approximation, navigation rules or publication freshness. Native resources belong to the named adapter.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners,
read/write sets, lifetimes, bounded progress and cross-project handoffs.

Reserved build boundary (INTERFACE target); algorithms and public APIs are pending.

## Scope and first task

Bounded execution and phase access declarations. Prerequisites: W0/W2/W6.

Audit Pipeline joins and construct sequential/parallel replay before workers.

## Exclusive ownership

- [Headers](../../../include/crucible/scheduling/README.md): include/crucible/scheduling/.
- [Sources/build manifest](../../../src/scheduling/CMakeLists.txt): src/scheduling/.
- [Tests/registration](../../../tests/scheduling/CMakeLists.txt): tests/scheduling/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Scheduling. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Exact IDs/events/resources, numeric tolerance, failure joins, shutdown and race checks.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.
