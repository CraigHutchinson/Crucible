# Interactions workstream — W6

## Hierarchy and acceleration boundary

Own exact resource/identity validation, arbitration and bounded proposals. Spatial summaries can locate candidates but cannot replace authoritative checks; structural/resource commits remain coordinated by Integration.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners, read/write sets, lifetimes, bounded progress and cross-project handoffs.

Finite reclamation provider (STATIC target); fusion/shatter remain pending.

## Scope and first task

Consumption arbitration and fusion/shatter proposals. Prerequisites: W3/W4/W5.

Specify resource accounting and proposal limits; audit commit failure guarantees.

## Exclusive ownership

- [Headers](../../../include/crucible/interactions/README.md): include/crucible/interactions/.
- [Sources/build manifest](../../../src/interactions/CMakeLists.txt): src/interactions/.
- [Tests/registration](../../../tests/interactions/CMakeLists.txt): tests/interactions/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Interactions. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Competition, overlap, stale IDs, exhausted capacity, exact resource/event order.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.

## First consumed provider

`Reclamation` now supplies bounded stock-to-reserve arbitration and staged cardinal
Blight publication. See [design](design.md) and [handoff](validation.md). Fusion/shatter
and complete W6 remain deferred; architect-owned Simulation wiring and executable gates
must accompany this provider before merge.
