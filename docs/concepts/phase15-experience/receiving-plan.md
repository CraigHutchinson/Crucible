# Proposed next sprint: receive a complete return-to-play loop

Design handoff,2026-10-09. Architect integrates this into the phase plan after
reviewing the current technical receiving baseline. This is a proposed workpackage,
not a claim that Phase14 is complete or implementation agents are dispatched.

## Balanced slice and explicit exclusions

Top-down: a player enters through a readable branded menu, launches a clearly
briefed mission, understands the result and returns later with reliable progress.
A separate cinematic spike expresses the high-fidelity swarm identity with actual
skip/focus/resize/audio behavior when a concrete route is available.

Bottom-up: reuse current bounded fields, clock, coherent snapshot, terminal mission
state and relay ledger. Domain math and biomass rules stay unchanged. Add only the
local mission-descriptor/profile transitions those screens actually consume.
Current audio/decode/PBR capabilities and complete save restoration are not inherited.

Meet through **Main Menu → Reclaim the Front → Results → committed completion →
Secure the Relay → Results → relaunch → verified Continue**. The mission-boundary
profile is the first persistence consumer. In-progress attempts explicitly restart
from their start. This receives progression through existing mechanics rather than
inventing a new economy to justify the menus. First Current onboarding receives its
own predicate later. Full cinematic fidelity is stretch after the accessible still
and handoff are proven, not a blocker for the playable loop.

This first slice targets the current small reference scenario. It does not promote
100K/150K native budgets, worker defaults, speculative shader/audio features,
terrain/bridge traversal, campaign combat, new growth/attrition or Quantum abilities.
The major Phase14 performance gates and direct worker FP restoration observation
remain separately owned and open. Uncontended native reservations apply to both.

## Proposed parallel work packages

Retain architect plus at most two workers. Assign exact paths and an immutable
contract checkpoint before dispatch. Shared artifacts have one owner; workers send
shared edits as patches. Table paths are ownership proposals for root to freeze,
not permission to edit them from this design worktree.

| Package / owner | Named end consumer and responsibility | Workstream boundary / prerequisite | Handoff and review gate |
|---|---|---|---|
| E00 / architect | Desktop state transitions consume menu intent; Runtime constructs current recipes and supplies completed outcomes. Freeze supported mission IDs, profile scope, input ownership, failure and state teardown order. | Root DesktopApp/main/Simulation/Runtime/Contracts/CMake/central docs remain root-owned. No new library pin by default. | Reviewed value contracts with real caller inventory, baseline tests and source checkpoint before workers edit. |
| E01 / workerA | Results-to-profile transaction and menu Continue consume bounded version1 progression data. Stable mission descriptors consume current scenario settings. | Propose one local application progression compartment with owned tests/docs; defining file locations are root's decision. Does not serialize ECS or own Runtime. | Valid/corrupt/future-version/oversized files, idempotent completion, replacement and recovery failure, no partial progress, new-attempt/reset policies; root integration receives the real path. |
| E02 / workerB | Actual menu/mission/briefing/results/options view consumes E00 values, returning typed local intents; focus and accessible navigation own input. | Presentation tests/docs and explicit new local screen implementation only; root owns DesktopApp event wiring. Can prototype layout against frozen copied values while E01 proceeds. | Actual clicked/keyed menu→mission→result navigation, supported-size captures, rejection feedback, disabled Continue/Next reasons, focus/resize/pause/close behavior. |
| E03 / root + workerB after E02 | Intro-to-menu handoff consumes one concrete cinematic/still route, skip/reduced-motion preference and optional bounded audio. | Bounded follow-on to E02; no third worker, no independent renderer framework. Codec/audio/GPU adoption only after capability receiving. | Timed native sequence, skip from each beat, no click-through, mute/device failure, actual memory/complete-frame evidence; generated stills alone cannot receive it. |
| E04 / architect | Combined playable/save/resume behavior receives both provider handoffs and records independent review, exact-head CI and publication. | Root exclusive integration/build/native reservation and central documents. Workers perform source/fixture review in parallel. | Complete receiving matrix below, then reviewable PR, actual required checks, merge and verified baseline. |
| Later Q / future freeze | Quantum Lens consumes cold complete state copy, bounded prediction and mission-local charges. | Undispatched after journey gate; root must freeze attempt-state/replay/charge semantics first. | Independent branch-state/live-ledger parity, refusal/debit atomicity, bound/latency and player-value evidence before production. |

No worker chooses a public save/executor/audio abstraction on the assumption a
future consumer might want it. Product-neutral proven requirements may be proposed
upstream with a minimal receiving reproducer; mission rewards, logo choreography,
charge policy and save compatibility remain Crucible policy.

## Transition and ownership freeze

Proposed states: Startup, Intro/StaticTitle, Menu, MissionSelection, Briefing,
Loading, ActiveMission, Pause, Results, SavePending/SaveFailed and ShuttingDown.
Options returns to its explicit origin. Do not use this list as a mandatory new
framework or public type; the concrete Desktop owner selects the minimal consumed
representation after reviewing current event handling.

Only ActiveMission advances normal gameplay. Briefing/Results/Menu do not accrue
elapsed clock debt. Loading must signal actual completion or failure; menu Begin
has a single transition identity and cannot double-construct a run. SavePending
keeps result state immutable and retains the prior committed profile. SaveFailed
offers Retry and an explicit unsaved exit. Close cancels work and joins/drains
owned producers before destroying their storage. A screen holds copied completed
state; it never keeps a mutable ECS reference or worker result span across epochs.

The profile owner stages one candidate generation and validates complete data
before publication. A result completion carries stable logical attempt/mission
identity so repeated Retry can never credit twice. The presentation does not infer
save success from a file appearing or from a spinner finishing. A new successful
Runtime construction/restart has fresh run identity; stale UI/observation receipts
cannot mutate it. Future mid-mission checkpoints have stricter complete-boundary,
pending-command, FP/rules fingerprint and cold restore gates in
[the experience design](experience-design.md#persistence-and-safe-continuation-policy).

## Acceptance matrix and honest evidence tags

Use these tags on sprint artifacts: **CONCEPT** for generated proposals;
**SOURCE/FIXTURE** for source review and automated behavior; **PLAYABLE** for a
received executable interaction; **PHYSICAL PROOF** for named device capture or
human observation. A physical operator run is still not a participant comprehension
study. Each physical result records source/pins/toolchain/device/settings and scope.

| Gate | Required observation | Evidence and failure rule |
|---|---|---|
| J1 objective/result | Launch both existing recipe types, show unambiguous goal/defeat/terminal reason, and retry without mutating the prior result. | SOURCE/FIXTURE terminal/restart regression plus PLAYABLE win and loss captures. Preserve current thresholds and64/48/16 ledger. |
| J2 full journey | Cold launch/menu/briefing/play/results/next/menu/close/relaunch/Continue follows the selected mission-boundary policy. | PLAYABLE actual keyboard and mouse run, source-backed transition receipts. No console workarounds or false mid-mission resume claim. |
| J3 safe progress | Save and restore compatible progression; inject incomplete/failed replace, missing/unwritable directory, corrupt/oversized/new-version file and duplicate terminal transaction. | SOURCE/FIXTURE plus actual supported-filesystem receiving. Previous valid save/result survives; no silent reset, duplicated unlock or partial live mutation. |
| J4 accessible control | Traverse every screen without pointer, cancel previews, use supported small-window/DPI/fullscreen layout, retain focus through options. | PLAYABLE resized captures, no occluded controls, shape+text refusal/selection. Participant comprehension has separate protocol/results. |
| J5 deterministic behavior | Menu, intro, result, paused time and save latency do not change mission tick outcomes. Same current command script reproduces full semantic state. | SOURCE/FIXTURE full unfiltered regression and rule-identity parity. Corrupt/unsupported restore is rejected before replacement. |
| J6 cinematic handoff | A concrete timed route meets readable wordmark, coherent shots, skip/no click-through/reduced motion and still/silent failure path. | CONCEPT boards guide direction; PLAYABLE actual sequence and PHYSICAL PROOF captures/listening establish delivered quality. Static-only can close handoff but not animated cinematic fidelity. |
| J7 resource/fidelity bounds | No per-step game allocation added; cold save/copy/decode cost scoped separately; fixed owned capacities and correct drain/destruction. | SOURCE/FIXTURE allocation/lifetime receiving, PHYSICAL PROOF complete-frame/selected-tier memory scopes. No simultaneous native campaigns. |
| J8 release/closure | Independent cpp-review, appropriate Debug/Release/sanitizer/race receiving, complete combined tests and required exact-head hosted checks. | Reviewable published changes, actual merge and verified main. Honest unresolved participant/audio/fidelity/performance gates remain named. |

Keep raw failed receipts and slower arms. A cinematic build that only plays a video
once has not received teardown, resize, focus or skip. A synthetic profile fixture
has not proved operating-system durability. A generated menu has not proved font
legibility in the real renderer. A smooth2048 scene has not proved150K full-frame
performance. Reuse Phase14's measurement discipline, never its unclosed headline.

## Test and review depth

Review contract placement/dependency direction, naming, ownership/lifetime and
all caller sites before integration. Validate maxima and arithmetic before storage
construction. Tests should target meaningful failures and invariants rather than
duplicate implementation. Receive preference/profile cold work outside hot loops;
if asynchronous persistence/audio is chosen, bound queues, cancellation and joins
through actual shutdown/failure cases instead of retaining borrowed screen state.

Core menu/progression changes need full existing mission/resource/replay coverage,
not only screen tests. Actual restore/save tests must exercise replacement failure
and old-data compatibility with independently checked expected state. A later
Quantum prototype must compare against unchanged live state and a independently
reconstructed forecast reference; eye-catching ghost movement is not correctness.

## Questions the architect should close at freeze

1. Which exact current recipe descriptors are exposed, and is First Current help
   only or a separately received mission predicate?
2. What is the supported profile directory/platform policy, and what bounded
   maximum records/file bytes and atomic-replacement guarantees will be received?
3. Which current input actions need distinct Fit/Fuse bindings without regression,
   and which minimum sizes/accessibility modes are committed for the next slice?
4. Which concrete intro route is viable after still-to-menu receiving: pre-rendered
   clip or bounded live scene? What audio/device capability is actually present?
5. Which Phase14 gates block adoption of settings/backends, and which experience
   work can proceed at the received small scale while those gates remain open?

These are design freeze decisions for root to resolve through current source and
receiving, not repeated user permission requests. Ship and review the bounded
journey first; iterate cinematic quality and one skill consumer on demonstrated
limits before expanding the game surface.
