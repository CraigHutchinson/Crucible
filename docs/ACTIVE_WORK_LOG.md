# Active work log

Claim exclusive paths and heavy CPU runs before starting; update on completion.
Check an old claim with its owner rather than assuming it expired. Benchmarks need
an uncontended host even when files do not overlap.

| Owner/package | State | Paths/resource | Start/base | Handoff |
|---|---|---|---|---|
| Architecture planning | Complete | Architecture, work breakdown, README links; no builds | 2026-10-01 / f835fe9 | Proposed design; implementation packages unstarted |
| Workstream groundwork / primary | Complete | Workstream docs, module CMake/header/source/test areas, root wiring; CPU released | 2026-10-01 / f835fe9 / workstream-groundwork | [Validation](workstreams/integration/validation.md); independent review clean; gameplay packages pending |
| Architect / root | Complete increment | Central docs, shared Simulation/main/root wiring; CPU released | 2026-10-01 / ab9583b / workstream-groundwork | [Wave 1 evidence](workstreams/integration/wave1-validation.md); all combined gates passed |
| wave1_integration / GPT-6.1 Sol low | Complete increment | .worktrees/integration: contracts, contract tests, integration audit docs | 2026-10-01 / wave1-integration | a62ef63 integrated; independent review complete; broader W1 contracts deferred |
| wave1_runtime / GPT-6.1 Sol low | Complete increment | .worktrees/runtime: runtime headers/source/tests/docs only | 2026-10-01 / wave1-runtime | 6141c5b integrated; clock/adapters and full W2 gates open |
| wave1_spatial / GPT-6.1 Sol low | Complete increment | .worktrees/spatial: spatial and fields headers/source/tests/docs only | 2026-10-01 / wave1-spatial | 3175301 integrated; full steering/painted flow open |
| wave1_blight / GPT-6.1 Sol low | Complete increment | .worktrees/blight: blight headers/source/tests/docs only | 2026-10-01 / wave1-blight | defa7f2 integrated; full W4 resource rules open |
| Game intent and concepts / architect | Complete | README, game-design, concepts, central links; no builds | 2026-10-01 / 16670fb / workstream-groundwork | [Game design](game-design.md), [three concepts](concepts/README.md); next sprint not started |
| Phase close and next plan / architect | Complete planning | PR merge, phases docs, workflow links; no local builds | 2026-10-01 / 722e7b3 / phase2-plan | Phase 1 merged by PR 1 at ab708a7; [phase 2 plan](phases/phase2.md), no workers dispatched |
| Hex reuse and world direction / architect | Complete docs | Reuse/game/phase docs; no Crucible code/builds | 2026-10-01 / b1a556b / hexgrid-and-terrain-handoff | Sub0HexGrid H0/H1 delivered separately; [terrain direction](decisions/terrain-and-world-extension.md) preserved; no application migration |
| Phase 2 architect | Complete increment | Contracts, Simulation/main, state copy/presentation, FieldSet observation, integration/docs/root wiring; CPU released | 2026-10-01 / 65bb8c4 / phase2-steer-inspect | [Phase 2 evidence](workstreams/integration/phase2-validation.md); shared gates passed |
| Phase 2 swarm / Sol 6.1 low | Complete increment | .worktrees/phase2-swarm: swarm headers/source/tests/docs | 2026-10-01 / 65bb8c4 / phase2-swarm | 18dfe8e integrated as e605903; bounded rule verified |
| Phase 2 runtime / Sol 6.1 low | Complete increment | .worktrees/phase2-runtime: runtime and telemetry headers/source/tests/docs | 2026-10-01 / 65bb8c4 / phase2-runtime | ad269ce integrated as 88b8a1e; clock/summary verified |
| Sprint reviews and authorized cleanup / architect | Complete docs | docs/sprint-reviews, phase/workflow links, branch refs only; no builds | 2026-10-01 / cdc6034 / sprint-review-records | [Reviews and archive](sprint-reviews/README.md); completed refs removed, detached worktrees/artifacts preserved |

New claims include agent, branch/worktree, package, exact paths, host/CPU reservation,
base SHA and dependencies. Completed rows link evidence or say documentation-only.

## Phase 3 architecture preparation, 2026-10-03

Base: `7692049a2c3b596f35ab27d159efdc215b52f84e`; branch
`plan/phase3-aligned-packages` in the current clone. No heavy CPU/build reservation.

| Owner | State | Exclusive paths / resources | Handoff |
|---|---|---|---|
| Architect | Complete planning; publication | AGENTS.md; central phase/reuse/review/workflow docs; no C++ edits | [Packages](phases/phase3.md), [resource rules](decisions/phase3-resource-rules.md); links/whitespace/oracle checks pass |
| resource_package agent | Complete read-only review | No writable paths or CPU claim | No blocker; resource ID-domain clarity addressed |
| hex_adoption_package agent | Complete read-only review | No writable paths or CPU claim | No blocker; historical hex sequence labeled superseded |

Implementation worktrees/branches and A/B write claims are created explicitly at
dispatch from the planning merge. These read-only assignments do not imply code dispatch.

## Phase 3 implementation dispatch, 2026-10-03

User authorized delegation, execution, push and merge. Plan PR 6 merged at
`837a38002bbc189e7fe319b069a1a6cbba9f28c4` after exact-head CI success.
Worker starting commit `71c14c0` is contained in that merge; architect merged main
before integration. cpp-write/cpp-review recovered from CraigHutchinson/Agentic-CPP;
workers load the full skills and four shared references before C++ authoring.

| Owner | State | Branch / exclusive paths / CPU | Handoff |
|---|---|---|---|
| Architect | Complete and merged | phase3-reclamation-hex (retained); contracts, Simulation/main/presentation, integration tests, root wiring/pins/CI and central docs; configure/build CPU reserved | [Combined delivery](workstreams/integration/phase3-validation.md); Debug/Release 18/18, local adjusted sanitizer 18 tests pass; CPU released |
| resource_package / P3-01 | Complete handoff and peer review | phase3-resource worktree, base 71c14c0 + contracts 7f0e75c; blight/interactions include/src/tests/docs; no CPU claim | 399e945 integrated b4c3dff; [provider](workstreams/interactions/validation.md) |
| hex_adoption_package / P3-02 | Complete handoff and consumer review | phase3-spatial worktree, base 71c14c0; spatial include/src/tests/docs; no CPU claim | 22a77d8 integrated ef1e877; [spatial](workstreams/spatial/validation.md), focused GCC 3/3 |

Phase 3 closed through [PR 7](https://github.com/CraigHutchinson/Crucible/pull/7)
at `21f37867bebec46f215cf489d77eee4db63b0165`; exact-head Linux/Windows
Debug/Release plus Linux sanitizer CI passed all five jobs / 18 tests each.
Local main matched origin/main cleanly. Worker handoffs and local integration
branch/worktrees/build/comparison artifacts are retained; no active worker or CPU claim remains.
This documentation-only closure records the merge and carries the next-phase recommendation.

## Phase4 dispatch, 2026-10-03

Base647f536; user requested next increment and restructured workstreams.

| Owner | State | Exclusive paths / branch / CPU | Handoff |
|---|---|---|---|
| Architect | Complete and merged | phase4-interactive (retained); InspectorSession/runtime, src/desktop, root manifests/pins/presets/CI, SceneUi and central docs/integration tests; CPU released | [Plan](phases/phase4.md), [rendering decision](decisions/phase4-rendering.md) |
| resource_package / P4-01 | Complete handoff/peer review | phase4-input; named Camera2D/FieldTool include/src/tests and camera-input.md; no shared manifests or CPU | f0c0129 integrated cd7aea1; independent input fixtures and reciprocal review |
| hex_adoption_package / P4-02 | Complete handoff/peer review | phase4-drawing; presentation/desktop include/src/tests/CMake (SceneUi reserved root), drawing.md; no CPU | e16c8d6 integrated 9aa9d6f; pixel oracle and reciprocal review |

Phase 4 closed through [PR 9](https://github.com/CraigHutchinson/Crucible/pull/9)
at `09ae74f9f2a1a05c95bb5e1d4196256880cb55a7` after exact-head Linux/Windows/macOS Debug/Release,
headless Release and normal Linux sanitizer CI passed. Local main matched
origin/main cleanly; reviewed tree matched published tree. Follow-ups and next
ownership recommendation are in [the completed review](sprint-reviews/phase-04.md).
Worker branches/worktrees and local build/visual/FMA receiving artifacts remain intact.

## Interim rendering spike, 2026-10-03

Base1d47fa6. User explicitly requested a bounded rendering architecture spike.
Architect owns spikes/rendering, root option/presets/CI and spike decision/evidence;
configure/build CPU reserved. Consolidate this small experiment under one owner;
retain production ScenePainter/Camera/Inspector without gameplay changes. Test a
startup-bounded owned32-byte instance candidate against the actual SDL world pass,
retained-frame isolation, camera-only redraw and advisory extraction/packing costs.
No GPU/console/device result is inferred from software rendering. Carry P04-F01/02/03
and P02-F02 unchanged; real GPU upload/fence/shader acceptance remains open.

Experiment implemented and independently reviewed by hex_adoption_package (read-only).
Review fixed moved-from observer ownership, sRGB labeling and both half extents.
Five Release process measurements and consumed fixtures are retained in
[the spike report](../spikes/rendering/README.md). Publication follows reviewed-tree
and exact-head CI gates; this bounded interim experiment does not dispatch phase 5.

## Phase 5 dispatch, 2026-10-03

Base0bc9731. User explicitly dispatches next sprint and push/merge. Architect reserves
shared contracts, src/desktop, integration tests, central docs/manifests and all build
CPU on phase5-reclamation-challenge. resource_package owns runtime objective/clock/
InspectorSession and runtime tests/local CMake; hex_adoption_package owns SceneUi,
ScenePainter and software fixtures. Shared contract frozen in phase5.md; no worker
build or shared-file write without handoff. Existing branches/artifacts retained.

Phase 5 implementation and reciprocal review complete; local acceptance recorded in
[receiving evidence](workstreams/integration/phase5-validation.md). User requested
permission-loss prevention: configured owned native test preflight now repairs modes
before execution, with a deliberate loss probe and narrow-scope fixtures. Publication
remains the architect claim; resume from the committed tree and retained evidence,
check hosted exact-head results, then merge and record closure.

Phase 5 closed: PR 12 merged at `57c1da65c2e6e74a6f9b7f76f596fb8d36bf590e` after hosted
head `94d2e269c5c6620d7a21acdd6b3629222ff3bdde` passed all eight jobs (desktop/sanitizer
26/26, headless 25/25, two native X11 smokes). Reviewed/published tree matched;
local main fast-forwarded and verified clean. All worker/architect write, review
and heavy CPU claims released. Four permission fixtures and deliberate executable
loss probe passed; macOS temporary-root fixture correction passed hosted acceptance.
Preserve branch/build/strategy/visual artifacts. Next action: start from merged main,
read phase-05 review, carry stable follow-ups and choose the next bounded increment.
Documentation-only closure follows; no repeated local C++ build is needed.

## Phase 6 planning and visual completion rule, 2026-10-03

Base `5411125`; architect owns documentation-only branch phase6-plan-visual-evidence.
User requests next sprint packages for collaborative delegation and visual examples
where viable at each iteration. Existing workers give read-only package reviews;
no implementation/build CPU claim is active. Phase6.md consolidates GPU/numeric/
retirement under one worker and retains a mission-route/evidence provider under
another; root owns shared wiring and validation. Added faction/material/control
extensibility note for three-plus factions and future multiplayer; no such gameplay
or network implementation is dispatched. Preserve completed Phase 5 artifacts.

## Phase 6 execution, 2026-10-03

User dispatched execution from `44701d6`; shared branch phase6-instancing-examples.
resource_package owns P6-M provider, runtime fixtures/manifests and local handoff;
hex_adoption_package owns P6-R gpu include/source/tests/local manifests and handoff.
Architect owns shared wiring, main/integration/diagnostic/capture, central docs and
all configure/build/test CPU. No worker builds/commits before integration handoff.
Tool paths: ../build-tools/cmake/data/bin/cmake and ctest; ../build-tools/bin/ninja;
CPM cache ../cpm-cache; SDL source ../SDL3 at existing full pin. Local Vulkan loader
exists, but driver/shader compiler provisioning is P6-0 prerequisite. Preserve all
Phase 5 branches, builds and captures; no cleanup.

P6-R and P6-M handoffs and reciprocal review complete; architect source checkpoints
b11a377/c9a8704/0cd1299. Local GPU Release31/31 and Debug29+targeted mission1/1
passed. Local sanitizer completion unconfirmed after execution-service disconnect.
No automatic test rerun. Exact authored remaining test payloads recovered into
immutable Git blobs; no source regeneration. Shared root CPU/build session outcome
cannot be observed until service recovery. Hosted exact-head nine-job acceptance
and fresh GPU exporter evidence are required before merge. Local branches/builds
and captures remain retained; local reconciliation is deferred until recovery.
