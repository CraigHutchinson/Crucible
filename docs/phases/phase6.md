# Phase 6: real instancing foundation and mission examples

Status: planned, ready for collaborative delegation; implementation not started.
Planning baseline `5411125d68889c073d56e1526b50c619cd237ed1`, 2026-10-03.
At dispatch, refresh from merged main containing this plan and record that SHA.
Consumes [Phase 5 review](../sprint-reviews/phase-05.md),
[instancing decision](../decisions/render-instancing.md) and
[visual completion criteria](README.md#visual-examples-at-each-iteration).
Also consumes [faction extensibility groundwork](../decisions/faction-extensibility.md):
material/entity kind and faction allegiance remain separate, with no two-side
assumption in instance colors. No faction/network implementation is dispatched.
This document defines future work; no package below has passed acceptance yet.

## Useful increment and scope

Deliver one opt-in, runnable GPU world-pass receiver using real indexed instancing,
owned completed snapshots and explicit upload retirement. Pair it with a small set
of reproducible mission strategy examples, terminal replay and actual visual captures.
Keep the current interactive desktop and SDL-free headless build working. The first
GPU receiver is a diagnostic application/offscreen capture, not replacement desktop
UI: fields/HUD still come from the established production painter for mission examples.
Do not combine SDL_Renderer and SDL_GPU ownership on the same window without a proven
interop contract. No renderer interface hierarchy, per-entity graphics objects or
parallel ECS mutation is required.

Core is one receiving backend (proposed Vulkan/SPIR-V on Linux) with a real GPU API
execution/readback gate, permitted on a software Vulkan device. This proves API and
shader behavior, not physical-device performance. Metal/D3D12/iOS are subsequent
receivers; portable sources and capability records must support their later work.
If no executing GPU receiver is available, record the blocker and retain partial
work; a software SDL comparison alone does not complete P6-R.

## Team and ownership reassessment

Architect plus two workers; roles describe responsibilities rather than permanent
folders. Reuse existing agents only after fresh claims and briefs. Planning reviews
are read-only; they do not dispatch implementation or reserve build CPU.

| Package / owner | Retain, consolidate, split or defer | Exclusive implementation paths | Consumed output |
|---|---|---|---|
| P6-R renderer worker | Consolidate backend, numeric layout, shader and upload/fence rules (prior R1/R2) | `include/crucible/presentation/gpu/`, `src/presentation/gpu/`, `tests/presentation/gpu/`, their local manifests, `docs/workstreams/presentation/phase6-gpu.md` | Concrete receiver and actual readback fixtures wired by architect |
| P6-M mission-evidence worker | Retain bounded runtime/provider and examples; defer balance changes | `include/crucible/runtime/ReferenceMissionRoute.hpp`, `src/runtime/ReferenceMissionRoute.cpp`, `tests/runtime/ReferenceMissionRoute.cpp`, runtime local manifests, `docs/workstreams/runtime/phase6-examples.md` | Shared fixed route consumed by existing CLI and mission exports; measured strategy results |
| P6-I architect | Retain shared wiring, integration and final review | `src/main.cpp`, `src/desktop/`, `tests/integration/`, all shared manifests/presets/CI/pins/contracts, `scripts/`, central docs and retained evidence | Optional GPU executable/capture, production route/export callers, combined validation and publication |
| Platform receiving | Split after P6-R contract is consumed | Future separate Metal/D3D12/platform paths; no writes this sprint | Phase 7 receivers with explicit device/toolchain gates |
| ECS/spatial/resources | Retain existing policy and pins | Read-only for workers | Existing conservation/query/replay oracles remain intact |

Workers send shared-surface patch requests; no concurrent edits to root manifests,
main, Simulation, Camera2D, ScenePainter or contracts. Do not move existing modules
solely to fit this team map. Architect serializes configure/build/test/measurement
CPU; workers may author and review while that run proceeds.

## P6-0: prerequisite audit and contract freeze (architect + renderer)

Before dependent code, inspect pinned SDL GPU headers and actual tool availability.
Record backend choice, shader compiler version/pin/provenance and reproducible shader
build command. No hidden fetch or opaque unchecked shader binary. Freeze a concrete
input/receipt protocol with the two callers; graphics-specific types stay outside
Core/ECS. The spike's raw world-space float record is a candidate, not a frozen ABI.

| Contract | Required decision before implementation |
|---|---|
| Owned input | Snapshot copied/packed by one coordinator; no retained ECS or InspectorSession borrow. Record tick and contiguous cell/marker ranges with startup bounds |
| Geometry/camera | Camera separate from per-frame world data; marker clamp 2–5 logical pixels; fitted viewport/scissor, endpoint and strict exterior behavior agree with Camera2D |
| Numeric representation | Evaluate normalized coordinates/origin rebasing against double projection. State tolerance and representable domain; unsupported conversion returns explicit failure before mutation. Preserve old valid production worlds through the existing SDL path |
| Shader layout/color | Stride, offsets, instance-rate attributes, unit quad/index order, color encoding, blend/order and output format verified by actual shader/readback. Draw base instance zero; do not derive ECS identity from shader built-ins. Per-instance color accepts arbitrary finite RGB in the frozen encoding with alpha=1 for this increment; cells/markers are material ranges, not two faction buckets. Transparency needs a later explicit blending/order gate |
| Bounded retirement | Start with three explicitly owned transfer/device slots, fixed capacities; no overwrite until the slot's submission fence retires. Slot count is a safe initial bound, not an optimality claim |
| Saturation/failure | Query submitted fences; if all slots remain busy, return a busy/skipped-publication receipt and leave slot ownership/data unchanged. Preserve last valid packet/frame; no unlimited wait in the live pump, cycling or hidden allocation growth. Submission failure has a separate receipt and cannot be counted as presentation |
| Teardown/output | Retire all submitted work before release; invalidate receipts on device failure. Core receiver is offscreen. Any native window receiver must preflight before swapchain acquisition; SDL forbids cancellation after acquiring that texture. Specify submit/failure handling after acquisition separately. Resize/replacement and teardown drain outstanding work; blocking drain/hung-driver limits are recorded, with process-level test timeouts rather than unsafe early release |
| Route provider | One pure fixed-route function returns an optional owned FieldEdit for a completed tick; CLI/export caller admits it and checks result before pumping. Preserve the existing 60-tick sweep exactly |

If the backend cannot meet a frozen rule, revise the decision with affected caller
owners before implementing a weaker behavior. Record failures as follow-ups; no
implicit GPU promotion or silent narrowing of GridConfig support.

## P6-R: concrete instanced world receiver

Build a default-off GPU presentation target. Use one static four-vertex/six-index
quad, separate instanced cell/marker draws, camera uniforms and bounded uploads.
Baseline is a full upload; dirty regions, color compression, fused ECS extraction,
concurrent snapshot exchange and density rendering are deferred until measured need.
Reuse SDL3 at its current full pin unless a reproduced blocker justifies a reviewed
pin change. Do not copy a generic graphics framework or create empty platform backends.

Required behavior and receiving evidence:

- Compile shaders reproducibly and execute them against real SDL_GPU API submission,
  readback and fence completion. Verify attribute offsets/stride and stable draw order.
- Read back a labeled synthetic palette fixture with at least three distinct marker
  colors and three cell colors. This tests arbitrary owned color input, not factions
  or player control; no affiliation is inferred from RGB.
- Retain input across later snapshot mutation/restart; camera-only redraw must not
  observe mutable ECS state. Explain whether representation requires repacking.
- Independent fixtures cover nonbinary cell sizes, tiny/large worlds including the
  prior half-cell underflow case, fitted edges, outside points, pan/zoom and clipping.
  Verify rejection preserves prior data and document fallback to the production path.
- Match geometry/color to the established software oracle using an explicit tolerance
  and interior/edge masks where GPU rasterization differs. Freeze those criteria
  before comparing output; do not loosen them merely to pass.
- Exercise occupied slots, delayed completion, full-capacity uploads, failed submission,
  shutdown and output replacement. Combine actual-device/readback tests with a
  deterministic retirement-state fixture; neither replaces the other.
- Run a bounded live diagnostic receiver from InspectorSession owned snapshots at
  2K samples (offscreen sequence is sufficient for this core gate). Capture start and evolved world frames plus one matched software/GPU
  pair. Sidecar metadata records mission/ledger/commands/backend and proves same input.

Completion requires an executing receiver and retained readback/capture, not just a
compiled shader. 100K/150K controlled full-frame comparisons are bounded stretch
only after correctness; shared/software runners give advisory results. No FPS claim.

## P6-M: reproducible mission/strategy examples

Extract only the existing swept-attractor recipe into the named Runtime provider:
slot 0, radius 8, strength 4, every 60 completed ticks; x=8/24/40/56, y=8 then 24,
repeat. It emits values and does not own a session or apply commands. Avoid a general
strategy DSL or input-script framework. Root wires both CLI `--mission --route` and
an integration export caller to this single provider.

Keep target 1,780/deadline 900, population, seed, resource and spatial rules unchanged.
Required cases: passive, existing sweep, stationary attractor and a bounded
repel/reposition/erase sequence using current live tool capabilities. Freeze exact
commands before running; measure the latter outcomes rather than presume wins.
Retain accepted/applied trace, completed terminal tick, ledger and an independent
full-state replay for every case. Add representative checkpoint observations to
show when strategies diverge without introducing gameplay telemetry infrastructure.

Passively LOST900/reclaimed1774 and swept WON267/reclaimed1780/five applied edits
are preserved receiving oracles. Provider tests use independently enumerated edits
at tick 0/59/60/239/240/479/480, not another copy of its arithmetic.
Requests at completed ticks 0/60 apply at boundaries 1/61 respectively. Verify export
inputs match the CLI route; admission is not application. Compare schedule partitions
without changing completed-tick input semantics.

Visual deliverable: actual production-painter ACTIVE, WON and LOST examples, plus a
compact comparison plot/table derived from retained strategy checkpoints. Root owns
export options/capture scripts and `docs/concepts/exports/phase6-*`; worker supplies
validated snapshots/trace recipes and raw receiving evidence. All captions include
scenario/tick and limits. Human comprehension/difficulty remains P05-F01; this pack
prepares a playtest but does not claim one occurred or justify retuning thresholds.

## P6-I: integration, validation and publication

1. Refresh baseline/claims; freeze P6-0 and create in-progress phase-06 review from
   the template. Record implementation worktree, tool paths and CPU reservation.
2. Dispatch P6-R and P6-M on disjoint paths. Each loads cpp-write/cpp-review for its
   C++ work; send provider contracts/patch requests before touching shared callers.
3. Wire the route consumer and optional GPU diagnostic as soon as each provider's
   contract passes; exchange owned replay snapshots as shared visual fixtures.
4. Perform self/reciprocal review once per coherent handoff, then architect reviews
   numerical domain, resource retirement, failure behavior and final caller wiring.
5. Run combined unfiltered Debug/Release/supported ASan/UBSan, existing spike/pixel/
   headless/X11 tests through `python scripts/run_tests.py --preset <preset>`.
   Preserve current eight hosted jobs and SDL-free headless. Add a dedicated Linux
   GPU receiver job only once shader tools and executing Vulkan device are verified;
   that job must fail on missing backend/readback rather than silently skip.
6. Capture and inspect planned examples; validate provenance/reproduction/links and
   retain raw outputs. Log device gaps separately from passed software-device tests.
7. Push a reviewed PR, verify exact-head CI, merge, verify local/remote main and close
   claims/review with actual SHAs. Checkpoint completed commands/evidence before long
   jobs; repair launch issues and rerun only affected checks on unchanged code.

The following configure/test interfaces are to be added by P6-I, not existing commands:
`CRUCIBLE_BUILD_GPU_RECEIVER=ON`, `gpu-debug`, `gpu-release`, and CTest labels
`gpu;readback` / `gpu;lifetime`. Record actual commands after wiring. Portable layout/
lifetime/provider fixtures remain in ordinary tests where they need no GPU runtime.

## Handoff and stop conditions

Every worker handoff supplies base/head, exact changed paths, public provider API,
caller patch requests, tests actually run, reproduction commands, raw evidence and
visual locations, review fixes and unresolved gates. GPU handoff also records compiler/
shader provenance, device/driver/backend, numeric tolerance and slot retirement rules.
Stop at the bounded package; report missing tool/device or changed-contract blockers
promptly. Keep useful branches/builds/captures; no cleanup of sibling worktrees.

## Carried follow-ups and subsequent sprint direction

| Follow-up | Phase 6 disposition | Next receiving gate |
|---|---|---|
| P06-F01 faction/multiplayer groundwork | Review packet/color assumptions only; no new simulation types | Three-plus faction/control/resource/command rules at first consumer |
| P05-F02 actual instancing | Prioritize P6-R; partial until public physical receivers/scale pass | P6 actual draw/readback, then Phase 7 device/frame evidence |
| P05-F01 mission comprehension/difficulty | Prioritize P6-M evidence pack; remains open | Human strategy/comprehension playtest, recorded observations before rule changes |
| P04-F02 native workloads/typography | Retain software UI; collect matched examples | Physical input-to-present/DPI/readability testing |
| P04-F01 iOS and P04-F03 Apple distribution | Defer implementation to receiver sprint | Packaging/signing/toolchain and physical lifecycle/touch proof |
| P02-F02 concurrent exchange | Defer; explicit GPU submission lifetime first | Real delayed reader/upload consumer before CPU frame leases |
| P01-F03/F04/F05 structural fusion/relay/scale | Defer mechanics; mission loop retained | Frozen ledger/identity/atomic commit and protection rules, then consumers |
| P02-F03 terrain, P03-F01 spatial costs, HX-07 oracle | Retain current oracles/pins; no migration | Measured workload or explicit gameplay traversal requirement |

Phase 7 is conditional on Phase 6 review, not an automatic dispatch. Reassess to
split one Metal/macOS+iOS packaging owner and one D3D12/Windows receiving owner,
with architect retaining Linux GPU receiving and common contract/CI. Each must
consume the same verified shader/layout/retirement contract and capture real target
examples. If P6 correctness/lifetime gates remain open, consolidate fixing them
before splitting platforms. Shader production, full desktop overlays, device input,
resize and failure paths must pass before any default-backend promotion. iOS signing
or physical hardware gaps remain explicit receiving blockers, not completed work.

Reuse disposition: R07/R08 remain local; R05/HX-07 unchanged; SDL3 reused. A
product-neutral defect gets a pinned reproduction and upstream proposal, not a new
utility extraction. Review existing sub0 projects again if a real storage/executor
need appears. Restricted SDK details remain outside public repository/evidence.

## Planning review resolutions

Two existing workers reviewed their package boundaries without edits/builds. Mission
review added independent request/application boundary checks and shared route/export
consumption. Rendering review corrected bounded-storage versus bounded-latency:
use [fence queries](https://wiki.libsdl.org/SDL3/SDL_QueryGPUFence) and busy receipts
in the live path, drain separately; three slots do not prove responsiveness. It
also separated offscreen acceptance from window ownership and
[post-acquisition cancellation restrictions](https://wiki.libsdl.org/SDL3/SDL_CancelGPUCommandBuffer).
These are verified against pinned headers; actual backend/tool feasibility remains
P6-0, not a completed execution claim.
