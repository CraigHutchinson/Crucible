# Spatial workstream — W3

Reserved build boundary (INTERFACE target); algorithms and public APIs are pending.

## Scope and first task

Stable cell bins and complete radius traversal. Prerequisites: W1; integration W2.

Define capacities and build a brute-force neighbor oracle.

## Exclusive ownership

- [Headers](../../../include/crucible/spatial/README.md): include/crucible/spatial/.
- [Sources/build manifest](../../../src/spatial/CMakeLists.txt): src/spatial/.
- [Tests/registration](../../../tests/spatial/CMakeLists.txt): tests/spatial/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Spatial. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Coincident/dense/border cases, stable order, no silent truncation, reused scratch.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.
