# Hierarchy and acceleration: application responsibility map

This supplements every Crucible stream without selecting a structure, implementing
a module, migrating bins or changing pins. Sub0HexGrid's
[upstream map](https://github.com/CraigHutchinson/Sub0HexGrid/blob/main/docs/workstreams/responsibilities.md)
defines geometry/interchange ownership; its
[programme](https://github.com/CraigHutchinson/Sub0HexGrid/blob/main/docs/phases/hierarchy-research.md)
defines competing private spikes before measured architecture selection. Crucible
owns game-use contracts; an upstream experiment cannot redefine them.
These are current assignments. A later measured product-neutral extraction requires
an explicit ADR and updated upstream/application ownership, not implicit transfer
from a successful private spike.

## Accountable owners

| Stream | Hierarchy/acceleration responsibility | Handoff / exclusion |
|---|---|---|
| Integration W0 | Simulation/ECS gather, exact-pin audit, tick/structural commit and coherent publication; appoints navigation researcher | Frozen epoch and copied values; coordinates owners without absorbing domain semantics |
| Contracts W1 | Consumed shared identity/snapshot/request/result values, capacities, failures and compatibility | Separates epoch, snapshot row, SampleId and ECS handle; no tree, reducer or queue algorithms |
| Runtime W2 | Admission, clock/replay/pause, scheduling policy and bounded catch-up | Partial work cannot advance a completed tick; no index algorithms or executor joins |
| Spatial W3 | Owned bins/occupied nodes, bounds cache, base occupancy/count summaries, rebuild/refit/dirty propagation and exact-query frontier | Reviewed geometry, coherent leaves+summaries, bounded node AND leaf work; no camera, route costs, domain reducer policy or ECS pointers |
| Fields W3 | Painted/radial field meaning, units, edits/sampling and domain reduction/invalidation | Immutable observations for Swarm; computed navigation flow is not automatically Fields-owned |
| Blight W4 | Cellular state, spread adjacency, resources and domain summary meaning | Hierarchy/view LOD does not alter spread or infer uniform infection from occupancy |
| Swarm W5 | Steering/next-state math, ordered neighbor consumption and movement/capability requirements | Reads immutable Spatial/Fields input and proposes outputs; no index build, camera or ECS mutation |
| Interactions W6 | Resource arbitration, exact identity validation and bounded consumption/fusion/shatter proposals | Refines summary candidates against authoritative state; commits through W0 |
| Scheduling W7 | Executor adaptation, access DAG, batching/partitions and CPU/device completion/failure joins | Executes W2 policy; no freshness, motion/navigation or LOD policy |
| Telemetry W8a | Bounded versioned observations/counters and drop policy | Owned records only; no logging-thread ECS/tree traversal, mutation or benchmark verdict |
| Presentation W8b/W9 | Owned snapshots, mini-map/zoom caches, visual reducers/error/freshness, picking/camera transforms and graphics uploads | Exact leaf picking; commands through W2; no authoritative index updates or simulation LOD |
| Validation W10 | Independent fixtures, comparison protocol and full consumer evidence | Reports build/update/query/order/render/path quality and memory/latency; does not invent gameplay tolerances or infer FPS from microbenchmarks |

Production paths remain those in the briefs. W0 owns shared Simulation, pins, root
wiring and central docs; W1 owns shared values. Affected owners review through W0
patch requests. An experiment assignment does not transfer another module's files.

## Navigation and device roles

Navigation is a phase-assigned research role accountable to W0, not an existing
production target. It defines fine connectivity, costs/clearance, portals/routes,
shared-goal fields, repair and route-quality evidence with W1/Swarm and the eventual
terrain owner. HH-09 must name a production home before a public service is added.
H grouping is not routing; occupancy is not passability; player-painted vectors are
not assumed to be computed integration fields. A consumed terrain phase must assign
its own owner; this supplement creates none.

A named external adapter owns native kernels, device allocation, mirrored buffers,
transfers and completion lifetime. W7 integrates joins/access sets; W2 owns scheduling
policy; W9 owns graphics/view upload lifetime. Sub0HexGrid X supplies consumer-gated
interchange/parity. Sub0ECS supplies storage/identity/executor guarantees at the actual
pin, not a spatial hierarchy or an implied CUDA/Vulkan backend.

## Summaries, snapshots and bounded work

Spatial owns base occupancy/count index data and caches approved conservative bounds.
Domain owners define mass/field/Blight/resource reducers, units and dirty conditions.
Presentation owns copied view caches and visual approximation; consuming a summary
does not grant gameplay authority. W1 owns value contracts, not reducer algorithms.
Separate use-case structures can share snapshot identity; one hierarchy is not mandated.

W0 freezes a tick for yielded gathering or supplies owned immutable input. W3 builds
pending leaves/base summaries; domain workers write only assigned pending outputs.
W0 publishes a coherent generation after required jobs finish. Presentation reads
owned copied/leased snapshots, not live ECS rows. Versions detect replacement, not
storage lifetime. W2/W0 define cancellation/lease and completed-tick policy; W7
enforces joins. Modules retain their own cursor/capacity/failure state; clocks remain
in Runtime. Budget node visits AND leaf entities, gather, sorting and reductions.
This documentation enables no concurrent execution.

Later spawn/destroy or retained-result ECS writes must define liveness/reuse/exhaustion
and structural barriers at the pin. Fixed-population SampleId does not settle that
policy. Snapshot epoch, row, application ID and ECS handle are distinct domains.

## Handoff and acceptance

Every handoff names both directions under the standing methodology: the intended
visible/interactive experience and its fidelity/feedback needs, plus verified
geometry/runtime/storage capabilities and measured limits. Reconcile them through
a received vertical slice; revise either side where costs or participant feedback
justify it. Concepts, synthetic fixtures, native graphics/input/audio and participant
evidence are distinct. Audio/background research gets an explicitly appointed W0
application role if needed; no new audio module or geometry responsibility is implied.

Current production placement remains authoritative: BoundaryPipeline and optional
RuntimeDiagnostics live in Runtime despite the W7/W8a role boundaries. Their files
remain Runtime-owned; extracting them requires an explicit phase handoff. Current
Presentation includes Camera2D/FieldTool/ScenePainter and an optional GPU receiver.
Hierarchy-specific mini-map/zoom cache and native spatial compute remain proposed;
this map does not replace the current application evidence with the earlier headless baseline.

Sub0HexGrid I owns standalone comparison fixtures/registration in its repository;
W10 owns Crucible consumer extensions and scenario/replay/render/navigation evidence.
Integrators agree shared input/counter semantics; one coordinator reserves each
timed run. There is no joint edit ownership of the same harness or result artifact.

Each assignment names one path owner/caller and defining contract owner, input/output
domains, read/write sets, borrow/lease expiry, generation, publication/completion,
capacity/failure, order/numeric/quality and work units. Cross-stream changes go through
W0/W1 and affected owners. Declared private variants may differ for experiments but
cannot silently duplicate/redefine production geometry or addressing.

Motion retains query/replay parity; views need explicit quality/marker/freshness and
real visual evidence; navigation needs legal routes and declared cost optimality.
W10 reports full-cycle costs, memory, latency tails and limitations per consumer.
Timed runs are serialized across projects. No hierarchy outcome or new module is
claimed. Self-review covers W0-W10 and upstream T/G/R/Q/H/X/I; actual interfaces
still freeze in the phase before dispatch.

## Phase14 proposed scale assignment

[Phase14](../phases/phase14.md) introduces a proposed parallel-compute axis, not a
hierarchy or navigation selection. This is the required all-stream responsibility
audit before implementation dispatch. Existing production remains sequential.
G0a freezes values/query/scratch/observation, G0b receives executor/backend adoption;
each exact path/branch/base and resource claim is recorded at dispatch. A and B
are proposed session owners, root is the sole shared-file/contract integrator.

| Stream | Decision / edit owner | Reads -> writes and defining responsibility | Lifetime / handoff gate |
|---|---|---|---|
| W0 Integration | Retain / root | ECS -> owned gather; validated staged rows -> serial ECS/resource commit; shared root wiring/pins/CI | No worker ECS pointers; every partition/callback joins before commit/capture; G1/G2/G7 |
| W1 Contracts | Retain / root | Consumed scenario/execution/observation value representation and failure/numeric policy | No native/library implementation leaks; root freeze plus affected A/B callers, G0a |
| W2 Runtime | Retain / root | Input cutoff/clock -> resolved execution policy injected into Simulation; inline outer graph -> coherent owned publication | No Runtime dependency in Core/Scheduling; restart replaces only after joins; G1/G2 |
| W3 Spatial | Consolidate with A | Immutable epoch/index -> complete sorted caller-owned query results | A owns spatial include/src/tests/local manifests/docs; per-partition startup scratch, no rebuild until join; G1 |
| W3 Fields | Retain sequential / root shared requests only | Committed edits -> immutable sampling for A | No field mutation during partition epoch; math and force rules unchanged, G1 |
| W4 Blight | Retain sequential / root shared requests only | Committed infection -> prepared spread/protection | Existing exclusive lease and joint resource commit, no parallel domain mutation; G1 |
| W5 Swarm | Consolidate with A | Complete gathered input/Spatial/Fields -> disjoint pending row proposals | A owns swarm compartments; arithmetic/FP/order parity, no destination publication on partial failure; G1/G2 |
| W6 Interactions | Retain sequential / root shared requests only | Post-move contacts -> stock/ledger/fuse/shatter proposals | Existing conservation/ordering and coordinator commit; not authority over view density, G1 |
| W7 Scheduling | Activate with A | Frozen ranges/callables -> executed proposals and joined status | A owns scheduling compartments; exact Pipeline queue/dispatch/body+callback guarantees; root owns upstream pin/wiring, G0b/G2 |
| W8a Telemetry | Retain / root | Copied numeric stage/capacity observations -> bounded records/cold decode | Existing RuntimeDiagnostics stays Runtime-owned; no logging worker ECS/index traversal, measured observation cost/loss |
| W8b Snapshots | Consolidate with B | Committed Simulation -> startup-capacity owned frames | B owns ScenarioSnapshot paths/tests; root outer publication. Borrow expires next capture/restart; no CPU reader overlap; G1/G6 |
| W9 Presentation | Consolidate with B | Owned frame/camera -> world/overlays/HUD/native submissions; view-only density | B owns presentation compartments; root DesktopApp/window/native wiring. Copy before GPU return; fence-retire bounded slots, G0b/G3/G6 |
| W10 Validation | Consolidate with B; root final verdict | Root-defined copied observations -> raw timelines/schema/capture | B owns only phase14-named harness/scripts/evidence; root existing scripts/manifests and all CPU/GPU reservations. Independent fullstate/query oracles, G1-G7 |

Navigation/terrain/hierarchy/native simulation compute are deferred; no folder or
API is created for them. No ownership transfers to upstream geometry. No simulation
LOD is permitted. Sub0Pipeline owns neutral executor guarantees; A supplies exact
source/repro and a separately claimed upstream package, root promotes pins only
after independent upstream receiving. The same agent cannot edit a cached dependency.

Top-down receiving is a readable moving100K/150K front under actual field control;
bottom-up receiving is complete queries, explicit storage/join bounds and whole
frame costs. Actual influenced population/concentration and view quality are
checkpointed before timed acceptance. Root owns shared contracts and artifact
inventories; affected briefs refer to this audit instead of creating duplicate
numeric policy. Agent concurrency never grants concurrent ECS, Pub or Log access.
