# Phase15 sprint review: first production playable

Status: in progress,2026-10-09. Accountable reviewer: architect/root.
Baseline: `13642fe79c848a8509f2935d7357ad107e309c81`, PR31 merge.
Planning checkpoint: PR32 merged as9f1716410cd223ad65237ded1779a7c3b4f39d54 after
all10 exact-head jobs passed on8bdf587 in run37955070682.
Plan: [Phase15](../phases/phase15.md). Decision:
[production rendering](../decisions/phase15-production-rendering.md).

## Intent, baseline and scope

Receive the approved cinematic/menu art direction and complete return-to-play
journey through a new production renderer. Source feasibility changed the initial
split: flat SDL painting remains concept/dev-test; production owns native graphics
and consumes themed ImGui. The first actual application controller/profile caller
is now received headlessly; native rendering/UI migration is still in progress.

The latest user steering includes DX12/Vulkan and macOS/iOS from the start. A/B
compared existing frameworks independently; root selects Filament1.77.3 as the
first fidelity receiving candidate with Vulkan/Metal. Native DX12 is an explicit
qualification item. Exact release-source/API/toolchain findings are recorded in
the [framework assessment](../decisions/phase15-framework-qualification.md).

## Work delivered and delegation

A completed a read-only guarded-sort/dense-commit performance audit. That work is
preserved and temporarily queued behind the native renderer foundation. B audited
the actual renderer, assets, SDL capabilities and cinematic route. Root integrates
the revised production/platform decision and freezes consumed contracts. Two
workers remain the maximum; no third implementation agent is dispatched.

## Findings and resolutions

| ID | Finding | Resolution / current evidence | Remaining gate |
|---|---|---|---|
| P15-F01 | Existing painter and optional GPU quads cannot express concept depth/materials | Source audit; select independent native production renderer | Actual lit/instanced scene and concept comparison |
| P15-F02 | SDL shared-device interop needs more state restoration than cache flush | Remove that proposed production bridge in response to user direction | Independent production device/lifetime receiving |
| P15-F03 | Handmade input/font/quad plumbing would repeat mature UI infrastructure | Select matched Dear ImGui with strong Crucible theme, SDL3 input and a reviewed framework graphics adapter | Actual focus, scaling, rendering and resource bounds |
| P15-F04 | Fresh InspectorSession identities are session-local | Root app owns separate attempt/transition identity | Stale result/input rejection and successful replacement fixtures |
| P15-F05 | DX11-only selection does not cover requested modern/Apple targets | Reopen selection; candidate Filament Vulkan/Metal, DX12 qualification retained | Exact-pin build/package and real platform receiving |
| P15-F06 | Framework helpers have context/staging/clamping/failure/completion caveats | Root verified release pin; scope narrow consumed adapters and honest receipts | Qualified lifecycle/storage/instance/error receiving before promotion |
| P15-F07 | Paused frontend Back had no controller transition | Close/discard the active attempt, clear snapshot and warn in paused view | Source review and dedicated local Release/Debug case passed; hosted/native input pending |
| P15-F08 | A profile changed to an unsupported version after load could be overwritten | Revalidate current destination under the owned save lease | Dedicated live-version-change refusal case passed locally; uncoordinated edits during lease excluded |
| P15-F09 | Unsigned formatted extraction can accept signed numeric aliases | Full-token unsigned from_chars parsing rejects signs/overflow | Dedicated signed-version/continuation refusal cases received locally |
| P15-F10 | Plan and brief defined intro timing and look but no nanite behaviour (wave, assignment, collision, decision LOD) | [Swarm behaviour design](../concepts/phase15-experience/nanite-swarm-design.md) and headless spike, merged per iteration: lying S0 7.66s; upright via raise S0c (8.79s), climb S0d (11.01s), hybrid S0e (9.17s) and scaffold S0f (10.21s: grounded stems swing up as drawbridge panels, then are climbed) within the user-approved 15s, played at 2/3 pace over 22.5s on screen (S0g; MP4 renders); lane branching removed visible contacts (S0h); 3D tessellation with staggered depth layers passes every gate at two layers (S0i); nanites are 3D bodies from one specification shared by simulation and renderer (S0j), now a tetrahedral proxy of the nanite design space with the form owned there, its shell elongating in motion around an unchanged core, and no simulation change (S0k); a cinematic close-up camera follows the construction, slowing to half pace, then pulls out to the logo, with an activity floor for camera LOD after hidden-tier climbers stalled (S0l); decision LOD by activity by default, with every discrete tier gated to converge in isolation after hidden units were found oscillating forever (S0m); first-come claims, stuck contingencies, selective magnetism; morphing tessellated tiles; Sub0Log JSONL events verified independently; Release/Debug/ASan pass all ten cases | Director review of the scaffold build (recommended) as video; authored panel selection; activity-driven decision LOD (backlog); title depth decision and no-enclosed-holes order for three layers; director review of the proxy body against the core-corrected form study; single-unit rig and docking study (design space); equivalence tolerance between LOD tiers (later); organic freeform construction backlog (O1 clumping to O4 freeform title); S1 native capture |

## Verification and useful artifacts

Current evidence includes source inspection, approved CONCEPT assets and the
[actual headless journey/profile receiver](../workstreams/integration/phase15-journey-receiving.md).
Final independent A/B plan reviews confirmed the matched ImGui/context/upload
ownership direction and identified orientation/font semantics, missing explicit
UI budget freezing and stale iOS/handoff wording. Root corrected the wording and
orientation/font contract and made numerical UI ceilings a required checkpoint
before dependent renderer dispatch. Those ceilings still require qualification.
Application Release/Debug builds and5/5 targeted cases passed in each, including
real mission completion->profile->relaunch. A received the exact Filament Release
runtime/material-tool/link closure at0e8a28e; its receiving record is integrated
with that checkpoint. No native sequence, audio, participant study or performance
acceptance has been executed. Actual captures are pending. Phase14 G3-G5 remain open.

## Retrospective and reuse

The concept demanded a materially stronger rendering foundation. User steering
made that foundation an explicit production migration target. Reuse upstream UI
backends and existing window/input support; keep logo/game policy in Crucible.
No generic RHI or new Sub0 graphics library is extracted without demonstrated
product-neutral consumers.

## Follow-ups and closure

P15-F01-F06 are open with owners/gates above. Carried Phase14 and earlier follow-ups
remain listed in the phase plan. Implementation, receiving, publication/merge and
claim closure are pending; historical worktrees and artifacts remain preserved.
