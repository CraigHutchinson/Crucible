# Phase14 scale contracts and architect review

2026-10-07; review finalized 2026-10-08. Design proposal for [Phase14](../phases/phase14.md), not a frozen ABI,
implementation result or performance receipt. G0 freezes only consumed signatures
after the bounded probes. Production home and semantic rules below are selected.

## Findings driving decomposition

| ID | Actual source/evidence | Consequence and disposition |
|---|---|---|
| P14-R01 | `spatial/Grid.hpp` owns one m_Results; TryQuery borrow expires on next query/rebuild | Parallel calls race. A owns a const immutable-index query seam plus caller-owned startup scratch; retain complete sorted result and current sequential convenience caller |
| P14-R02 | `swarm/Steering.hpp` owns one output buffer and promises whole-destination preservation | Do not call current TryCompute on sliced inputs. A factors the existing row math over complete input and disjoint pending ranges; root publishes only after all validation/joins |
| P14-R03 | Exact CPM Pipeline bf2ecce priority pool has fixed workers but priority_queue dispatch storage; inFlight increment precedes potentially throwing push | Worker count alone does not bound storage. This is an inspected risk, not a reproduced current-HEAD bug. Receive dispatch allocation/failure behavior first; scoped upstream repair/bounded queue if needed, no copied executor |
| P14-R04 | Current BoundaryPipeline uses runInline; all jobs access coordinator-owned session/frame/mission | Keep outer graph inline. A's synchronous inner partition graph executes only pure row proposals; never move coordinator graph/Log/Pub/ECS onto worker pool |
| P14-R05 | InspectorSession fixes64x32 and DesktopApp2K/quota; Populate repeats positions modulo cell count | Define full evolving scale scenario/settings. Same-arena high density and declared density-preserving larger world are separate workload classes; record active ticks to reject terminal-frame FPS |
| P14-R06 | OffscreenRenderer is fixed1280x720 world/readback, no window/HUD; SDL desktop is a separate renderer | Measure complete native SDL path first. One concrete live GPU world/HUD/present consumer only if required; offscreen speed cannot receive HW-05 |
| P14-R07 | ScenePainter creates sample quads/indices with minimum marker half2px | Overview may become an opaque carpet. B freezes view-only culling/density quality and reports authoritative versus rendered/aggregated counts; exact picking and simulation never use visual approximation |
| P14-R08 | Architecture prints older ECS/Pipeline audit hashes; Phase13 records still say PR29 publication pending | Reconcile current main/pins and exact-head receipt without closing unreceived audio/native/participant gates |

Sources were inspected read-only, including exact cached Pipeline source. Current
Pipeline pin is `bf2ecce80545d2fcef94e73f2da6ed425153ba46`; ECS is
`1b1114ad2a15569ce106da429e94435912144f4f`. Other full pins stay authoritative in
cmake/DependencyPins.cmake. Current sibling HEAD is not automatically the pin.

## C1 immutable epoch and query storage

G0a freeze, 2026-10-08, from Pub checkpoint `3f85c82`: add const
`Grid::tryQuery(center, radius, callerScratch)` returning an optional borrowed
complete sorted span; retain exclusive `TryQuery` through owned scratch. Scratch
must fit all committed samples and be disjoint from index and other active tasks.
Add const `Steering::tryComputeRows(fullInput, fields, constGrid, firstRow,
pendingRows, queryScratch)` returning checked success. Range length is the pending
span size; fullInput remains complete, pending spans are disjoint. Invalid ranges,
nonfinite input and insufficient scratch reject before writes; an unexpected
membership failure may leave staging partial and must never publish. Existing
TryCompute invokes shared row math into its startup staging and retains its whole
destination/overlap contract. A owns Spatial/Swarm source/tests/local manifests.

Root freezes one platform-free `ScenarioSettings` value carrying population,
grid, field capacity and steering/resource/optional structural settings; Inspector
owns optional mission policy and derives all frame capacities from validated grid.
Existing session/desktop constructors forward through settings to preserve2K
behavior. Scale launches100K/400x250 or150K/500x300 use no mission cutoff. Desktop
tool radius/magnitude are startup settings shared by native input and scripted
routes; restart reuses exactly the same scenario. Initial distribution retains
deterministic cell-centered placement for the first density probe; no extra seed
surface is added without a consumer. Root owns every shared settings/runtime/app edit.

WorkerB initially owns scalable painter validation/culling/counts and a concrete
D3D11 completion observer for a production native probe. GPU-completed rendering
and presentation-call evidence remain distinct: inspection found pinned SDL's
wrapper can hide backend present failure. G0b must receive a checked handoff path
or retain that gate blocked; completion cannot certify successful present/scanout.
No backend adoption is implied. B sends DesktopApp and root-manifest patches.

Integration owns gathered ascending-ID values, Fields observation and committed
Spatial index. No ECS pointers escape gather. A defines the Spatial query contract:
immutable index reads during one coordinator-frozen epoch, each task borrowing its
own startup result scratch until that task's next query; no rebuild until all tasks
and callbacks finish. Radius filtering is inclusive, complete and ascending-ID,
including coincidence/self; no silent truncation. Inadequate scratch reports failure
before authoritative publication. Bounds include sample capacity, bin storage and
query capacity per partition; report memory cost proportional to worker count.

Steering reads the entire immutable input for neighbor ID lookup and writes only
its assigned nonoverlapping pending range. Use the same arithmetic/order as the
sequential oracle. Fields sampling is read-only. Root validates all range coverage
and results before serial ECS value commit. Preserve the existing ordering:
structural boundary commands apply before gather/movement; reclamation/protection
apply after movement; completed tick/hold/frame publication follows successful
resource commit. All remain coordinator-owned. G0 enumerates every TryQuery/TryCompute consumer,
tests and snapshot/GPU caller before changing surfaces.

## C2 scheduling and failure joins

Next-wave design freeze, 2026-10-08 (implementation waits for the corrected
upstream merge/pin): Contracts carries startup `ExecutionSettings` with workers
and partitions. Runtime resolves partitions0 to1 for workers1 and twice workers
otherwise; ordinary default remains workers1 until actual G5 evidence supports
an AUTO promotion. Scheduling consumes only resolved positive counts and startup
sample capacity. One coordinator worker uses the same startup graph inline;
selected multiple workers use an exclusively owned PriorityExecutor directly,
never a per-tick ScopedExecutor. Its queue capacity equals the partition count.

`RowPartitions` owns one untimed startup graph, fixed range/result metadata and
one startup-owned row callable. Actual mobile rows are balanced into contiguous
disjoint ranges by quotient/remainder, exactly covering the complete input.
Zero rows succeed without dispatch; an empty-capacity scenario retains its valid
single empty partition. Counts/representability and empty callable reject before
launch. The constructor primes graph caches through an explicitly inactive
no-op epoch, not a tryRun0 path that skips the graph. No real row callback executes
at startup. Simulation owns complete query scratch per partition: memory is
P*N*sizeof(SampleId), plus fixed task/range metadata. Whole-input validation per
partition remains part of measured cost; no unchecked query API is implied.

Run outcomes distinguish complete, invalid rows, unsupported floating-point mode
and failed work. Capture the coordinator environment per run; pool jobs install
and restore worker fenv and received x86 rounding/FTZ/DAZ controls before returning
to Pipeline completion. Inline execution preserves the coordinator's ordinary
exception-flag effects. A mode that cannot be received produces no row callback
for that partition; only a successfully joined graph whose sole limitation is
unsupported FP may request sequential recomputation before ECS commit. Submission,
callback or graph failures dominate that status and retain existing fail-stop
behavior. Never disguise rejection as a floating-point fallback. All accepted
bodies, completion callbacks and callable targets finish before any return.
G1 receives standard rounding modes and supported denormal controls/restoration;
G2 receives startup/first/warm storage, partial failure joins, reuse and destruction.
No public borrowed-executor overload or test-only production knob is introduced.

Contracts owns the consumed startup execution value representation; Runtime
resolves policy (sequential, selected bounded workers, chunk count and measured
auto policy) and injects it into Simulation. Core must not include/link Runtime:
Runtime already depends on Core. Scheduling implements the consumed adapter and
depends on Pipeline/value contracts, never Core or Runtime policy.
Simulation owns its execution context and calls it synchronously from the
coordinator. Worker count, submitted partition count, queue capacity and all scratch
are fixed/validated at startup. Prefer a small multiple of workers, never one job
per entity. No pool worker submits then waits recursively on the same pool.

Receive bodies AND completion callbacks before returning, publishing or destroying
input/context. No timed jobs/orphans in this slice. Dispatch rejection/exception or
row failure joins every already-started partition, leaves destination unpublished
and triggers existing fail-stop/last-good-frame policy. Commands previously applied
at that boundary are not rolled back; no whole-tick transaction claim is added.
Close/restart cannot replace the run while partitions or callbacks still borrow it.

Finite storage/work counts are bounds; noncooperative OS work does not gain a hard
wall-clock bound from a timeout. External test-process deadlines report a failed
receiving run and cannot license releasing live storage. Verify exact pinned
PriorityExecutor dispatch, completion, shutdown and failure with independent fixtures
before enabling its consumer option. Missing neutral guarantees go to Sub0Pipeline,
with root pin promotion only after upstream merge and exact-version game receiving.
Startup failure preserves the previous restart run. No private threadpool clone.

## C3 frame publication and graphics

The coordinator publishes a complete owned ScenarioSnapshot after tick success.
B owns snapshot capacity edits, root owns publication wiring. Drawing stays on the
desktop coordinator; CPU simulation partitions join before drawing. The snapshot
borrow expires on next successful capture/restart/destruction. No asynchronous CPU
renderer retains that borrow. GPU submissions copy packet/camera data into owned,
bounded slot resources before returning; three slots are the starting received
design, with capacity/generation/fence/drain preserved for any live adapter.

All-busy means counted visual backpressure: do not overwrite in-flight data or
pretend an old frame is newly simulated. Record skipped/repeated/displayed tick age.
Input, world overlay and HUD use the same camera/layout/run generation and confirmed
tick; UI previews are explicitly pending. Camera-only redraw may reuse a retained
packet but cannot masquerade as an evolving frame in the performance sample.
Generation checks detect stale receipts, not storage lifetime.

View density/culling has an independent quality decision before implementation:
overview shows front/motion/field direction, close zoom shows local individuals;
exact object picking resolves authoritative copied samples. Record submitted sample
count, visible individuals, aggregated marks, hidden samples, cells and HUD separately.
No simulation LOD, approximate geometry or neighbor cap enters this decision.

## C4 observation and workload policy

G0a receiving refinement, 2026-10-08: the short native probes expose simulation
as the dominant cost. InspectorSession owns startup-bounded integrated
`TickObservation` storage, disabled at capacity0. One successful StepGraph call
records run/tick, applied-command count and its complete boundary interval, from
before command service through snapshot/mission publication. Never derive tick
percentiles by dividing a multi-tick pump duration. Records append an immutable
prefix; counted drops block acceptance, successful restart resets storage/identity,
and failed or paused boundaries add no successful row. Positive capacity on the
direct comparator is rejected because it captures once per pump, not per boundary.

Optional Simulation phase clocks are a separate explicit attribution arm:
gather/sort, index rebuild, proposals, ECS commit, resources and final spatial
rebuild, plus actual row/query/storage counts. Disabled phase clocks return absent
statistics. Baseline and candidate acceptance arms use identical outer observation
capacity and leave phase clocks disabled; stage instrumentation overhead must be
qualified independently. The observed PR29 control preserves its Simulation,
Spatial and Swarm source bytes and rejects unsupported stage attribution. These
copied values add no worker/storage borrow or Core-to-Runtime dependency.

Root defines numeric copied observations; B owns the capture schema/harness. No
logging thread traverses Simulation/Spatial or stores borrowed spans. Hot observations
use startup-bounded storage with counted drops; output/JSON serialization is cold.
Frame timeline carries source/run/frame identity, completed ticks, applied command
sequence, active/terminal state, stage begin/end, GPU submission/completion identity,
presentation-call event and backpressure. Report dropped observations; insufficient
correlation cannot establish latency or a full-frame pass.

Baseline and candidate receive identical scenario and input traces at their exact
source trees. Worker1/2/N and backend combinations preserve world semantics and
visual quality. Freeze density/occupancy and geometry explicitly, plus memory/task
capacity and permitted workload timeout. Hardware inventory and native renderer
selection remain separate. Physical scanout/input-to-photon is not inferred from
SDL_RenderPresent return or an offscreen fence.

## Proposed artifact/consumer audit

| Proposed capability/home | Named consumer | Dependency direction |
|---|---|---|
| Spatial immutable query/caller scratch | Swarm ordered row kernel, sequential oracle | Spatial depends on Contracts/H2, never Scheduling or Presentation |
| Swarm row compute/staging | Simulation via Scheduling partitions,1-worker comparator | Swarm reads Spatial/Fields/Contracts, no ECS/platform |
| Concrete Scheduling adapter and partition execution | Simulation tick, synchronous coordinator | Scheduling depends on received Pipeline and consumed value/callable contracts, never Runtime policy |
| Scale startup settings/commands where actually needed | InspectorSession, Simulation, DesktopApp and capture harness | Root Contracts remain platform-free; preserve default small scenario |
| Scalable painter or selected live GPU adapter | DesktopApp world/overlays/HUD/present | Presentation reads owned snapshots; desktop owns native device/window |
| Full-frame observations/capture | Production DesktopApp and benchmark exporter | Numeric bounded producer observations, cold standalone export; no generic metrics framework |

No signatures, public wrappers, stubs or build options are added by this proposal.
cpp-write/sub0 profile applies when writing C++; cpp-review plan mode checks L0
consumer wiring, L1 acyclic ownership and L2 explicit storage/failure boundaries.
Style/L3 and runtime acceptance are separate later gates.

## Review disposition and open freeze gates

Two independent read-only reviews supplied the shared scratch, density, early
terminal, renderer and executor risks. Architect incorporated each into the plan;
R01-R08 are resolved in decomposition, with their implementation receiving gates
still open. Wave0 is dispatchable preparation after fresh claims; parallel production
execution and backend adoption remain blocked by G0/C1-C4 until exact interfaces,
numeric/visual oracles, budgets and device scope are checkpointed. G0a freezes
safe sequential scenario/query/schema preparation; G0b receives actual executor
and backend/completion observation after complete sequential SDL probes. Do not
block the baseline probe on the backend choice the probe exists to inform.

Human reviewer receives product fit/overview quality and reference-hardware budget
choices. Target feasibility, safe native lifecycle, subtle concurrency and actual
performance need independent execution evidence; this plan review establishes none.

Final closure review corrected the capture package's G0a probe prerequisite and
explicitly preserved pre-movement structural commands. Consumer review closed its
four measurement/workload findings; the final front-control criterion also requires
attribution against a no-command reference. Foundation review confirmed its six
original corrections, then flagged the two ordering/prerequisite clarifications
above before its final turn was interrupted by a usage limit. Root checked and
applied those clarifications; no additional independent clean-pass claim is made.
