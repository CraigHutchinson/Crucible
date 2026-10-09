# Swarm workstream — W5

Phase14 proposed assignment: workerA factors ordered row computation over complete
immutable input into disjoint pending outputs; root commits only after all joins.
Existing math/FP/order and failure-preservation stay authoritative. See
[scale contracts](../../decisions/phase14-scale-contracts.md) and the
[all-stream audit](../hierarchy-boundaries.md#phase14-proposed-scale-assignment).

## Hierarchy and acceleration boundary

Own motion/steering math and deterministic neighbor consumption over immutable Spatial/Fields input. Propose next-state outputs; do not build occupied trees, mutate ECS or adopt visual LOD as simulation behavior. Navigation is a separately assigned research role.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners,
read/write sets, lifetimes, bounded progress and cross-project handoffs.

Phase 2 delivered bounded separation/radial steering and transactional next-state
scratch; [integrated evidence](../integration/phase2-validation.md) records wiring.
See [rule](design.md), [handoff](validation.md) and [cpp-review](review.md).

## Scope and first task

Steering and next-state integration. Prerequisites: W2/W3.

Extend fixed integration with specified steering math and scratch outputs.

## Exclusive ownership

- [Headers](../../../include/crucible/swarm/README.md): include/crucible/swarm/.
- [Sources/build manifest](../../../src/swarm/CMakeLists.txt): src/swarm/.
- [Tests/registration](../../../tests/swarm/CMakeLists.txt): tests/swarm/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Swarm. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Independent reference math, symmetry, zero distance, finite state and two scales.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.
