# Presentation workstream — W8b/W9

Phase14 proposed assignment: workerB owns ScenarioSnapshot capacities, scalable
world/overlay/HUD drawing, view-only density quality and phase14 frame capture.
Root owns DesktopApp/window wiring. Measure current native SDL first; a live GPU
adapter is conditional required core only after G0b, not offscreen promotion.
See [scale contracts](../../decisions/phase14-scale-contracts.md) and the
[all-stream audit](../hierarchy-boundaries.md#phase14-proposed-scale-assignment).

## Hierarchy and acceleration boundary

Own copied/leased view snapshots, mini-map/zoom caches, visual reducers/quality/freshness, camera transforms, exact picking fallback and graphics upload lifetime. Read authoritative state and emit commands through Runtime; do not update simulation indexes or navigation state.

See the [application responsibility map](../hierarchy-boundaries.md) for defining owners,
read/write sets, lifetimes, bounded progress and cross-project handoffs.

Phase 2 provides startup-sized owned snapshots consumed by main, replay checks and SVG export.
Phase 3 extends owned frames with row-major stock and an optional conserved ledger.
See [phase 3 evidence](../integration/phase3-validation.md).
See [design and lifetimes](design.md) and [combined evidence](../integration/phase2-validation.md).

## Scope and first task

Owned snapshots, drawing and input mapping. Prerequisites: W2/W8a; rendering W6/W8b.

Retain sequential capture now; add leases only with a concurrent reader. Record a backend ADR before graphics dependencies.

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
control dependency visibility, not tick scheduling. The owned copy is implemented; the phase 4 concrete SDL painter and portable camera/tools now consume it. Concurrent exchange remains deferred.

## Acceptance and handoff

Slow readers, supersession, full pool, upload release, close and platform visual evidence.

Run the preset build, stream CTest label and combined suite when integrated. Report
commands/results, platforms, changed paths/SHAs, production wiring, shared patch
requests and unmet prerequisites. Follow [the task brief](../../work-breakdown.md).
Completion requires both a real consumer and acceptance evidence.

Phase 4 providers: [camera/input](camera-input.md), [drawing](drawing.md),
[combined validation](../integration/phase4-validation.md), and
[rendering decision](../../decisions/phase4-rendering.md). ScenePainter stays in the
optional `Crucible::ScenePainter` target; Camera2D/FieldTool stay in portable
`Crucible::Presentation`.
