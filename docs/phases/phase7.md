# Phase 7: failure receiving and mission timing investigation

Status: dispatched, 2026-10-03. Baseline `0767a3033a84417c19bde9a4ff46dca0ce118f99`.
User authorizes collaborative execution, push and merge. Consume the
[Phase 6 review](../sprint-reviews/phase-06.md),
[GPU contract](../decisions/phase6-gpu.md),
[faction groundwork](../decisions/faction-extensibility.md) and
[reuse catalog](../reuse/README.md). Execution service recovered; the old local
Phase 6 branch/builds/captures are preserved. This new clean branch starts at remote main.

## Outcome and ownership

Deliver evidence for the existing concrete renderer's failure/retirement contract
and a reproducible command-timing investigation for human playtest preparation.
Keep game rules, default desktop renderer, ECS ownership and dependency pins intact.
No platform-backend promotion follows from injected or software-device evidence.

| Package / owner | Division / exclusive paths | Consumed result |
|---|---|---|
| P7-R / renderer worker | Consolidate receiver, slot and failure investigation; `tests/presentation/gpu/`, `docs/workstreams/presentation/phase7-faults.md` | Same production GpuOffscreen archive exercised through executing SDL/Vulkan calls and private test-executable wrappers |
| P7-M / mission worker | Retain independent mission investigation; `tests/integration/mission_sensitivity.cpp`, `docs/workstreams/runtime/phase7-mission-investigation.md`, `docs/playtests/reclamation-phase7.md` | Existing coordinator/route/painter consumed by an opt-in investigation and a short schedule fixture |
| P7-I / architect | Shared manifests, CI, central documentation, capture scripts/evidence and all configure/build/test CPU | Integrated targets, inspected visuals, source checkpoints, exact-head acceptance and publication |
| Platform backends | Defer split until common retirement/fault receiving is consumed | Later Metal/D3D12 shader/window/device spike with physical-device gates |
| Simulation/factions/terrain/concurrency | Retain existing boundaries; no implementation dispatch | Carry stable resource/identity/replay and future consumer gates |

Workers do not build, commit or publish. Root serializes heavy CPU and stages only
completed coherent packages. Production changes discovered by fixtures require a
concrete bug and affected-caller review, rather than a speculative renderer framework.

## Investigation gates and shootouts

| Known uncertainty | Bounded investigation / alternatives | Decision / stopping gate |
|---|---|---|
| SDL failures are hard to induce deterministically | Compare Linux linker wrappers against a private per-instance call table/friend seam and a separately compiled test variant | Choose wrappers for the current Linux receiver; reject binary-divergent test branches and unconsumed public backend interfaces. Same production archive, explicit intercepted-call scope, real underlying resource creation |
| Three fixed slots do not prove busy behavior | Submit three distinct frames on an executing device while deliberately withholding fence observations; compare to drained baseline | Fourth submit is busy with no map/upload/submit mutation; pending readback leaves destination unchanged; drain permits independent three-frame pixels/ticks, then reuse expires the old receipt |
| Cleanup and error latch are unexecuted | Fail each startup allocation after prior success; startup/frame acquire/map/pass/submit and readback-map calls | Owned resources balance after partial construction; cancellation/fence ownership accounted; failure publishes no completed frame and latches submit/readback unusable |
| Blocking drain has no SDL timeout | Isolated children for injected failed wait and intentional hung wait; parent observes explicit markers and enforces a bounded timeout | Failed drain refuses resource release and expected termination is distinguishable from a crash; hang child is killed/reaped. Never hang the CTest runner inline |
| Four examples do not establish strategy robustness | Frozen cadence/start shootout: 30/0, 120/0, 60/60; original 60/0 remains the Phase 6 reference | Same route positions, slot/radius/strength, 1780 quota/900 deadline/2048 population. Every measured case conserves biomass and independently replays full terminal state. Outcomes are observations, not tuned target assertions |
| Current startup has no configurable RNG seed | Inspect actual coordinator before proposing seed comparisons | No seed API added. Timing is the bounded variable; scenario/initial-state robustness remains a separate future investigation |
| Automated replay does not establish comprehension | Prepare exploratory human protocol before revealing scripted route | Device/build, unaided objective/control/pause/restart observations recorded when humans participate; protocol preparation is not a completed playtest |

GNU linker wrapping redirects undefined references; pinned SDL uses internal
`*_REAL` symbols. Verify receiving hooks by device/handle/phase/count, not solely
by observing an expected exception. See [GNU linker options](https://sourceware.org/binutils/docs/ld/Options.html).
Wrappers and fault controls belong only to the Linux fixture executable; production
constructors, packet/shader ABI and default callers stay concrete. Injected submit
failure must not hide or leak a real acquired fence; consume/cancel the real command
explicitly according to the pinned SDL contract.

The mission study consumes `GetReferenceMissionRouteEdit(index * 60)` with unchanged
slot 0/radius 8/strength 4. Request at completed tick d+k*c applies at d+k*c+1;
stop immediately at actual terminal. Short independent schedule/application fixtures
run in ordinary CI. The three full studies are an explicit Release evidence run,
avoiding another expensive matrix-wide three-case replay. Existing four-strategy
full-state tests remain enabled without reduced checks.

## Acceptance and visual evidence

1. Read-only package gap reviews and seam/timing decisions precede dependent code.
2. Self and reciprocal C++ review; no unresolved correctness/ownership blocker.
3. Full Linux Debug/Release/ASan-UBSan suites; ordinary SDL-free headless preserved.
   Hosted Linux/Windows/macOS Debug/Release plus normal sanitizer and GPU job pass
   at the exact final PR head. Actual GPU readback and fault/retirement fixtures run
   in Release, Debug and sanitizer. Leak checks remain enabled; retain scoped -O1
   and sanitizer-only direct X11 linkage from Phase 6.
4. Each controlled failure proves the intended hook was hit and checks relevant
   resource/command/fence invariants. Isolated expected termination/hang never treats
   an arbitrary nonzero exit as a pass. Physical device loss remains untested.
5. Capture and inspect the three distinct retained GPU outputs and a labelled
   pending/busy/drain timeline from actual receipts. Export mission tick-60 and
   terminal production-painter states plus a plot derived from retained raw results.
   Preserve source revision/dirty status, commands, ticks, tool/device/shader hashes
   and exact reproduction. A picture never proves failure cleanup or hardware speed.
6. Python subprocess tests declare native child prerequisites through CTest
   `REQUIRED_FILES`; permission repair considers only those explicit paths and
   direct commands, never arbitrary arguments. Verify a deliberate process-only
   executable-loss probe before long runs.
7. Publish reviewed remote source checkpoints before long acceptance. CI runs on
   PRs and main pushes; cancel obsolete runs for the same PR, avoiding duplicate
   branch-push/PR builds. Preserve successful receipts; unknown interrupted runs
   are not counted green. Merge the completed increment and verify local/remote main.

## Known limits and next split

P06-F02 becomes partial only when controlled fault/retirement evidence passes:
hardware device loss, driver hangs and physical delayed saturation remain separate.
P05-F02 keeps physical public-device and full-frame 2K/100K/150K gates open. Before
Metal/D3D12 receiver work, plan a shader-production comparison (build-time translation
versus runtime library), verified uniform/color parity, window/overlay ownership,
resize/input/present and teardown evidence. No toolchain/platform winner is chosen
without a runnable target receiver; iOS packaging/touch/signing remains P04-F01/F03.

P05-F01 human comprehension/balance remains open. P06-F01 faction identity, relations,
control/resource authority and replay must be frozen at their first actual consumer;
extra colors still represent presentation values. Carry P04-F02, P02-F02,
P01-F03/F04/F05, P02-F03, P03-F01 and HX-07 unchanged. R07/R08 stay local; no pin
upgrade, upstream extraction or storage framework is justified by this investigation.
Restricted SDK information stays outside the public repository and evidence.

After this sprint, consolidate remaining receiver/device evidence or split platform
owners only after reviewing these limits and actual results. Mission playtest owner
remains independent; architect retains common wiring/CI. No automatic next dispatch.
