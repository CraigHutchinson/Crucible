# Reuse and upstream improvement catalog

Crucible is a proving ground for a stronger set of reusable sub0 libraries as well
as a game. Application work should produce useful library feedback: clearer contracts,
reproductions, tests, integration examples and controlled performance evidence.
Extraction is one route; improving an existing project is often the better route.

Review this catalog at every phase start and close. A candidate is not a commitment
to create a repository, introduce a dependency or move working code immediately.
Current consumption follows [full pins](../../cmake/DependencyPins.cmake), not sibling
HEADs. The [phase 1 audit](../workstreams/integration/wave1-pinned-audit.md) records
the guarantees actually checked. No upstream edits were made during this catalog work.

## Catalog

| ID / area | Current placement/status | Reuse direction and phase action | Gate / evidence |
|---|---|---|---|
| R01: ECS storage and identity | Sub0ECS master (v2 merged), consumed by Simulation | Feed bounded creation/identity and consumer integration findings into Sub0ECS; keep game entities/rules in Crucible | Phase 1 audit records 24-bit indices, generation reuse and lack of transactional creation guarantee; the current pin bounds creation at `Entity::kMaxEntities` (terminating) and Simulation refuses larger populations first; upstream changes need current-source reproduction and strict consumer checks |
| R02: typed delivery | Sub0Pub main (v2 merged), production scoped synchronous input | Improve callback lifetime examples and bounded ingress integration when a real input adapter arrives | Preserve explicit unsubscribe/drain ownership; the pinned audit is evidence, not a claim that every finding remains at current HEAD |
| R03: execution and joining | Sub0Pipeline, production sequential boundary graph | Feed concrete bounded executor/join/cancellation requirements upstream during W7; retain game phase policy locally | Exact-version join/failure fixtures before changing pins; no new concurrency work in phase 2 |
| R04: compact observation | Sub0Log, optional bounded RuntimeDiagnostics | Numeric refused commands and terminal summaries; existing upstream API suffices | Phase12 receives decode, exhaustion/drop accounting, scoped binding, restart and full-state parity |
| R05: hex topology and geometry | [Sub0HexGrid H2](Sub0HexGrid.md), finite regions/complete candidates delivered; consumed by Crucible spatial Grid | Phase 3 P3-02 preserves physical/query compatibility and delivers HX-07 receiving fixtures; hierarchy is separate research | Upstream PR 4 delivers package/numerical evidence; [Phase 3 evidence](../workstreams/integration/phase3-validation.md) records complete-query/lifetime/replay promotion gates |
| R06: spatial bins and radius traversal | Crucible::Spatial, integrated hex geometry with private rectangular compatibility | Keep entity binning local for now; identify whether a geometry library can support it without owning SampleId/ECS or gameplay | Compare complete queries to brute force for the chosen topology; second demonstrated consumer before extracting a generic binning library |
| R07: bounded input/trace/state exchange | Crucible::Runtime and planned owned read model | Record patterns that may improve Pub/Pipeline or justify later extraction; avoid a generic runtime framework | Concrete consumers and bounded lifetime/capacity tests; phase 2 only adds the owned copy it uses, not an unconsumed exchange |
| R08: steering, Blight and resource rules | Crucible domain modules | Keep gameplay policy local; feed any general storage/math defect upstream with a minimal reproduction | Product-neutral boundary and independent reusable consumer required before extraction; game tuning remains in Crucible |
| R09: paging/cache foundations | Sub0MemPage/Sub0TieredCache, future evaluation | Reuse existing libraries if a measured world/data residency requirement appears; simulation can supply real workloads and feedback | No current Crucible consumer or demonstrated residency bottleneck; postpone integration until measured need |
| R11: finite viewport geometry | Crucible Camera2D and painter clipping | Retain local; independent camera/clipping package is a candidate | Second editor/viewer consumer, finite/edge/extreme fixtures, no SDL/ECS/game coupling |
| R12: build capability tooling | Python prerequisite and CTest runners | Retain local; small shared Python package is a candidate | Second CMake repository, bounded probes/process ownership, configured-toolchain and failure receipts |
| R10: surface height and terrain transitions | [Future world direction](../decisions/terrain-and-world-extension.md) | Preserve planar height/deformation and later spherical geometry; keep mining/material/fusion policy in Crucible until a reusable boundary has real consumers | Finite height/storage, traversal and material conservation fixtures; sourced spherical adjacency/metric specification before code |

Status vocabulary: **consumed** means an actual caller exists, **proposed** means a
design candidate, **upstream finding** requires exact-version evidence, and **deferred**
means the consumer/need is not yet demonstrated. Mark extracted only after the
standalone project and Crucible consumer both pass their gates.

## How an increment feeds reusable libraries

Phase14's selected [scale proposal](../phases/phase14.md) receives R03 bounded
executor dispatch/joins with an actual Simulation caller and R06 immutable complete
queries with a Swarm caller. Improve Pipeline only from exact-source fixtures and
independent package plus game receiving; do not duplicate its executor locally.
Domain/view policy stays local, H2 geometry stays upstream, R09 caches remain
unconsumed. This is a planned disposition, not new pins or measured improvement.

For a candidate or upstream finding record: originating phase/commit, domain behavior,
product-neutral capability, named receiving consumer, existing project alternatives,
ownership/lifetime/capacity/failure contract and current evidence. Include the smallest
reproduction or fixture and the reason local ownership is or is not sufficient.
On closure, add the upstream issue/PR/commit and the Crucible pin/consumer validation.
Do not equate a proposed optimization with a measured improvement.

Choose one disposition: retain local, reuse an existing library, improve upstream,
or propose extraction. Domain naming and policy stay out of library APIs. Use one
source of truth after extraction; avoid maintaining copied implementations in both
projects. A library must be independently buildable/testable with target-scoped
requirements, explicit consumer/package validation and documentation of supported
platforms. Follow its own contribution/review workflow before editing upstream.

## Phase 2 decision

Hexagonal spatial topology is a likely direction raised by the user. Its standalone
kernel has been initialized in the user-designated repository. Treat adoption R05 as
an early design gate; reserve the minimal bounded investigation in the existing
architect/stream A ownership instead of adding another permanent agent. The current
rectangular grid remains the verified baseline until a replacement passes its oracle.
Do not duplicate all grid/steering work for two backends or promise a hex migration
within the phase's quota. A recorded specification/decision can be a useful increment
even if library implementation is scheduled next.

## Phase 2 disposition and feedback

Base 65bb8c4; delivery in [phase 2 evidence](../workstreams/integration/phase2-validation.md).
R05 is retained as a standalone H1 library; H2 regions/radius candidates remain the
next reusable increment. No geometry copy, second backend, new pin or upstream edit
was required. R06 stays local: stable-ID queries now have a real steering consumer
checked against an independent all-pairs oracle. R07 stays local: startup-owned state
copy and bounded clock are consumed by main/replay/SVG; no generic exchange is extracted.
R04 remains deferred because summary values do not require a logger adapter. R08's
separation rule is game policy, so it is deliberately retained in Crucible. R01's fixed
population and 24-bit guard remain relevant before any future structural resource work.
No measurement or upstream optimization is claimed from the functional fixtures.

## Phase 3 disposition

R05 is consumed at aaae5c2fc5731a23db94fa947bbb0182d0ea69fd. Normal-domain
assignment/candidates reuse H2; IDs, bin storage and exact point filtering remain
R06 local policy. Private startup/domain/environment compatibility and exact-scan
query fallback preserve the previous contract. The benefit is shared checked geometry
and reproducible receiving evidence, with no speedup claim. R08 finite reclamation
remains local game policy. R07 owned frames now carry stock and ledger without a
concurrent exchange. See [delivery and HX-07 handoff](../workstreams/integration/phase3-validation.md).
No upstream source edit or hierarchy selection was made.

## Phase 4 disposition

R07 owned frame values now have live drawing/event consumers. Keep Camera2D,
InspectorSession and FieldTool local: they encode the current fixed world/tool
contract. SDL3 supplies the concrete platform/2D adapter with a pinned full revision;
no new generic renderer interface or extracted framework is justified. The software
pixel fixture and native-window event smoke exercise different boundaries. No
upstream code defect or measured optimization was found; HX-07 and the H2 pin stay
unchanged. A future GPU upload must establish resource retirement and a real
concurrent reader before adding frame leases/exchange.

## Phase 5 disposition

R07/R08 remain local: objective policy consumes an owned ledger at each completed
boundary; presentation consumes an owned mission value alongside the snapshot.
The clock's optional startup-owned stop callback has an actual mission caller, not
a generic scheduling framework. SDL3/H2 pins and receiving evidence remain intact.
[Instancing design](../decisions/render-instancing.md) carries the CPU packet
experiment into explicit shader/upload/retirement gates. No upstream defect or
controlled speedup was found, and no extraction or pin update is justified.

## Phase 6 close assessment

Pinned SDL3 supplies the executing Vulkan receiver; H2 and all sub0 pins remain
unchanged. Owned normalized records, weak submission identity and bounded slot
retirement remain local presentation integration. R07/R08 do not acquire a generic
exchange or graphics framework. No demonstrated storage/residency bottleneck or
product-neutral upstream defect justifies a new dependency or extraction this phase.
See [receiving evidence](../workstreams/integration/phase6-validation.md); physical
receivers and fault injection are follow-up gates, not upstream library claims.

## Phase 7 close assessment

Fault receiving consumes the unchanged production SDL GPU archive through local
Linux fixtures; no generic call table/backend framework is extracted. The mission
study consumes coordinator/route/painter without seed APIs or rule changes. R07/R08
remain local and all SDL/H2/sub0 pins unchanged. No exact-version upstream defect,
residency need or second product-neutral consumer warrants extraction. See Phase 7
receiving for controlled failure/timing limits and the distinct physical/human gates.

## Phase 9 investigation disposition

The relay-density consumer reuses production Simulation, HeadlessSession,
ScenarioSnapshot and existing H2-backed radius queries, with an independent
owned-state brute-force oracle. Structural and arena ledger/authority policy remains
R08 local. R07 needs a concrete command union only when fuse/shatter has a real
consumer; no speculative envelope/exchange is added by this investigation.
R01 retains startup identities and avoids unverified ECS allocation promises.
No upstream defect, pin change, second generic consumer or controlled optimization
justifies extraction. [Selection](../decisions/phase9-selection.md) carries the next
consumed gates; experiment results do not claim a library speedup.

## Phase 9 full-source reuse review

The [sub0 adoption audit](phase9-sub0-adoption.md) distinguishes actual production
and test-only callers, confirms full pins and prioritizes receiving opportunities.
BUILD_TESTING=OFF now omits acquisition of Pub/Pipeline/Log; their existing stack
integration remains tested. ECS/H2 and optional SDL stay production dependencies.

The [extraction review](phase9-extraction-review.md) ranks bounded input handoff
(R07, preferably an upstream Pub companion), finite viewport geometry (R11), and
build capability tooling (R12). Keep all three local until a named independent
consumer, minimal neutral contract, license/package decision and receiving fixtures
justify promotion. Camera/game/GPU policy must not enter HexGrid. No new repository,
pin or speculative adapter is introduced this sprint.

At structural dispatch, reuse fixed ECS identities and existing complete queries;
centralize the consumed full-state comparison as structural fields are added.
Optional Log diagnostics follow a real bounded-record caller. Pub delivery and a
Pipeline graph require an actual delivery/dependency need and explicit teardown/join
receiving; library adoption alone is not a reason to add either wrapper.

The [owned-library roadmap](sub0-roadmap.md) makes long-term sub0 evolution and
upstream requirement/pin-promotion ownership explicit.


## Phase 11 disposition

R02/R03 now have real desktop, route and export callers. Pub scoped delivery copies
through the existing queue; Pipeline orders commit/capture/mission under one
coordinator. Current APIs suffice, full pins retained after exact-source comparison.
R07 receipt/mission/cutoff policy remains local application glue; no umbrella or
bounded broker/executor copy. R01 fixed identities remain received; dynamic creation
and recycling retain upstream capacity/provenance/generation/failure gates. R04
bounded diagnostics and measured R03 run allocation budgets are received in Phase12.
Warmed integrated pumps allocate3 times/96 requested bytes; admission and diagnostic
refusal emission allocate zero replaceable C++ objects. Retain exact pins and existing graph:
no demonstrated need for new storage APIs. No extraction or measured speedup is claimed.
