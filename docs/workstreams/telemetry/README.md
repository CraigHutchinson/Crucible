# Telemetry workstream — W8a

Reserved build boundary (INTERFACE target); algorithms and public APIs are pending.

## Scope and first task

Bounded phase observation and Log adapter. Prerequisites: W1.

Specify summaries/drop counters; never query ECS from logging threads.

## Exclusive ownership

- [Headers](../../../include/crucible/telemetry/README.md): include/crucible/telemetry/.
- [Sources/build manifest](../../../src/telemetry/CMakeLists.txt): src/telemetry/.
- [Tests/registration](../../../tests/telemetry/CMakeLists.txt): tests/telemetry/.
- This docs folder; add design.md, decisions.md and validation.md when evidence exists.

Claim these areas in [ACTIVE_WORK_LOG](../../ACTIVE_WORK_LOG.md) before editing.
Shared Simulation/query declarations, root wiring, pins, CI and another stream's
contracts require an integrator patch request. See [the stream guide](../README.md)
for explicit source registration and local test commands.

## Consumer and boundary

Target: Crucible::Telemetry. The [architecture](../../architecture.md) names its
production consumer and input/output, capacity and lifetime contracts. Build links
control dependency visibility, not tick scheduling. A reserved target is not an
implemented capability. Add no stub-success API or worker execution without gates.

## Acceptance and handoff

Decoded tick/phase records, disabled path, full capacity and logger lifetime.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.
