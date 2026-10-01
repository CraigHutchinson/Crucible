# Contracts workstream — W1

Groundwork: shared Position/Velocity values and fixed tick interval exist; scenario/command contracts are pending.

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
