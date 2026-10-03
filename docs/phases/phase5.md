# Phase 5: playable reclamation challenge

Complete, 2026-10-03. [PR 12](https://github.com/CraigHutchinson/Crucible/pull/12)
merged at `57c1da65c2e6e74a6f9b7f76f596fb8d36bf590e` after all eight hosted jobs passed.
Dispatch baseline `0bc9731230d55442ca2eaae8670203121bc747a7`.
User authorized implementation, delegation, push and merge. [Review](../sprint-reviews/phase-05.md).

Deliver a bounded mission loop over the existing finite-resource arena: visible quota,
completed-tick deadline, latched win/loss, terminal command rejection and fresh restart.
This is a reclamation challenge preceding the planned relay/fusion scenario. Freeze
rules in [the mission decision](../decisions/phase5-reclamation.md) before implementation.

| Stream | Decision / owner | Exclusive writes / result |
|---|---|---|
| Mission runtime | Consolidate objective and completed-tick stop; resource_package | Runtime InspectorSession/ClockDriver and mission implementation/tests/local inventories; evaluates every completed tick |
| Mission presentation | Retain concrete painter; hex_adoption_package | SceneUi/ScenePainter and software fixture; visible instructions/progress/outcomes without changing world geometry |
| Integration/design | Architect | Contracts, src/desktop, integration tests, shared manifests/docs and CPU; live caller, instancing decision, publication |
| ECS/resource/spatial | Retain existing code | No population/ledger/topology change; verified H2 pin and replay stay intact |
| GPU/platform | Defer implementation; specify consumed gate | Actual instancing, shader/transfer retirement, public devices and packaging are separate foundation work |

Stable shared value: `contracts/ReclamationMission.hpp`. InspectorSession accepts an
optional third constructor argument `std::optional<ReclamationMissionSettings>` default
`std::nullopt` (preserves inspector/replay fixtures), and exposes
`GetMission() const noexcept -> std::optional<ReclamationMissionProgress>` by value.
Desktop explicitly opts into default settings. SceneUi appends optional `mission`.
One coordinator; no ECS/GPU borrows or background exchange. Workers share this branch
with disjoint paths; root serializes all configure/build/measurement CPU.

Acceptance: deterministic boundary win, deadline loss, success on exact deadline,
catch-up stopping at first terminal boundary, no paused deadline progress, blocked
boundaries preserving mission, terminal edits denied, resume unable to revive outcome,
restart resets mission/queue/trace/frame, and production event/painter consumers.
Preserve Debug/Release/Linux ASan/UBSan and Linux/Windows/macOS CI, SDL-free headless,
native X11 smokes and spike pixel fixtures. No human playtest, GPU timing or tuned
difficulty claim. Bounded stretch: a reproducible actual mission frame.

Rendering learning: [instancing decision](../decisions/render-instancing.md) distinguishes
two SDL geometry batches from true instance draws. Keep camera separate, explicit
ownership and independent overlay passes. Candidate payload savings are arithmetic;
extra extraction cost and numerical/device gates block production promotion.

Carry P01-F04/F03/F05, P04-F01/F02/F03, P02-F02/F03, P03-F01 and HX-07 into review.
Reuse disposition: R07/R08 retained local game policy, SDL3/H2 reused unchanged;
no new reusable utility or upstream change absent a product-neutral defect/consumer.
