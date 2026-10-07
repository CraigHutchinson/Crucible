# Phase 9 reuse and extraction review

2026-10-05. Source review of production `include/`, `src/`, scripts, rendering and
structural spikes, CMake and the [reuse catalog](README.md). This identifies
adoption opportunities, not new libraries, measured optimizations or upstream
defects. Caller counts below are distinct local integration paths, excluding unit
tests; several paths inside Crucible are not independent product consumers.

## Existing reuse first

The [sub0 adoption audit](phase9-sub0-adoption.md) records exact pins, production
ECS/HexGrid consumers, test-only Pub/Pipeline/Log and optional SDL3. Test-only
packages are now acquired only with BUILD_TESTING enabled. Standard containers,
algorithms, mutexes, chrono and callbacks remain the default for small owned seams.
No allocator, executor or cache dependency has a demonstrated missing capability.

## Product-neutral seam and extraction matrix

| Candidate and actual local callers | Neutral seam / existing alternatives | Keep local policy | Cost and promotion gate |
|---|---|---|---|
| Bounded sequenced ingress / trace: CommandIngress owned by HeadlessSession (1 owner); desktop/CLI/density feed that same path | Atomic owned batches, fixed storage, monotonic sequence, cutoff-prefix draining and out-of-band close. `std::vector`, `std::mutex`, `std::span` implement storage/synchronization. First compare a Sub0Pub extension at its actual current source; synchronous typed delivery alone is not this queue | FieldEdit validation, field capacity, completed-tick application, replay payloads, authority and future fuse rejection | Medium/high: separate payload validation from queue, freeze borrow/close/overflow behavior, preserve no-allocation boundary. Independent product needs batch+cutoff semantics; run late-producer, wrap/full, sequence exhaustion, all-or-nothing and teardown/race fixtures. Prefer upstream fit before a new repository |
| Exact fixed-step clock: InspectorSession and CLI example (2 construction paths) | Integer rational accumulation, bounded catch-up and discarded-time accounting. `std::chrono` and ordinary arithmetic remain first choice; Pipeline executes work but is not proven to provide this elapsed-time policy | 60Hz, four-tick catch-up, pause/stop/outcome and HeadlessSession summaries | Medium: tiny arithmetic kernel may be reusable; extracting the whole lifecycle couples another application to this session. Second consumer with a different tick rate/catch-up policy; overflow, exact remainder and failure accounting oracle before considering a base library |
| Owned read model: ScenarioSnapshot used by InspectorSession, CLI, rendering spike and density (4 paths) | Capacity-checked copy into owned typed buffers; `std::vector`/`std::span` already express it. No asynchronous consumer currently needs a lease/exchange | Samples/fields/infection/stock/ledger, coordinator observation order and state schema | High relative to wrapper size: templates/schema protocols would add more code than the current wrapper. Keep local. Promote only after an independent delayed reader proves lease/backpressure/lifetime requirements; do not turn synchronous snapshots into a pool preemptively |
| Orthographic camera: desktop input/software drawing, GPU projection and rendering spike (3 paths) | Finite world↔logical viewport transforms, fit/pan/zoom and half-open input edges. Existing math/standard arithmetic suffice; SDL renders/events but does not define this complete world-camera contract | GridConfig, world bounds, zoom1..16, layout and input gestures | Medium: separate finite rectangles/points from grid topology; avoid general matrix/scene APIs. Independent editor/scientific viewer caller, extreme/letterbox/edge/anchor fixtures and adapter parity before a small standalone geometry package |
| Segment/quad clipping: ScenePainter helpers (1 owner, multiple overlay uses) | Double-precision finite clipping before float narrowing; scalar arithmetic plus SDL clipping. SDL viewport clipping does not replace validation before raster coordinates are narrowed | Hard-coded View, colors, arrow/ribbon/corridor styling | Low local refactor, medium extraction. Parameterize a concrete rectangle only when another renderer needs it; independent geometry consumer and exact segment-edge/nonfinite/large-value fixtures. Do not put generic planar clipping into a hex-topology package without a consumed fit |
| Radial/capsule field sampling: FieldSet sampled by Simulation and Steering (2 branches of one simulation) | Point-segment distance, normalized direction and falloff arithmetic using `std::hypot`/`std::clamp` | Field slots, force units/signs, stable sum saturation, controller commands and game tuning | Low math-helper seam, high field-framework debt. Keep game policy local; independent non-game force/path consumer plus oracle before extraction. A few formulas alone do not justify a repository |
| Fence slot / receipt: SlotSchedule used by OffscreenRenderer (1 owner); GPU receiver consumes that renderer | Bounded generation-tagged slot retirement. `std::array`, weak issuer identity and SDL_GPU fences already provide the mechanisms | Three slots, instance layouts, Vulkan shaders, RGBA8 size, device-error latch and fail-stop drain | High: a generic recycler could conceal device synchronization and stale-receipt hazards. Keep local until a genuinely separate upload/readback consumer exists; both need busy, stale/foreign receipt, destruction, failed drain and physical-device receiving |
| Prerequisite runner: check_prerequisites CLI and run_tests import (2 paths) | Bounded subprocess output/time, configured compiler/cache reading, capability receipts and CTest inventory. Python stdlib + CMake/CTest already do most work | GPU test names, Crucible option/preset mapping, SDK/device acceptance and artifact-permission policy | Medium: best independent utility hypothesis; separate generic probe execution from project capability declarations. Second repository adoption, Windows process-tree limitation, cancellation/output bounds, configured-toolchain/stale cache and ownership/path tests are gates |
| Capture/provenance helpers: mission, GPU, Phase7 and flow scripts (4 paths) | Revision/tree/dirty-state metadata, executable invocation and artifact receipt writing using Python stdlib | Scene schedule, palette, per-phase evidence schema and actual-vs-generated labels | Low local helper debt; repository extraction premature. Consolidate only shared provenance/failure primitives, not one configurable capture framework; preserve each script's reproduction and limits |
| Spatial bins: Simulation/query consumers (1 owner, multiple calls) | Stable-ID binning and complete exact-radius filtering around consumed H2 candidate geometry | SampleId, fixed entity capacity, physical compatibility/fallback and query borrow | Existing R06, retained. Second independent entity store and full brute-force/lifetime receiving before a new spatial-index package; never fork a second geometry kernel |

## Highest-value base-library hypotheses

1. **Bounded input handoff**, potentially an opt-in Sub0Pub companion rather than
   a new runtime. The distinct value is batch admission and boundary cutoff, not
   another pub/sub API. Investigate current upstream fit only when a second
   application actually needs these semantics; runtime trace/game replay stays local.
2. **Finite viewport geometry**, a small independent camera/clipping package for
   a second editor or inspection tool. Keep it independent of SDL, ECS, hex grids
   and game fields. Current multiple render paths establish a local seam, not an
   independent market or extraction justification.
3. **Build capability receipts**, a Python utility shared by another CMake project.
   Probe execution/configured-toolchain diagnostics are reusable now in shape;
   device-specific acceptance is project policy. Prefer a small shared script
   package over a new C++ library or infrastructure framework.

None justifies creating a repository now. Each needs a named second receiving
consumer, existing-library comparison at current source, minimal package surface,
license decision and evidence that removed duplication exceeds packaging/versioning
cost. Sub0HexGrid is already the independently consumed geometry extraction; do not
inflate it with camera, gameplay or GPU APIs. No speedup or new upstream defect is
claimed from this source review.

## Concrete DRY work before extraction

- Full-state equality is repeated in `src/main.cpp` and
  `spikes/structural/relay_density.cpp`. Centralize a local owned-state comparison
  when adding structural snapshot fields so neither silently omits participation,
  members or generations. Retain independent numerical fixtures; equality alone
  is not an oracle and tests must not merely reproduce production calculations.
- Software `ScenePainter` and GPU `InstancePacket` repeat substrate-state colors
  and the nanite color. One local typed palette consumed by both can prevent
  drift; keep SDL/GPU conversion in their adapters and faction policy outside it.
- The `{24,96,1232,520}` viewport is repeated in the software painter, GPU
  projection validation, GPU receiver and rendering spike. A consumed local
  layout value can remove drift without adding an abstract renderer or resizing
  contract. Preserve each adapter's current validation.
- Capture scripts repeat Git provenance collection. A local Python helper can
  provide bounded failure-aware metadata while keeping per-scene commands and
  claimed evidence separate. This review does not assert those existing scripts
  currently bound every Git subprocess.

Root integration owns any resulting code changes; none is implemented by this
review. Existing pins and standard facilities remain the baseline. Reassess
R01/R02/R03/R04/R05/R06/R07/R08 at structural dispatch with actual callers and
fixtures, rather than treating extraction candidates as committed workstreams.
