# Crucible architecture

[Game design](game-design.md) owns product intent; this document owns implemented
boundaries and the gates for changing them. [Concepts](concepts/README.md) guide
material and shape readability. Art does not define resource rules or select a backend.

Dispatch baseline is PR22 merge `fc9a4689992e0f803faf07d4315ddb1bf8d78ce1`.
[Phase10](phases/phase10.md) extends the sequential owned command loop with fixed
mobile/anchored/lost participation, one relay lattice, protection and hold. No ECS
allocation/destruction occurs after startup. FLOW remains a straight capsule current.
Factions, threaded gameplay, population growth and terrain remain future work.

## Ownership and consumers

| Boundary | Implemented owner and consumer | Constraint |
|---|---|---|
| Simulation | ECS state, stable sample IDs, fields, Blight, spatial bins and scratch; consumed by Runtime and headless scenarios | One exclusive coordinator; no platform, Pub callback or Log-global dependency |
| Contracts | Grid/settings, BoundaryCommand, identities/activity, ledger and state-copy values | Standard C++ value types; add fields only with a receiving caller |
| Spatial / Fields / Swarm | Pinned H2 geometry and local bins; field sampling; bounded separation/steering | Immutable tick input, startup-sized output; complete-query and independent numerical oracles |
| Blight / Interactions | Cardinal current/next infection buffers; deterministic stock-to-reserve arbitration | Stock is independent of infection; atomic resource publication, not whole-tick rollback |
| HeadlessSession | Borrowed Simulation, bounded ingress, completed command trace | Simulation outlives session; only ingress supports producer threads |
| InspectorSession | Owns Simulation/session/clock and a reusable ScenarioSnapshot | Restart constructs a full replacement first; failed construction preserves the old run |
| Presentation | Camera2D/FieldTool and owned copied frames; concrete ScenePainter | No ECS views in drawing; no generic renderer hierarchy |
| Optional GPU receiver | Owned InstancePacket, camera uniforms, three fence-retired slots and readback receipts | Explicit lifetime/generation checks; no default desktop promotion |
| Scheduling / Telemetry | Sequential stack integration and reserved future adapters | Folder/INTERFACE targets do not imply a production threaded scheduler or logger |

Core publicly exposes Contracts/ECS and project requirements; domain targets remain
private. Runtime links Core; future Pub/Pipeline/Log adapters require real callers.
Production builds with testing disabled fetch ECS/H2 and optional SDL only.
Pub/Pipeline/Log are test-only until concrete runtime adapters consume them.
Target-scoped CMake, explicit source lists and full pins keep builds and stream ownership
reviewable. No stub API, copied geometry kernel or single-implementation interface is needed.

## Boundary input and completed ticks

CommandIngress owns a mutex-protected value ring. Admission validates a complete
nonempty batch and either copies all values or rejects without consuming a sequence.
A full ring rejects newest. Closing is out-of-band; shutdown never needs queue space.
Pause stops ticks while bounded admission remains open.

At a boundary, HeadlessSession captures a sequence cutoff and drains only that
prefix. Trace capacity failure preserves the boundary. Field/fuse/shatter commands apply in sequence against committed state before
movement; earlier actions are visible to later actions. Normal structural refusals
are recorded without stopping. Completion records payload, sequence, tick and result. Unexpected
application/tick failure stops the session; already applied edits are not rolled back.
Replay prevalidates a fresh matching scenario's entire trace, then applies it at the
same boundaries. Exact checks include IDs, positions/velocities, flow endpoints,
activity, infection, every stock cell, structure settings/members/generation/hold
and all ledger diagnostics. A shared owned-state comparison is consumed by CLI/spike
and structural receiving; independent geometry/arithmetic oracles remain separate.

| Current tick order | Reads | Writes |
|---|---|---|
| Apply boundary commands | Admitted owned prefix, committed mobile positions | Fields, participation, membership and mass/loss |
| Gather / rebuild / steer | Tick-start mobile samples, fields and complete bins | Reusable sorted input and next-state scratch |
| Publish movement | Computed next position/velocity | Existing ECS component values; world edges clamp |
| Prepare cardinal spread | Committed infection | Leased next infection buffer |
| Arbitrate reclamation | Post-move contacts in cell/ID order, next infection | Pending stock/ledger and clearing; clear protected centers after harvest, then joint commit |
| Rebuild / complete / observe | Moved state and committed resources | Mobile bins, completed tick/hold/mission and owned snapshot |

The optional radial-only path precedes the bounded steering setting. Resource-disabled
scenarios retain ordinary spread; the legacy ECS-only constructor retains its isolated
integration workload. Future parallel phases must declare reads/writes, use disjoint
scratch and join before structural mutation. A DAG alone does not make ECS or Pub safe.

## Resource and structural rules

Conservation is `initial = remaining stock + mobile + reserve + structure + lost`.
`harvested` and `work_actions` are diagnostics. Existing resource-disabled and legacy
quota modes remain unchanged. The [structural rule](decisions/phase9-structural-proposal.md)
selects the lowest64 eligible IDs, anchors them at the relay and refunds the lowest48
on shatter, losing16. Empty/stale/insufficient requests do not mutate rule state.
Only mobile identities steer, query and reclaim; all identities are retained in owned
state. Protection clears pending infection after harvest; committed infection can
emit one final wave on the fusion tick. Hold advances only on completed boundaries.
Victory checks harvested quota plus120 hold before deadline/starvation defeat;
terminal outcome closes admission and restart replaces the entire run. This fixed
identity design does not imply allocator, dynamic-growth or ECS rollback guarantees.

Kind, allegiance, controller and presentation are distinct future concepts. The
[faction board](concepts/factions-deathmatch-v2.png) establishes color-plus-shape
readability, not hostility, owned stock or authority. The
[Phase 9 comparison](phases/phase9.md) precedes those consumers; avoid adding unused
faction types or a multiplayer framework. Terrain/traversal remains the
[world extension gate](decisions/terrain-and-world-extension.md).

## Observation, graphics and lifecycle

ScenarioSnapshot allocates once and captures owned values only under coordinator
exclusivity. A borrow expires on the next successful capture/restart/destruction;
rejected capture preserves the retained frame. There is no concurrent snapshot
exchange. Add leases or a buffer pool only when a delayed concurrent consumer needs
them and proves slow-reader/backpressure/teardown behavior.

The desktop painter batches cell/sample geometry and clips world overlays before
narrowing to SDL raster coordinates. FieldTool distinguishes uncommitted previews,
admitted edits and refusal. The optional SDL_GPU Vulkan receiver consumes normalized
copied records, retains camera-independent packets and publishes only completed
readback pixels. Controlled software-device faults and retirement have fixtures;
physical loss/performance and a second backend remain separate gates. See
[rendering](decisions/phase4-rendering.md), [GPU decision](decisions/phase6-gpu.md) and
[Phase 7 review](sprint-reviews/phase-07.md).

Teardown closes admission, stops boundaries and joins producers/readers before
releasing borrowed storage. Future Pub callbacks must disconnect/drain while ingress
still exists; future executor jobs must join before their captures die. GPU resources
remain until fence/drain policy permits release. World replacement needs the same
quiescence. No silent continuation after an unexpected tick failure.

## Pinned guarantees and acceptance gates

- ECS `8391f81fd74a016564b4711b074eb286d3c5e14b`: trivially copyable components
  up to 64 bytes, 64 component types and 32 exact declared queries. IDs use a 24-bit
  index domain; do not assume transactional creation. Keep grids and large state outside components.
- Pipeline `f6f54c623908649e8daac3613545062cf08b3822`: construct graphs on one
  thread, drain callbacks and join orphaned timed work before releasing borrows.
  Current gameplay is sequential; the thread-per-job and pool executors are disabled.
- Sub0HexGrid H2 supplies checked assignment/conservative candidates. Crucible owns
  IDs, bins and exact filtering, with private rectangular and exact-scan fallbacks
  for unsupported geometry/radius domains. Blight still uses cardinal adjacency.
- Pub lifetime/threaded delivery needs exact-version receiving before production use.
  [Pinned audit](workstreams/integration/wave1-pinned-audit.md) and the
  [reuse catalog](reuse/README.md) record what has actually been checked.

Run supported Debug, Release and ASan/UBSan through `scripts/run_tests.py` with
capability preflight. Use independent numerical/ledger oracles, full-state replay,
capacity/rejection and retained-state/lifetime fixtures. Threaded work adds race
coverage or an explicit limitation. Physical GPU, iOS and human tests require
[capability-matched receiving](workstreams/integration/hardware-receiving-backlog.md).

The 100K–150K / 60 FPS target needs full tick and upload/presentation-inclusive frame
measurements under the [benchmark protocol](benchmarking.md). Isolated ECS timing,
a rendered image and a software Vulkan pass establish different facts.

## Longer-term owned-library integration

The [sub0 roadmap](reuse/sub0-roadmap.md) assigns upstream requirements and receiving
ownership. Pins are reproducibility checkpoints; missing neutral capabilities should
be specified and evolved upstream before new game behavior depends on them. ECS
owns capacity/identity/creation guarantees, HexGrid geometry, Pub delivery lifetime,
Pipeline dispatch/join, and Log bounded observation. Runtime retains command cutoff
and domain rejection; simulation retains biomass/faction rules. MemPage/TieredCache
await measured backing/residency consumers. Introduce a consumed adapter once, with
package fixtures, explicit failure/lifetime contracts and exact-pin game receiving.
