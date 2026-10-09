# Phase15: first production playable

Status: authorized implementation,2026-10-09. Dispatch baseline is PR31 merge
`13642fe79c848a8509f2935d7357ad107e309c81`. Root integrates on
`codex/production-playable`. The [in-progress review](../sprint-reviews/phase-15.md)
records evidence and open gates; a plan is not a delivered renderer.

## Consumer and foundation reconciliation

The player launches a cinematic nanite identity, reaches an immediately usable
menu, understands a mission, plays, receives an explained result and returns with
reliable mission-boundary progress. The approved
[concepts](../concepts/phase15-experience/README.md) are a demanding visual target:
beveled silver wedges, cyan cores, industrial surfaces, readable CRUCIBLE assembly,
controlled depth/light and a calm, legible menu. Actual executable captures must
be compared with those concepts throughout implementation.

The source audit found flat SDL geometry/debug text and optional unlit offscreen
GPU quads. Those are insufficient for that target. The user explicitly directs
that SDL rendering become a concept2D/dev-test platform, with an independent
production renderer as the migration target. SDL window/events may remain platform
support. No production SDL_Renderer fallback, shared-device interop or hidden
prototype launch is accepted. See the
[production rendering decision](../decisions/phase15-production-rendering.md).

Use Dear ImGui with a strong Crucible theme for production controls, subject to
actual focus/scaling/legibility receiving. User steering includes DX12/Vulkan
and macOS/iOS in the initial architecture. Select Filament1.77.3 as the first
bounded fidelity receiving candidate: Vulkan on Windows/Linux and native Metal
on macOS/iOS. Native DX12 remains an explicit qualification item through
WebGPU/Dawn or the wgpu-native alternative; Filament has no directly listed native
DX12 backend. The [framework qualification](../decisions/phase15-framework-qualification.md)
records alternatives, exact source evidence and stop conditions. No platform is
called supported until its build, package and native receiving pass.

## Revised workstream split

Retain architect plus two workers. Reassign A from the completed performance audit
to production rendering; B owns art-directed frontend/choreography. Root owns
application transitions, persistence, shared values, production caller wiring,
pins/build/CI, central docs, review and serial hardware receiving.

| Package | Owner / exclusive paths | Production consumer and gate |
|---|---|---|
| P15-R0 graphics foundation | A; new `presentation/production/production_renderer` header/source, owned material/UI-adapter/renderer tests/docs/local manifests | New `crucible_game` app consumes one concrete framework adapter. Receive exact-pin tooling/runtime and one lit wedge plus themed ImGui, resize/completion/capture/teardown and failures. Queued work is not confirmed presentation. |
| P15-E0 identity/frontend | B; new `application/frontend` and `presentation/production/cinematic_choreography` files, owned tests/assets/docs | Root app supplies borrowed view values; UI returns one correlated typed intent. Author silver/cyan/amber theme, licensed typography, stable focus, safe-area/touch layouts, mission/options/result screens and deterministic twelve-second wordmark choreography. No Runtime/profile mutation. |
| P15-J0 journey/progress | Root; application compartment, production app/main, shared contracts/build/CI/docs | Intro/menu/briefing/play/results/save/relaunch/Continue. Only active play advances Runtime; fresh attempts have application-owned identity. Mission-boundary persistence preserves last good progress on rejection/failure. |
| P15-R1 gameplay migration | A + root after R0 | Production renderer consumes actual owned mission snapshots; root receives picking/tool input and existing mission/replay semantics. The prototype is preserved as a comparator, not composited or launched as a production fallback. |
| P15-P0 carried scale work | Root/A after rendering checkpoint and explicit CPU handoff | Guarded ID sorts and exact commit lookup audit is preserved. Resume controlled optimization and unchanged Phase14 G3-G5 acceptance after the renderer foundation is reviewable. No speedup or native budget inferred from the new small-scale scene. |
| Later Quantum Lens | Deferred | Requires completed journey and separately frozen prediction/attempt/charge/replay contracts; no speculative skill or economy API. |

Shared contracts must be reviewed by affected owners before implementation. New
paths have one defining/edit owner. Workers use isolated worktrees from the exact
contract checkpoint and cannot run heavy builds or native measurements without
root's CPU/GPU handoff. Existing historical worktrees and raw evidence stay intact.

## Sequence and receiving

1. Qualify the exact framework/runtime/tool/UI version closure and platform
   contracts; freeze portable scene/view values and publish the source checkpoint.
2. Implement R0 and E0 in parallel. Receive a native lit scene and operational menu
   before expanding the cinematic. Use actual captures to revise geometry, lighting,
   composition and UI; generated stills alone do not receive fidelity.
3. Integrate the twelve-second sequence, immediate skip, reduced motion, focus and
   resize behavior. A static/reduced-motion mode uses the production renderer.
4. Receive mission rendering/input migration, existing recipe rules and safe
   progression/persistence end to end. Include macOS Metal and iOS build/package,
   safe-area/touch/rotation/background receiving in this phase; physical hardware
   and signing availability are named gates. No mid-mission restore claim.
5. Review combined C++, run appropriate Debug/Release/sanitizer/omission checks,
   receive native lifecycle and publish/merge exact-head passing increments.
6. Resume qualified scale optimization on received hardware, keeping Phase14
   service/pacing/useful-parallelism targets unchanged.

The whole Phase15 milestone requires the journey J1-J8 from the
[receiving design](../concepts/phase15-experience/receiving-plan.md), strengthened
here by actual animated cinematic quality and the production-only rendering path.
The earlier design's cinematic-stretch and SDL-still-fallback proposals are
superseded. Bounded subpackages may merge with explicitly open milestone gates.

Audio is currently disabled in SDL's received build. Sound fidelity needs a named
bounded production consumer, licensed assets and actual device/listening evidence;
silent execution cannot close an audio gate. Participant comprehension/balance,
DX12 qualification and unavailable physical faults retain separate gates.

## Carried follow-ups and bounds

Phase14 G3-G5, P01-F05/HW-05 remain open. P05-F02/P06-F02/HW-06 now receive the
new production device/lifecycle subset, not a second-backend claim. P12-F01,
P04-F02 and P01-F04 receive frontend/input readability; human evidence remains
distinct. P12-F02/P13-02 retain actual audio receiving. P09-F01/P05-F01 remain
participant strategy/balance work. Prior terrain/growth/concurrent-reader
follow-ups remain deferred. Prior iOS follow-ups are reassessed through the
explicit Apple build/package/input and physical receiving gates above.

No gameplay arithmetic, biomass ledger, complete neighbor query, worker default
or mission thresholds change for visual fidelity. Decorative intro nanites are
view-owned and never impersonate authoritative gameplay population. Warm UI/font
costs, cold startup/resize/save costs and simulation hot-path allocation gates are
recorded separately. Native build/capture/performance campaigns are serialized.
