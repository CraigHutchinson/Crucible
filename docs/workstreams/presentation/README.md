# Presentation workstream — W8b/W9

Reserved build boundary (INTERFACE target); algorithms and public APIs are pending.

## Scope and first task

Owned snapshots, drawing and input mapping. Prerequisites: W2/W8a; rendering W6/W8b.

Implement snapshot leases first; record backend ADR before graphics dependencies.

## Exclusive ownership

- [Headers](../../../include/crucible/presentation/README.md): include/crucible/presentation/.
- [Sources/build manifest](../../../src/presentation/CMakeLists.txt): src/presentation/.
- [Tests/registration](../../../tests/presentation/CMakeLists.txt): tests/presentation/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Presentation. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Slow readers, supersession, full pool, upload release, close and platform visual evidence.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.
