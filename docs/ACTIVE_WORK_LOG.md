# Active work log

## Sub0 pin refresh, 2026-10-06

Baseline `c195f8d` clean main; branch `chore/sub0-pin-refresh`. Integrator-only:
promote Sub0Pipeline to main `bf2ecce` and Sub0ECS to master `924be41` (v2 merged
and deleted upstream), adapt the two Pipeline call sites and manifests, refresh
current-state docs. Root owns pins, CMake, BoundaryPipeline call site, stack test
and all serial Debug/Release build/test CPU. Status: local receiving complete in
fresh trees (MSVC 19.51 Debug 35/35; Release with diagnostics and benchmarks 37/37);
CPU released. Sanitizer receiving is hosted-only (the preset needs GCC or Clang);
published as a PR for exact-head CI before merge.

## Phase 12 dedicated Windows receiving, 2026-10-05

Baseline `835c781` (PR24 merged), clean main at dispatch; branch
`phase12-runtime-diagnostics`. User requests resumed Phase12 and authorizes graphics,
rendering and interactive playtesting on this dedicated machine. Retain two bounded
packages; root owns shared wiring, plans/review, publication and all serial CPU/GPU
runs. Existing worktrees and Phase2 build artifacts are preserved.

| Owner | Exclusive paths/resources | Status |
|---|---|---|
| Architect/root | InspectorSession, desktop/main, manifests/CI, central docs/evidence; all build/test/measurement and graphics time | Complete; all9 final-head CI jobs passed; PR25 merged c4ae9c0; hardware released |
| diagnostics | RuntimeDiagnostics header/source/test and phase12 diagnostic handoff only | Complete; handed off and independently reviewed |
| measurement | runtime_backbone benchmark, capture script and phase12 budget handoff only | Complete; five paired captures/evidence received |

Phase13 will be shaped by actual native capability/input/readability evidence.
Automated interaction is identified separately from participant balance testing.

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

Phase6 implementation merged by PR15 at 7317a03c8bca6848b17a352f0a192fb0d8e0e375 after exact-head
97ff7d3 all nine hosted jobs passed. Both recovered-file owners verified published
blob identity; fresh hosted GPU captures/provenance uploaded. Worker claims closed;
hosted build CPU released. Local execution service remains unavailable, so its CPU/
run completion and local checkout reconciliation are unconfirmed. Preserve local
0cd1299 source branch, builds/captures and all prior artifacts; no cleanup or reset.
Documentation-only closeout records actual acceptance and remote-source checkpoint
workflow. Next dispatch must start from verified merged main; P06-F02 remains open.

## Phase 7 dispatch, 2026-10-03

Base `0767a303`; branch phase7-fault-receiving. User authorizes plan, investigation,
collaborative execution, push and merge. Local execution recovered; new clean branch
tracks fetched main, old Phase 6 source/build/capture artifacts retained. No old CPU
run remains live; its interrupted sanitizer outcome stays unconfirmed.

| Owner | Claim | Exclusive paths / CPU | Handoff |
|---|---|---|---|
| Architect / P7-I | Remote delivered; local recovery blocked | Shared manifests/CI, central docs, scripts/evidence; all configure/build/test CPU | [Plan](phases/phase7.md), [review](sprint-reviews/phase-07.md) |
| hex_adoption_package / P7-R | Authored/reviewed | tests/presentation/gpu and phase7-faults.md; no heavy CPU | Same production receiver via Linux link wrappers; controlled failures/retirement |
| resource_package / P7-M | Authored/reviewed | tests/integration/mission_sensitivity.cpp; runtime phase7 investigation and human playtest docs; no heavy CPU | Frozen command-timing shootout and short schedule fixtures |

Both read-only planning audits consumed. Root stages/publishes coherent checkpoints
before long acceptance; workers do not commit/push. Pins/rules/default renderer stay
unchanged. Actual hardware loss, physical platform and human playtest remain open.

P7-R/P7-M authored and reciprocally reviewed; root owns final hosted receiving.
Local full GPU Debug34, corrected Release GPU receiving and SDL-free Release28
passed. Local LSan /proc namespace blocks sanitizer acceptance; hosted leak-enabled
checks mandatory. All three frozen timing studies won with full-state replay; root
inspected nine state exports/two plots. Original local evidence copying completed,
but final reads/writes stalled and environment_offline prevents hash review/local
reconciliation. Preserve all files/branches/builds, do not regenerate source.
PR17 receives remotely checkpointed source. The reviewed workflow retains fresh
mission/GPU evidence; Phase 8 proposal remains not dispatched pending closure/gates.

PR17 merged at d2fda2a8042cf9d1b681dc142d3e021bd5a2fa37 after final head8b3eec
passed all nine hosted jobs in run37144416481. Actual tested synthetic checkout19fa1fd
and actual merge share treeefc6982. Remote main verified; local checkout unavailable.
Artifact11281862369 (665196bytes, e9888f...01dd3 ZIP digest) retains fresh GPU/mission
captures for90days; full API/log receiving review is clean. P7-R/P7-M claims closed;
root hosted build CPU released. P07-F01 remains blocked for original local bytes/
hashes, Git capture publication and safe local reconciliation; preserve all artifacts.
Do not dispatch Phase8 until that baseline gate and actual tool/device audit pass.

## Phase 7 recovery and prerequisite increment, 2026-10-03

User requests development/test system/hardware prerequisite gates and dedicated
hardware-session backlog. Source baseline origin/main2e0f3ec; isolated worktree
Crucible-receiving, branch phase7-recovery-prerequisites. Original local files
committed7240f97 and tagged phase7-original-evidence-recovery; no cleanup/reset.
Original11 PNGs, raw output hashes and actual source/executable/shader hashes
match capture metadata. Relevant source inputs match merged main. Fresh isolated
checkout matches merged baseline; original raw directories/builds remain intact.

| Owner | Claim | Exclusive paths / CPU |
|---|---|---|
| Architect | Active recovery/integration | Runner/CMake/CI, central docs, evidence/publication; all build/test CPU |
| prerequisite_checks | Active | scripts/check_prerequisites.py, tests/workflow/test_prerequisites.py; no builds/commits |
| hardware_backlog | Active | docs/workstreams/integration/hardware-receiving-backlog.md; no builds/commits |

No Phase8 renderer/field implementation dispatch. Prerequisite failure blocks
required work; no disabled leaks, silent skips or asserted hardware availability.

2026-10-03 follow-through receiving: root ran 26 capability fixtures and 5 narrow
mode-repair fixtures successfully. Bounded real development compile/run passed;
sanitizer runtime blocked on LeakSanitizer /proc access, and X11/Vulkan child
connectivity blocked in the current execution namespace. Failure receipts retained
under phase7-evidence/prerequisite-local-*.json. No production C++/shader changes,
full local suite reruns or new physical-device acceptance. Hardware backlog and
runner/CI wiring reviewed reciprocally without blockers; hosted final-head CI next.

Final fixture receiving: 30 prerequisite/runner tests and 5 permission tests passed.

Hosted run37155413952: Windows release cold MSVC canary exceeded10s;
required check blocked before build. Windows CI allowance now bounded30s/probe
and60s total; no bypass/retry. New source head requires new exact-head acceptance.

Hosted run37155547246 found macOS normal-variable compiler metadata absent from
CMakeCache and Linux assumptions in mocked workflow fixtures on Windows. Fixed
bounded generated compiler discovery (cache wins; ambiguous paths block), explicit
fixture platforms and separate Windows direct-child coverage. Local36 prerequisite/
runner fixtures and5 mode-repair fixtures passed. Production checks retain their
scope; a final new head must pass all nine jobs before merge.

## Git LFS setup, 2026-10-04

Root owns `.gitattributes`, checkout settings in `.github/workflows`,
`CONTRIBUTING.md` and this log on `chore/git-lfs`, based on `50f61d8`.
No build CPU claim or gameplay edits; existing Phase 8 worktrees are preserved.
Binary art/media use LFS, including existing PNGs via index renormalization.
Published history is retained. Verification: LFS object integrity, attributes,
pointer inventory, byte hashes and remote upload/download; hosted CI before merge.

Local setup complete at `e9212e9`: 26 PNG payloads match their original Git
blobs byte-for-byte; pointer SHA256/size and `git lfs fsck --pointers` pass.
Repository ownership and admin/push permissions verified by the GitHub connector.
Automatic approval review rejected both push attempts, including the retry after
verification, because prior push authorization was not accepted by the reviewer.
No remote branch, uploaded-object receipt, download acceptance or CI pass is
claimed. Publication awaits renewed explicit approval in this session.

User explicitly renewed commit/push/merge authorization. Shell push then failed
for missing HTTPS credentials; publication uses the connected GitHub app.
Temporary branch `chore/lfs-upload` uploaded the 26 existing public PNG payloads
using a repository-scoped Actions token; no Git history was pushed by that job.
[Run 37233289404](https://github.com/CraigHutchinson/Crucible/actions/runs/37233289404)
passed upload, download into an empty LFS object store, and integrity verification.
The bootstrap workflow is excluded from the setup PR and merged main.
Final configuration/pointers await exact-head hosted CI and merge.


## Phase 9 rules and concept integration, 2026-10-05

Dispatch baseline PR21 merge `a95c5111991f441a451df144fdf443d2379a0939`,
independently cloned; no existing workspace/worktree modified. User explicitly
permits commit, push and merge. Repository guidance limits the team to architect
plus two workers, with root owning shared surfaces and all build/test CPU.

| Owner | Exclusive claim | Status |
|---|---|---|
| Architect | README, architecture/game-design, phase/review indexes, CMake/CI, evidence and publication; serial CPU | Active |
| structural_rules | Structural proposal; `spikes/structural/relay_density.cpp` and README | Delivered; root receiving |
| arena_rules | Arena proposal/SVG; independent structural/spike review | Delivered; fixes sent to owner |

Phase9 scope is the bounded investigation already requested by the prior plan:
compare exact rules and execute current production state, then choose the next
consumed increment. No parallel simulation model, structure command API, faction
framework, pin change or physical-device claim. `cpp-write` and `cpp-review` are
not present in the available catalog or searched workspace/skill directories;
existing C++23 conventions and independent source review are used, with behavior,
replay and supported sanitizer receiving retained.


Phase9 local receiving: Debug/Release30 each pass, final extrema metadata receives
its targeted spike rerun in each; full Release900 study/query oracle/conservation
and all three complete replays pass. Initial full Debug180s timeout is recorded;
normal local LSan preflight blocks on /proc, without suppression. Static plot and
arena design mock inspected. Selected next contract64→48+16 from the frozen
production count evidence; no structural implementation or human balance claim.
Workers complete, shared CPU released; final hosted publication receiving active.

Final publication is tracked by [PR22](https://github.com/CraigHutchinson/Crucible/pull/22):
reviewed head/tree, nine-job exact-head acceptance and actual merge/main receipt.
Path/CPU work is complete; publication claim closes with that receipt. No retained
historical worktree/branch/artifact cleanup was performed.

## Phase 10 structural loop, 2026-10-05

Baseline PR22 merge fc9a468. User renews implementation/commit/push/merge permission.
Architect plus the same two workers; root owns all serial build/test CPU.
Architect owns runtime/contracts (BoundaryCommand), mission/main, root inventories,
docs/roadmap and evidence/publication. structural_rules explicitly owns shared
StructuralState/SampleState/BiomassLedger/StateCopy and Simulation plus interactions
paths/tests. arena_rules owns presentation/desktop and presentation tests. All work
on phase10-structural-relay, disjoint paths; no worker builds or stages others' files.


## Phase 11 dispatch, 2026-10-05

Verified baseline PR23 merge `262c4eba4211779042f17bf5a2da03d333c35a91`;
reviewed tree `57c36aee978ff0cfd30697de90beff86a970792b` and nine exact-head
jobs passed in run37247900294. Fresh isolated clone; prior artifacts preserved.
Branch `phase11-sub0-backbone`; user authorizes commit/push/merge.

| Owner | Exclusive paths | Status |
|---|---|---|
| Architect | Shared runtime wiring/ClockDriver/InspectorSession, CMake/CI, differential receiving, central docs; all serial configure/build/test CPU | Active |
| delivery | IntentDelivery header/source/test; runtime phase11-delivery handoff | Active |
| execution | BoundaryPipeline header/source/test; runtime phase11-execution handoff; pinned ECS/API audit | Active |

Simulation/presentation rules consolidate under architect receiving; delivery and
execution split at the existing ingress/session and owned-frame contracts. No
workers run builds or edit shared manifests. Deferred: threaded ECS/graph, Log
adapter without a bounded consumer, GPU/device/iOS/human gates, world growth.
`cpp-write`/`cpp-review` remain absent from available catalog and searched skill
directories; source conventions and independent worker/architect review retained.


Phase11 local receiving complete: Debug35/35, Release35/35, final graph/route
Release2/2 and graph-reuse Debug receiving. Normal local LSan canary blocked by
/proc/task without leak suppression; hosted normal sanitizer required. Worker claims
complete, architect owns publication. Source checkpoint473e4b4/treeced6889 preserved
in [PR24](https://github.com/CraigHutchinson/Crucible/pull/24). Actual integrated
reclamation SVG/provenance visually inspected; phase review owns findings/follow-ups.
Shell HTTPS push lacks credentials; connected GitHub app publishes the identical
reviewed tree. No binary LFS workflow or permission escalation required.
