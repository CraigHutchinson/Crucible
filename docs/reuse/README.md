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
| R01: ECS storage and identity | Sub0ECS v2, consumed by Simulation | Feed bounded creation/identity and consumer integration findings into Sub0ECS; keep game entities/rules in Crucible | Pinned audit records 24-bit indices, generation reuse and lack of transactional creation guarantee; upstream changes need current-source reproduction and strict consumer checks |
| R02: typed delivery | Sub0Pub v2, stack round-trip consumer | Improve callback lifetime examples and bounded ingress integration when a real input adapter arrives | Preserve explicit disconnect/drain ownership; the pinned audit is evidence, not a claim that every finding remains at current HEAD |
| R03: execution and joining | Sub0Pipeline, sequential stack consumer | Feed concrete bounded executor/join/cancellation requirements upstream during W7; retain game phase policy locally | Exact-version join/failure fixtures before changing pins; no new concurrency work in phase 2 |
| R04: compact observation | Sub0Log, stack decoded-record consumer | Use consumed runtime summaries to improve logging adapters, teardown examples and round-trip tests | Phase 2 summary first; logger adapter follows an actual caller and bounded storage/lifecycle evidence |
| R05: hex topology and geometry | [Sub0HexGrid groundwork](Sub0HexGrid.md), standalone kernel initialized; Crucible adoption pending | Reuse checked geometry, then add bounded regions/candidates; phase 2 reviews topology before spatial expansion | Kernel numeric/installed-package fixtures pass; complete radius candidates and integrated Crucible query/replay evidence required before a pin/migration |
| R06: spatial bins and radius traversal | Crucible::Spatial, integrated rectangular implementation | Keep entity binning local for now; identify whether a geometry library can support it without owning SampleId/ECS or gameplay | Compare complete queries to brute force for the chosen topology; second demonstrated consumer before extracting a generic binning library |
| R07: bounded input/trace/state exchange | Crucible::Runtime and planned owned read model | Record patterns that may improve Pub/Pipeline or justify later extraction; avoid a generic runtime framework | Concrete consumers and bounded lifetime/capacity tests; phase 2 only adds the owned copy it uses, not an unconsumed exchange |
| R08: steering, Blight and resource rules | Crucible domain modules | Keep gameplay policy local; feed any general storage/math defect upstream with a minimal reproduction | Product-neutral boundary and independent reusable consumer required before extraction; game tuning remains in Crucible |
| R09: paging/cache foundations | Sub0MemPage/Sub0TieredCache, future evaluation | Reuse existing libraries if a measured world/data residency requirement appears; simulation can supply real workloads and feedback | No current Crucible consumer or demonstrated residency bottleneck; postpone integration until measured need |
| R10: surface height and terrain transitions | [Future world direction](../decisions/terrain-and-world-extension.md) | Preserve planar height/deformation and later spherical geometry; keep mining/material/fusion policy in Crucible until a reusable boundary has real consumers | Finite height/storage, traversal and material conservation fixtures; sourced spherical adjacency/metric specification before code |

Status vocabulary: **consumed** means an actual caller exists, **proposed** means a
design candidate, **upstream finding** requires exact-version evidence, and **deferred**
means the consumer/need is not yet demonstrated. Mark extracted only after the
standalone project and Crucible consumer both pass their gates.

## How an increment feeds reusable libraries

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
