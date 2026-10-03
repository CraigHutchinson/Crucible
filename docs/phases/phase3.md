# Phase 3: finite reclamation and a verified hex receiving contract

Status: architecture preparation started; implementation has not been dispatched.
Date: 2026-10-03. Architect plus two cooperative workers maximum. This replaces
the resource-plus-upstream-H2 proposal: H2 has shipped, so do not repeat its work.

## Baselines and outcome

Crucible inspected main: `7692049a2c3b596f35ab27d159efdc215b52f84e`.
Sub0HexGrid inspected main: `aaae5c2fc5731a23db94fa947bbb0182d0ea69fd`.
[H2 delivery](https://github.com/CraigHutchinson/Sub0HexGrid/blob/aaae5c2fc5731a23db94fa947bbb0182d0ea69fd/docs/phases/h2-delivery.md)
and [PR 4](https://github.com/CraigHutchinson/Sub0HexGrid/pull/4) supply upstream
finite-region/candidate/package evidence. [PR 5](https://github.com/CraigHutchinson/Sub0HexGrid/pull/5)
defines competing hierarchy experiments, not a selected implementation. Both repos
had no open issues/PRs when inspected. Recheck main and claims at implementation
dispatch; use the merged planning commit as each worker's actual starting SHA.

Core exit: a reproducible headless scenario where arriving nanites reclaim finite
stock, clear depleted infection and credit an owned biomass ledger. Full-state replay
explains every transfer. Population remains fixed. This is the first W6 increment,
not spawning, attrition, fusion/shatter or the playable relay mission.

Second lane: prove a Crucible-owned hex adapter preserves its spatial query and
steering contracts. Promote only after independent query/lifetime/replay gates and
a documented consumed benefit. Otherwise retain rectangular production and publish
the receiving fixtures/finding. Core gameplay depends on neither hex nor hierarchy.

## Work division and sequencing

| Owner / package | Deliverable and receiving caller | Exclusive writable paths | Prerequisite / shared requests |
|---|---|---|---|
| Architect / P3-00 | Resource and physical/query contracts with independent tables | contracts headers/source/tests; docs/decisions; central docs | Freeze actual consumed values before dependent implementation |
| Worker A / P3-01 | Stock, stable bounded arbitration, staged Blight consumption and reserve ledger; Simulation calls it | include/src/tests under interactions and blight; docs/workstreams/interactions and blight | Resource gate; Simulation/state copy/root requests to architect |
| Worker B / P3-02 | Hex receiving proof, candidate adapter and promote/defer evidence; Steering/Simulation call it if promoted | include/src/tests under spatial; docs/workstreams/spatial | Physical/query gate; shared values/pins/root requests to architect |
| Architect / P3-03 | Tick composition, owned inspection, full-state replay and CLI evidence | Simulation/main/presentation, integration tests, root wiring/pins/CI, central docs | Integrate A on rectangular baseline, then evaluate B independently |

Consolidate Blight/interactions under A because resource publication has one owner.
Retain Spatial under B because completeness and representation are independently
testable. Consolidate steering/runtime maintenance and observation with integration.
Defer separate fields, telemetry, scheduler and rendering workers: no new consumer
requires their expansion. Upstream hierarchy research keeps its own programme.

## P3-00: contract gate

Read [game intent](../game-design.md), [phase 2 review](../sprint-reviews/phase-02.md)
and the [reference resource decision](../decisions/phase3-resource-rules.md).
The decision fixes finite integer units, source/conversion, work budgets, contention,
clearing/reinfection, conservation and failure examples. Values are architect-selected
reference behavior, not playtested balance. Freeze actual consumed C++ values and
oracle tables before A writes public APIs. Add no unused loss/structure/growth API.

For B freeze physical bounds and pointy layout: origin (0,0), circumradius
cell_size/sqrt(3), so flat-to-flat width equals the old square cell width. This is an
evaluation hypothesis, not a product-wide topology rule. Derive an outward-rounded
covering axial rectangle from the actual transform/rounding envelope, with proved
padding; merely mapping the four corners is insufficient without coverage proof.
Validate int32 bounds, uint64 counts, host conversions, C+1 and byte products before
allocation. Retain GridConfig's closed physical rectangle and cardinal Blight.

Required cpp-write/cpp-review skills were not present in this execution workspace.
This manual architecture review makes no skill-execution claim. Resolve that authoring/
review prerequisite before substantive C++, preserving the repository's existing rule.

## P3-01: finite reclamation

A implements the [frozen rules](../decisions/phase3-resource-rules.md) after the value/
fixture gate. Resource configuration is opt-in: legacy/phase-2 scenarios preserve their
current behavior. Inputs are immutable gathered samples mapped by post-move positions
to containing rectangular Blight cells, not hex bins. Simulation owns ECS; A never
touches it. Startup-sized proposals, pending cell state and ledger; no tick growth.

Prepare spread from prior committed infection, resolve proposals in cell/ID order,
then publish infection/material/ledger together. Existing Blight::Step commits immediately;
it is not staged state. Introduce only the consumed prepare/commit path needed by
resource ticks, retaining legacy Step behavior. Invalid input/capacity/overflow leaves
resource and infection state unchanged; no partial credit or stock removal.

Acceptance: partial/depleted/zero-stock/reinfected cells, empty/disabled work, contention,
shuffled IDs, per-ID/global budgets, edge mapping, invalid/duplicate IDs, nonfinite
positions, output/proposal capacity, checked totals/counter overflow and preserved
destinations/state. Verify actual-consumer allocation reuse. Run interactions/blight
checks and hand off exact commits, shared patches, commands/results and limitations.
Stop at reclamation into reserve; structural growth/attrition/fusion are subsequent gates.

## P3-02: hex receiving proof and gated promotion

Geometry comes from the inspected H2 commit; storage/IDs/exact filtering stay in
Crucible. The application-only SpatialIndex example returns rows, uses hypot and
lacks Crucible duplicate-ID/clamp/order semantics. Do not link/copy it wholesale as
a production backend. No generic backend interface or permanent dual backend.

Preserve TryRebuild's fixed capacity, nonfinite/duplicate rejection and unchanged
committed state on failure. Clamp samples and query centers to [0,width] x [0,height].
Preserve invalid query rejection, inclusive double squared-distance predicate,
radius-zero coincidence, ascending SampleId results and span expiry at next query
or rebuild. Candidate r/q order is not reduction order. IDs, rows, cells and indices
remain distinct. Each query finishes synchronously; cursors do not imply sliced ticks.

H2 rejects some unsupported arithmetic, while Crucible accepts every finite
nonnegative float radius, including FLT_MAX. If candidate construction rejects a
valid query, scan committed samples with the same predicate and reused result buffer,
then sort IDs. Never drop hits or convert a formerly valid query to nullopt. Reject
unsupported startup mapping before allocation; keep rectangular production if its
supported domain cannot be preserved. Record fallback frequency in any measurements.

Independent scan oracle must not reuse candidate/mapping helpers. Compare complete
sorted IDs on 0/tiny/2,048, shuffled, coincident, dense, outside-clamped, all corners,
hex edge/vertex/next-representable and tiny/large/max-finite-radius inputs. Independently
check covering-region uniqueness/completeness and numerical limits. Check rejection,
borrow expiry and allocation reuse. The architect supplies a pinned evaluation branch
only when needed; no mutable sibling HEAD or unrelated dependency refresh.

Integrated baseline/candidate replay compares samples, velocities, fields, infection,
material and ledger. Occupied-cell counts are representation diagnostics and legitimately
differ: assert each against its own expected layout, not equal across topologies.
Keep one production implementation at promotion; retain scan fixtures and git baseline.
Promotion requires exact result parity, lifetime/capacity proof, integrated replay and
a named benefit; a passing microbenchmark alone is insufficient. Otherwise defer.

Bounded follow-on handoff to [upstream hierarchy experiments](https://github.com/CraigHutchinson/Sub0HexGrid/blob/aaae5c2fc5731a23db94fa947bbb0182d0ea69fd/docs/research/hierarchy-experiments.md):
immutable tick/ID/position traces, movement fractions and query workloads, exact ECS
pin, ownership/publication rules and known/unknown budgets. Motion is the actual
consumer; mini-map/zoom/navigation remain separate future consumers. Do not choose
a hierarchy or implement HS-01/02/03/04, a renderer or navigation here. HX-05 tighter
candidates and HX-06 sampling stay upstream-owned.

## P3-03: composition and evidence

Integrate A first using current rectangular Spatial. Compose boundary commands,
immutable tick-start steering, next movement, prepared spread, post-move interaction
inputs, arbitration, coordinator publication and owned inspection. Atomicity covers
the resource/Blight transition; it does not promise rollback of already applied
boundary edits or all movement. Unexpected tick failure stops runtime and suppresses
successful completed-tick publication; continuing is unsupported.

Copy every cell's infection and stock plus conserved ledger entries into owned state.
Snapshot capacity rejection retains the previous frame and all destinations. Main
consumes an opt-in reference scenario with initial/final ledger and visible transfers.
Preserve legacy checksum 225000 and existing unconfigured behavior. Actual-state SVG
with the ledger is stretch after core gates; a window/input loop is deferred.

Then evaluate B on identical trajectories/resource fixtures. If B fails, record the
reproduction and retain the rectangular implementation while A ships. Full-state
replay at 0/8/2,048 populations over 60 ticks includes pauses, different elapsed
schedules, accepted traces, retained snapshots and stopped failure. Exact comparison
is within one build; no cross-compiler bitwise identity or FPS claim.

Use configure/build/test debug, release and supported sanitize presets; focused labels
for iteration, unfiltered ctest for integration. Existing 13 tests stay passing; add
meaningful behavior/lifetime fixtures. Final implementation-head CI covers Linux/
Windows Debug/Release and Linux ASan/UBSan. Worker-only compile is not integration proof.

If quota permits timing stretch, compare baseline/current on an uncontended host with
identical seeds/compiler/options and alternating repeated runs. Report gather/sort,
both rebuilds, all queries/ordering, steering, spread, consumption, snapshot extraction
and whole tick, allocation/capacity bytes, occupancy/candidate/output work and sampled
tails. H2 standalone timings do not forecast Crucible performance. No invented budget.

## Cooperation and handoff

Use isolated worktrees/build trees from the planning merge. Claim exact owned paths,
branch/base and CPU before work; no worker edits another stream or shared root files.
Architect alone owns contracts/root wiring/pins. Send precise shared patch requests.
Notify both peer and architect on contract changes and ready handoffs; cross-review
read-only. Re-freeze changed fixtures before dependent edits. Serialize heavy builds/
measurements. Handoff records base/head, paths/contracts, actual caller, shared patches,
commands/results, review findings and unresolved gates. Stop at the bounded package.

Close with combined review, exact-head CI, PR/merge/baseline verification and a completed
[phase 3 review](../sprint-reviews/phase-03.md). Architecture preparation below is not
an implementation dispatch or delivered gameplay claim.

## Carry-forward

| ID | Disposition / accountable owner | Gate |
|---|---|---|
| P01-F03 | Core P3-00/01/03; architect + A | Finite ledger, actual reclamation caller and replay; full W6 remains partial |
| P02-F01 | Upstream H2 satisfied by PR 4; downstream adoption open, B + architect | Crucible query/lifetime/replay before promotion |
| HX-07 | Receiving contract/trace handoff, B + architect | Physical bounds, identity/epochs and reproducible query fixtures |
| P01-F04 | Rendering/input deferred; presentation architect | Backend ADR and playable controls/feedback |
| P01-F05 | Structural/concurrent/scale deferred; integration/scheduling | Generation/commit/join and complete-workload evidence |
| P02-F02 | Concurrent exchange deferred; presentation | Actual concurrent reader/upload first |
| P02-F03 | Height/mining/bridges/sphere deferred; game/geometry | Material/traversability and spherical metric/adjacency gates |

After review select one next increment: reserve-to-mobile growth, first input/display
backend, or measured hierarchy consumer experiments. Do not add all three to this phase.
