# Phase 8: playable flow before visual fidelity

Status: recovered and merged 2026-10-04 in [PR 21](https://github.com/CraigHutchinson/Crucible/pull/21),
merge `a95c5111991f441a451df144fdf443d2379a0939`. Original local receiving is preserved;
the recovery merge did not rerun builds/tests.
Architect accountable. See [receiving](../workstreams/integration/phase8-validation.md)
and [review](../sprint-reviews/phase-08.md). The reviewed
[next rules investigation](phase9-investigation.md) is dispatched in [Phase 9](phase9.md).
Baseline PR19 merge `50f61d89bb1324d2f3bdaa09e1b16bcf34fec6d9`, independently fetched
and reconciled locally after all nine exact-head jobs passed at 8f94b6f.
Consume [Phase 7 review](../sprint-reviews/phase-07.md),
[game intent](../game-design.md), [faction groundwork](../decisions/faction-extensibility.md),
[hardware backlog](../workstreams/integration/hardware-receiving-backlog.md),
[flow freeze](../decisions/phase8-flow.md) and [concept gallery](../concepts/README.md).

The user's new priority is a game mock-up that plays before visual flair. Two
inspected art studies now distinguish resources, nanite/Blight variants for four
factions and a proposed deathmatch arena. They guide later fidelity, not gameplay
rules. The smallest consumed increment is a straight drag flow, primitive direction
cues and honest field feedback in the existing reclamation challenge.

## Workstream reassessment

| Prior stream | Disposition / reason |
|---|---|
| Shader/second-backend rendering | Defer production expansion; HW-02 capability audit and HW-03 build-time/runtime shootout remain decision spikes. No verified physical second-backend session exists here; graphical polish must not block playable flow |
| Mission/presentation field inspection | Retain and expand into a consumed FLOW gesture and primitive directional cues; owned command/snapshot boundaries already exist |
| Simulation/contracts/fields | Split numerical command/force/replay receiving from desktop gestures around the frozen FieldEdit contract |
| Resource/game rules | Retain current ledger and quota/deadline; representative art is an interpretation, not new economy/fusion/combat policy |
| Factions/deathmatch/networking | Prepare a bounded rules spike and primitive milestone plan; no speculative types or transport this increment |
| Integration/CI/visual receiving | Consolidate under architect; one CPU owner, published source before long checks, actual images alongside generated concepts |

Architect plus two workers maximum; all work shares one explicit worktree with
exclusive paths. Workers do no builds, staging, commits or pushes. Cross-stream
changes require notification and a frozen caller contract. No permanent platform teams.

## Packages and consumed gates

| Package / owner | Exclusive surfaces | Result / receiving gate |
|---|---|---|
| P8-F / numerical worker | `include/crucible/contracts/FieldEdit.hpp`, `src/contracts/FieldEdit.cpp`, `include/crucible/fields/FieldSet.hpp`, `src/fields/FieldSet.cpp`, contract/field and `tests/swarm/Steering.cpp` tests, flow replay/oracle comparisons and CLI/JSON export audits in `src/main.cpp`, `src/desktop/GpuReceiver.cpp`, `tests/runtime/`, `tests/integration/{phase2,phase3,mission_examples,mission_sensitivity}.cpp`; `docs/workstreams/fields/phase8-flow.md` | Capsule tangent force consumed by Simulation; invalid/zero/extreme/capacity/overlap fixtures, endpoint-preserving owned observation and full replay. Shared contract ownership explicitly delegated by architect against frozen decision |
| P8-P / desktop worker | `include/crucible/presentation/FieldTool.hpp`, `src/presentation/FieldTool.cpp`, `src/desktop/DesktopApp.hpp/.cpp`, `include/crucible/presentation/desktop/SceneUi.hpp`, `src/presentation/desktop/ScenePainter.cpp`, `tests/presentation/`, `tests/integration/desktop.cpp`; `docs/workstreams/presentation/phase8-flow.md`, `docs/playtests/reclamation-phase8.md` | Key4/toolbar drag-preview-release admission, cancel and queue feedback; clipped arrows and radial direction shapes with independent pixel/event fixtures. No competing edits to numerical/export surfaces |
| P8-I / architect | Shared CMake/presets/CI, source checkpoints, capture scripts, central docs/art/reviews and serial CPU | Review coherent handoffs reciprocally with cpp-review; target checks then combined exact-head CI. Capture and inspect actual applied/preview/fit/zoom examples and preserve provenance |

Freeze consumed command before authoring; [the decision](../decisions/phase8-flow.md)
is authoritative. Existing SDL/H2/sub0 pins, GPU lifetimes, default concrete renderer,
mission2048/1780/900 and ledger remain intact. Core gates outrank stretch; no arbitrary
curves, textured art, fusion, terrain/traversal, faction components or networking.

## Sequence, spikes and next increments

1. Complete PR19 receiving/merge and local baseline reconciliation (received).
2. Generate/inspect resource and multi-faction/deathmatch art; document interpretation
   and proposed/current distinctions (received; image hashes in concept provenance).
3. Dispatch frozen numerical and desktop packages; notify peers when the contract is
   ready. Publish reviewed coherent source before long builds/CI. Serialize native CPU.
4. Receive independent numerical, replay and actual SDL input/drawing tests; preserve
   legacy radial mission outcomes. Record local capability blockers without claiming
   passes; use hosted exact-head Debug/Release/ASan/UBSan across supported public targets.
5. Capture actual implementation examples, visually inspect and complete the
   [Phase 8 review](../sprint-reviews/phase-08.md). Push, merge and verify main.
6. Next decision spike: compare a small single-player faction/deathmatch arena against
   extending reclamation with structural allocation. Specify kind/faction/controller,
   relation/authority, resource ownership/contested order, both organism commands,
   combat/loss, win/elimination/ties and replay before implementation. A primitive
   three-plus-faction local arena precedes network transport and fidelity work.
7. Retain HW-02 availability/remote-session spike and HW-03 two-prototype shader
   production shootout before any second-backend expansion. Pick an approach only
   after an available receiver executes its output. HW-04..HW-07 physical/full-frame/
   failure/iOS/human packages remain separate; absent hardware/people keeps gates open.

## Retained findings and reuse

P07-F01 recovery is received via PR19 original bytes/hashes and independently reopened
hosted evidence. P05-F01/P04-F02 human comprehension/DPI/input-to-present stay open;
prepare a comparative FLOW/attract/repel task without inventing participants.
P05-F02 physical/full HUD/window/2K/100K/150K and P06-F02 physical failure policy remain
hardware-gated. P04-F01/F03 iOS signing/device/lifecycle stays separate. P06-F01 faction
identity/control/ledger groundwork is carried into the deathmatch rules spike.
P01-F03/F04/F05 and P02-F02 relay/fusion/concurrent exchange/scale, P02-F03/P03-F01/HX-07
terrain/traversal/spatial/geometry gates remain retained rather than reset.

R07/R08 owned observations and resource rules remain local policy. Reuse existing
FieldEdit ingress, FieldSet, Camera2D, SDL3 and pinned H2; no generic renderer/flow
framework, new pin or upstream speed claim is justified. A straight current is a
small gameplay consumer, not proof of reusable engine extraction or arbitrary paths.
