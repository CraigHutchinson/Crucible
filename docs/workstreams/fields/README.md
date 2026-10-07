# Fields workstream — W3

## Hierarchy and acceleration boundary

Own player-painted/radial field units, sampling, edits and domain reduction/invalidation. Consume Spatial addressing as needed; computed route integration/flow fields require an explicitly assigned navigation owner and are not automatically this module.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners,
read/write sets, lifetimes, bounded progress and cross-project handoffs.

Radial attractor/repulsor slots are integrated into scenario movement and validated.
Straight painted FLOW is delivered in Phase8 with owned endpoints and replay.
Computed navigation flows remain separate research. See
[FLOW receiving](phase8-flow.md) and [initial evidence](../integration/wave1-validation.md).

## Scope and first task

Painted flow and attractor/repulsor sampling. Prerequisites: W1; integration W2.

Specify bounded edits and immutable sample inputs with runtime owner.

## Exclusive ownership

- [Headers](../../../include/crucible/fields/README.md): include/crucible/fields/.
- [Sources/build manifest](../../../src/fields/CMakeLists.txt): src/fields/.
- [Tests/registration](../../../tests/fields/CMakeLists.txt): tests/fields/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Fields. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Analytic fixtures, edge behavior, capacity and finite-value rejection.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.
