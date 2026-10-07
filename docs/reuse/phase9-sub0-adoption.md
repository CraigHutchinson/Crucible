# Phase 9 library adoption audit

2026-10-05. Read-only receiving review of application sources, target links,
`cmake/DependencyPins.cmake`, `cmake/Dependencies.cmake`, the fetched Release
sources under `build/release/_deps/*-src`, and the reuse catalog. No library API,
pin, benchmark, upstream issue or extraction was changed. Fetched HEAD inspection
matched all five sub0 full pins below; this does not assert that upstream HEAD has
the same implementation or that fetched working trees were independently pristine.

## Actual adoption

| Library / exact pin | Actual caller and boundary | Disposition |
|---|---|---|
| Sub0ECS `8391f81fd74a016564b4711b074eb286d3c5e14b` | `include/crucible/simulation.hpp`, `src/simulation.cpp`: concrete Worlds, declared queries, startup creation and sequential iteration; linked by Core | Production-consumed. Stable application SampleIds and owned captures protect callers from row ordering. Proposed structural mode can retain existing ECS identities rather than require growth/destruction |
| Sub0HexGrid `aaae5c2fc5731a23db94fa947bbb0182d0ea69fd` | `src/spatial/Grid.cpp`: PointyLayout, AxialRegion and CandidateCells; Spatial links its public target; simulation steering/radius inspection and relay spike consume those queries | Production-consumed. H2 geometry is reused; bin storage, exact distance tests and sample ordering remain local. Cardinal Blight adjacency is a separate rule |
| Sub0Pipeline `f6f54c623908649e8daac3613545062cf08b3822` | Only `tests/integration/stack.cpp`: sequential integrate→telemetry graph; `tests/integration/CMakeLists.txt` links core and Headless | Stack-test-only. Runtime ticks directly; no production DAG, worker dispatch or joining adapter. Reserved Scheduling target is not adoption |
| Sub0Pub `2cd3daf15e44c9429fd2cfbc4ca2fcb723b77ced` | Only `tests/integration/stack.cpp`: synchronous TickCommand publish sets a local inbox flag before the graph runs | Stack-test-only. Desktop/headless directly use owned CommandIngress. No production subscriber, concurrent lifetime receiver or bounded Pub adapter |
| Sub0Log `85498bb7b735f8c7e43653955be582ee903c653f` | Only `tests/integration/stack.cpp`: file-backed Logger, ScopedBind, one emitted checksum record and independent SegmentReader/Decoder after writer teardown | Stack-test-only. Production summaries/JSON are not a Sub0Log adapter; Telemetry is reserved |
| Sub0MemPage / no pin | No dependency declaration, fetched source, include, link or caller found | Deferred. No verified API review or demonstrated paging need in this repository |
| Sub0TieredCache / no pin | No dependency declaration, fetched source, include, link or caller found | Deferred. Current bounded arena does not establish world residency, cache tiers or streaming demand |

The stack test is useful integration evidence, not an engine assembled from all
five runtime libraries. Label R02/R03/R04 explicitly **test-only** until a shipped
caller receives their behavior. ECS's upstream target requests C++20, while Crucible
requires C++23 through `crucible_options`; this is compatible minimum-feature
metadata, not a source/pin mismatch. All declared upstream test/example/benchmark
options relevant to these packages are disabled; no unrelated dependency tool
suite is pulled into the application's acceptance run.

Other reuse is concrete: SDL3 (pin `829a65d769d935c4852f8159e964312c0957260a`)
provides optional native input/software drawing and SDL GPU receiver in
`src/desktop/`, `src/presentation/desktop/`, `src/presentation/gpu/OffscreenRenderer.cpp`
and `cmake/DesktopDependencies.cmake`. Standard spans, vectors, algorithms, mutexes,
chrono and RAII already serve ownership and bounded startup storage. Matplotlib
serves the offline evidence plot; it is not a new game dependency. No second
renderer, generic task framework or numerical dependency is justified by Phase 9.

## Debt and source mismatches

1. **Test-only dependency acquisition: cleanup received locally.**
   Before this review `cmake/Dependencies.cmake` unconditionally fetched
   Pipeline/Pub/Log even when BUILD_TESTING=OFF; none is linked to the production
   executable graph. They were EXCLUDE_FROM_ALL, so this was acquisition/configuration
   surface, not evidence that their archives bloated the shipped binary. Root now
   gates those packages on BUILD_TESTING and guards Pipeline sanitizer target
   iteration with if(TARGET). The changed source was inspected. A fresh test-off Release build passed, its mission/route replay won at tick267
   with conservation and full-state equality, and the three test-only source
   directories were absent. Test-off sanitizer configuration also passed using
   the pinned ECS/H2 sources; this is not a sanitizer runtime receipt. The reconfigured test-on Release build passed and stack_round_trip passed1/1.
   Exact-head hosted CI remains the publication gate.
2. **Stale implemented-module summaries: corrected and inspected.**
   `include/crucible/{spatial,fields,interactions}/README.md` and
   `src/{spatial,fields,interactions}/README.md` previously called implemented
   packages INTERFACE reservations with pending algorithms. Their CMake files build
   explicit STATIC sources. Root corrected all six summaries during this review:
   Spatial now describes H2-backed stable-ID complete queries and fallbacks; Fields
   describes bounded radial/FLOW slots; Interactions describes finite stock-to-reserve
   reclamation with structural fusion/loss remaining a follow-up. These current
   texts were inspected against actual source/callers.
3. **Unconsumed reservations.** `src/scheduling/CMakeLists.txt` and
   `src/telemetry/CMakeLists.txt` really are source-free INTERFACE targets; no code
   links their aliases. They emit no dummy objects and expose no invented APIs.
   Keep their status explicit or remove their inert wiring during a focused cleanup;
   filling them with wrapper classes just to claim adoption creates avoidable debt.
4. **No unjustified architecture extraction identified.** The concrete workstream
   helper only establishes target/source ownership. Clock's optional stop callback
   has a mission caller. GPU packet/slot state has an optional receiver and lifetime
   fixtures. SDL's dummy backend is a test capability, not a fake production API.
   Prepared infection, snapshots, fields and queues own real used state; preserve
   these consumed boundaries rather than replace them with speculative base classes.

## Prioritized next adoption increments

| Priority | Small consumed increment | Required receiving evidence / stop criterion |
|---|---|---|
| 1: honest dependency boundary | Gate stack-only packages and correct module/catalog status | Test-on/off clean configure/build; stack round trip; sanitizer target guard. Pins and gameplay unchanged |
| 2: Sub0Log diagnostics | Optional production summary/rejection diagnostic consumer outside the ordinary HUD; existing pinned Logger already supports createInMemory and counted drops | Owned startup-aligned backing storage, ScopedBind lifetime, decoded known records and deliberate exhaustion with Stats loss count. Summary cadence explicit; diagnostic failure must not change commands, ledger or replay. Use existing logger instead of a new binary format |
| 3: Sub0Pub receiving adapter | Only when a second independent input producer actually needs typed delivery: concrete subscriber copies complete payload into existing bounded ingress | Explicit post-construction trySubscribe, normal full/closed responses, disconnect while derived members remain alive, producer join and concurrent source-mutation/lifetime fixture. Pub does not replace the queue or its boundary cutoff; skip adapter if direct ingress remains sufficient |
| 4: Sub0Pipeline sequential consumer | At a real need for declared task dependencies, first receive one production untimed sequential phase graph with owned tick inputs/outputs | Compare complete scenario state/replay with direct baseline; explicit resource access/dependencies; dispatch/body failure and join fixtures. Parallel execution waits for valid partitions, race/lifetime evidence and measured end-to-end benefit; adding Pipeline around one tick alone is not an improvement |
| Deferred: paging/cache | Measure a concrete terrain/world working set before evaluating MemPage/TieredCache | Obtain exact source/pins, inspect APIs, show residency/eviction lifetime and budget requirements plus an actual receiver. Current 2,048-cell fixture is not that need |

These are ordered opportunities, not promised new libraries or concurrent sprint
scope. Complete the structural gameplay contract before scheduling generalization.
The selected fixed-identity lattice should reuse existing ECS, complete spatial
queries, owned commands and snapshots; it does not need a new allocator dependency.

## Product-neutral upstream findings

Exact local source reconfirms the earlier [pinned audit](../workstreams/integration/wave1-pinned-audit.md).
The following are source findings/receiving proposals, not newly executed repros or
reports filed against today's upstream branches:

- **ECS capacity and liveness:** pinned `include/sub0ecs/entity.hpp` masks the
  index to 24 bits and compares index/version without occupied-state validation.
  A tiny isolated repro can create/release one Entity, fabricate its now-current
  version and observe alive; separately document/version-wrap ABA. This is not a
  legal Crucible operation, which retains private startup identities and bounds
  creation. Before future growth, propose checked capacity/handle provenance and
  receive the upstream fix on exact versions; never infer it from SampleId.
- **ECS reservation/rollback:** pinned `store/world.hpp::reserve` reserves records
  only. `create` performs allocator, partition/row and component operations with
  no visible transaction guard. A bounded fault-injection fixture must fail each
  allocating step and inspect world consistency before promising transactional or
  allocation-free creation. The fixed-identity proposal avoids depending on it.
- **Pipeline completion and timeout:** pinned IExecutor requires wait_all to include
  completion callbacks; runImpl calls it on the normal path. Timed noncooperative
  work can outlive run and requires join_orphans before borrowed captures expire.
  Upstream feedback should carry a minimal owned-buffer/callback failure/timeout
  fixture, not general claims that the executor makes ECS mutation safe.
- **Pub callback ownership:** pinned broker invokes synchronous callbacks outside
  its lock; the subscriber source requires derived registration/disconnection for
  concurrency and warns against mutual cross-disconnect waits. A receiving example
  that copies into a bounded sink is more useful than duplicating the broker.
- **HexGrid:** current production query falls back to exact scan outside its supported
  geometry/rounding path. Existing `tests/spatial/GridHexTests.cpp` and relay query
  parity are real consumer evidence. Remove compatibility only after equivalent
  extreme/rounding/capacity fixtures; no new H2 defect or hierarchy requirement was
  established. Keep any general geometry improvement in Sub0HexGrid.
- **Log:** the pinned createInMemory and Stats APIs already address bounded backing
  storage and visible drops. Receive them in the consumer before requesting a new
  storage abstraction. Existing one-record file decoding does not prove bounded
  production pressure or concurrent logging behavior.

## Potential reusable base packages

| Candidate boundary / existing code | Reuse destination and reason | Gate before a standalone fork/extraction |
|---|---|---|
| Bounded owned command journal: `runtime/CommandIngress`, `HeadlessSession` | Strongest candidate; share capacity/cutoff/sequence/storage patterns as Pub/Pipeline receiving examples first | Independent product receiver, payload-neutral contract, normal application rejection versus fatal failure, bounded trace exhaustion, teardown and full replay. Keep field/mission rules local; desktop+CLI are callers in one product, not independent adoption |
| Stable-ID spatial bins: `spatial/Grid` | Extend existing Sub0HexGrid only for geometry; entity-ID bin indexing could later form a separate small package | Second real consumer, caller-selected ID type, exact radius/capacity/borrow contract and controlled workload evidence. Do not move ECS or game adjacency into the geometry library |
| GPU submission retirement: private `presentation/gpu/SlotSchedule` and receiver lifetime | Small possible receiver-neutral value-state utility; SDL ownership/upload remains concrete | Second executing backend, nonwrapping identities, completion/abandon/device failure tests. Current three-slot graphics policy alone does not warrant a renderer framework |
| Capability/test preflight: `scripts/check_prerequisites.py`, `scripts/run_tests.py` | Potential shared development tooling for owned projects | Independent repository consumer, explicit capability schema and bounded subprocess behavior. Preserve project commands/labels separately; not a C++ runtime dependency |

Do not extract biomass, faction relations, relay rules, force tuning, mission clocks,
HUD commands or material palettes. Forking is justified by a consumed neutral
boundary and demonstrated maintenance need, not source-file size or a desire to
populate the family. No new standalone package is recommended for this sprint.
