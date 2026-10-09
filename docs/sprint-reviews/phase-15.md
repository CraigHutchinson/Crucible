# Phase15 sprint review: first production playable

Status: in progress,2026-10-09. Accountable reviewer: architect/root.
Baseline: `13642fe79c848a8509f2935d7357ad107e309c81`, PR31 merge.
Plan: [Phase15](../phases/phase15.md). Decision:
[production rendering](../decisions/phase15-production-rendering.md).

## Intent, baseline and scope

Receive the approved cinematic/menu art direction and complete return-to-play
journey through a new production renderer. Source feasibility changed the initial
split: flat SDL painting remains concept/dev-test; production owns native graphics
and consumes themed ImGui. Current work is planning/contract receiving; no new
rendering or persistence implementation is claimed.

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

## Verification and useful artifacts

Current evidence is read-only source inspection and approved CONCEPT assets.
Final independent A/B plan reviews confirmed the matched ImGui/context/upload
ownership direction and identified orientation/font semantics, missing explicit
UI budget freezing and stale iOS/handoff wording. Root corrected the wording and
orientation/font contract and made numerical UI ceilings a required checkpoint
before dependent renderer dispatch. Those ceilings still require qualification.
No new build, native sequence, audio, saved-game journey, participant study or
performance acceptance has been executed. Actual captures and commands will be
recorded at each received package. Phase14 native G3-G5 remain open.

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
