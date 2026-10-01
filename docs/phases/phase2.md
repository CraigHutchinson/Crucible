# Phase 2: make the sequential simulation steerable and inspectable

Status: delivered; see [handoff and evidence](../workstreams/integration/phase2-validation.md). Date: 2026-10-01. Dispatch baseline: main 65bb8c4; see
[execution contracts](../decisions/phase2-execution.md). Source baseline: phase 1 head
`722e7b3`, merged at `ab708a7` through [PR 1](https://github.com/CraigHutchinson/Crucible/pull/1).
Dispatch from the verified current main commit and record that SHA in each claim.

## Intended increment

Advance from independent module prototypes to a sequential swarm scenario whose
movement has a bounded local rule, whose wall-clock driver has explicit lifecycle
behavior, and whose completed state can be inspected without ECS borrows. Preserve
exact headless tick replay. This is useful groundwork for the playable loop and a
small visual presentation; it does not implement the full relay mission.

Must-have delivery is a deterministic reference scenario with local separation and
radial field steering, bounded clock driving and owned state inspection. Stretch is
one actual-state visual export showing the swarm, fields and Blight at selected ticks.
Generated concept art remains design exploration; a real-state export shows what
the simulation actually did.

## Workstream review for this phase

Use two implementation workers plus the architect. Three active ownership streams
replace phase 1's four dispatch streams. Module folders remain unchanged.

| Stream / owner | Consolidated scope | Exclusive paths / shared requests | Result and consumer |
|---|---|---|---|
| A: swarm behavior / worker | W5 first increment, with existing W3 grid/fields as inputs | swarm/ code/tests/docs; spatial/ changes only if architect explicitly transfers the claim | Bounded sequential separation + radial force rule, reusable scratch and independent reference fixtures; Simulation is the caller |
| B: runtime and observation / worker | W2 clock/lifecycle + the minimum consumed W8a summary | runtime/ and telemetry/ code/tests/docs | Clock driver, pause/resume/close behavior and bounded tick summary; headless driver/main consumes them |
| C: composition and state inspection / architect | W1 consumed state values, W8b owned-copy first increment, integration/review; W9 export only as stretch | contracts/, presentation/, Simulation/main, integration tests, central docs and root wiring | Owned completed-state read model, stronger replay oracle, combined scenario and validation; headless state inspection consumes the copy |

Integration/Contracts stays with the architect because the next contract is small
and coupled to actual consumers. Spatial/Fields needs no separate worker until A
demonstrates a missing input. Blight's existing spread rule is retained as an input;
resources and interactions are deferred. Runtime and telemetry share one owner to
avoid inventing a summary no driver consumes. Presentation groundwork stays with the
architect until its state contract is useful; a full graphics stream is not justified yet.

## Contract gate before parallel edits

Review the likely hexagonal spatial direction and [Sub0HexGrid proposal](../reuse/Sub0HexGrid.md)
before widening spatial contracts. Architect/stream A records a bounded topology ADR:
chosen layout/boundaries/query semantics, geometry reuse boundary and migration scope.
Keep Blight adjacency a separate gameplay choice. The verified rectangular baseline
continues if the decision/extraction is unresolved; clock and inspection work can proceed.
Sub0HexGrid's standalone kernel was initialized in its user-designated repository;
region/candidate completeness and actual migration remain gated. No dual-backend
implementation or terrain/spherical work is required for this phase.

Architect and affected owners agree the dense input/output shape, stable SampleId
ordering, borrowed lifetimes, scenario limits and summary/read-model fields. Name
each consumer before adding an API. Simulation gathers immutable tick-start values;
workers' domain code computes exclusive next-state scratch. Only the coordinator
accesses ECS. No per-tick capacity growth, world pointers or borrows in published values.

Separate three notions explicitly: live session state, a completed-state copy and
the accepted command trace. Clock diagnostics are not replay commands. SampleId
remains sufficient for the fixed population; it is not promoted to a destruction-safe
entity identity. A later resource/structural phase must revisit generation semantics.

## A: bounded local steering

Freeze a small project-defined rule before authoring code: separation from neighbors
inside a finite radius, existing radial field acceleration, finite total acceleration
and speed limits, then fixed-step integration into next-state scratch. Define self
exclusion, coincident-neighbor behavior, world clipping and stable reduction order.
Alignment/cohesion and painted flow are subsequent increments, not hidden requirements
of this one. Do not implement a recalled external flocking algorithm without its source.

The grid's borrowed result expires at its next query. Consume each result before
querying again. Never use immutable SampleId as an array index without an explicit
validated mapping. All steering reads the same tick-start state so entity traversal
order cannot change its input. Rebuild committed-position diagnostics after integration.

Acceptance: independent brute-force rule oracle, isolated/pair/coincident/dense/border
cases, input-order agreement, finite bounded outputs, scratch reuse and integrated
fixtures at tiny and 2048-entity scales. Compare movement through owned state rather
than aggregate checksum alone. No performance claim or full W5 completion.

## B: bounded clock and consumed observation

Keep exact-tick HeadlessSession as the authority. Add a driver that accepts elapsed
time explicitly; use a steady clock only at the outer caller. Start at the existing
60 Hz with at most four catch-up ticks per pump. Specify fractional remainder,
excess-time discard/counter, negative/overflow elapsed input, pause/resume baseline
and close behavior. Do not fabricate clock progress for a rejected/failed boundary.
Return a blocked/stopped result so trace exhaustion cannot create a busy retry loop.

Pause discards elapsed play time; resume must not run accumulated paused-time ticks.
Every actual tick still captures its own command cutoff. Clock-independent replay
keeps its existing exact boundary semantics. Define these policies in the runtime
decision record with controlled elapsed-time fixtures before wiring a wall-clock loop.

Publish only a small summary consumed by main/state inspection: completed tick,
applied-command count, ingress rejection/pending statistics and clock discarded time.
Use owned values and bounded counters/storage. A Log adapter is optional only after
the summary caller exists and its startup/flush/teardown is demonstrably bounded;
do not force Pub/Pipeline adapters into this phase merely to fill the backlog.

Acceptance: zero/fractional/exact elapsed intervals, catch-up cap, discard, pause,
resume, late cutoff arrival, closed/full/failed session behavior and equivalent final
state for admitted traces despite different wall-time schedules. No sleeping timing
tests, background ECS query, executor concurrency or complete W2/W8a claim.

## C: owned state and integration

Add the minimum owned completed-state copy needed for inspection: stable IDs and
positions (velocity if the steering oracle consumes it), bounded field values and
Blight display cells plus geometry and tick identity. Specify destination capacity
and failure before mutation; an insufficient destination must not expose a partial
frame as a complete one. Populate pre-sized destination storage without exporting ECS
or module scratch spans. Do not add resource/structure state that does not yet exist.

Use the copy now in a headless inspection/replay fixture: compare ordered identities,
positions and infection cells for matching traces; checksum is only an extra diagnostic.
Simulation/shared wiring is architect-owned, so worker handoffs remain path-disjoint.
Retain the original ECS-only constructor/workload and scenario compatibility coverage.

This increment is a single-coordinator owned copy. Three-slot exchanges, reader
leases and GPU upload retirement are deferred until there is a concurrent/display
consumer. They remain W8b gates; avoid unconsumed concurrency scaffolding today.

Acceptance: empty/full state, inadequate capacity, tick identity, deterministic order,
independent copies that survive subsequent ticks, geometry consistency, no escaping
borrows and full-state replay checks. Root performs cpp-review, integration and one
combined supported Debug/Release/ASan/UBSan pass after the meaningful handoffs.

## Visual stretch: one real-state artifact

Start only after A/B/C core fixtures and review pass, with enough quota to review
and validate the export. Prefer a deterministic SVG view written from the owned
copy outside the tick: map bounds, swarm dots/density, infected cells, radial field
extents and tick label. Export a few selected ticks from the same scenario so movement
can be inspected side by side. Open/render the artifact for visual QA; publish the
files and reproduction command. One static frame is sufficient if that is all the
remaining quota supports.

This supplies a visual presentation without selecting a window/GPU stack or adding
new graphics dependencies. It is a diagnostic view, not the concept-art HUD or a
playable RTS. A real window/input/rendering path remains W9 after its backend and
snapshot lifetime decisions. If the export needs extra simulation APIs or destabilizes
the core, defer it and record why; do not reduce required validation to obtain it.

## Integration order, stop conditions and exit

1. Freeze consumed contracts and record the phase ownership review; dispatch A and B.
2. Architect implements/tests the minimal owned inspection path while workers progress.
3. Review and integrate A, then B and the combined caller; adjust ordering only with
   an explicit consumer/dependency reason. Exchange changed contracts with affected peers.
4. Run the combined gate once. Fix findings; then decide whether stretch fits the quota.
5. Record delivery/remaining gates, exact commits and results; push, verify CI, merge
   to main and confirm the merged baseline before another phase starts.

Each worker stops at a review-ready bounded handoff; it does not consume spare quota
on extra features. If quota tightens, preserve the smallest fully wired A/C movement
and inspection increment and a separately reviewable B clock increment. Leave deferred
work explicit rather than landing stubs or bypassing gates. Do not start new agent
research, heavyweight measurements or speculative abstractions during wrap-up.

Phase exit requires a useful integrated scenario, documented behavior/capacity,
reviewed maintainable code and honest validation. It does not require the stretch,
100K-scale timing, fusion/shatter, victory/defeat, a threaded executor or a playable UI.
Retrospective must reconsider these priorities and workstream consolidation before
phase 3; the eventual resource phase must freeze the game-design ledger first.
Update the [reuse catalog](../reuse/README.md) with concrete upstream findings,
extraction decisions and validated receiving callers. Strengthening reusable sub0
libraries is a project outcome; proposed changes are not recorded as delivered work.
