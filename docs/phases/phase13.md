# Phase 13 proposal: readable secure-relay play with bounded sound

Status: P13-01 dispatched 2026-10-07 from main `5b84ccd` (PR28); P13-02 sound
remains undispatched. Phase12 merged at `c4ae9c0`; PR26/27 refreshed Pipeline/ECS
and PR28 reconciled ownership since then. Read its completed
[review](../sprint-reviews/phase-12.md) and the current
[coherence review and package contract](../decisions/phase13-coherence.md). This plan
selects no new backend, dependency pin or public library API.

## Player task and two-way design loop

One player should be able to identify the objective, steer a gathering with FLOW,
understand queued versus applied input, secure the relay and recognize the result
without reading a diagnostic transcript. Keep the existing fixed scenario, quota,
conservation, fuse/shatter and completed-tick rules. Improve how those rules are
presented before tuning them.

Top-down work starts with a small screen/interaction/sound concept for that task:
readable objective and controls, visible tool/slot selection, an unclipped FLOW
action, clear queue/application/refusal feedback and a legible relay hold/result.
User direction: evaluate borderless fullscreen as the default launch experience,
with an explicit toggle/escape to a usable window. Compare one or two layouts at
the actual fullscreen monitor size and the smaller window. The current fixed
1280x720 letterboxed canvas may become more legible when enlarged, but receive
physical text size and hit geometry rather than assuming fullscreen resolves
readability. Audition a short bounded background layer and distinct cue patterns for
application, refusal and terminal outcome. Generated concepts or synthetic audio
can help choose direction; they cannot establish executable behavior or participant
comprehension. Desired feedback is prompt and consistent with published state;
pause, mute, device failure and restart behavior must be specified before wiring.

Bottom-up work receives the concrete layout/input/audio requirements against
the current SDL desktop, owned snapshots, command receipts/trace and sequential
Pub/Pipeline path. Reconcile each requirement with those verified capacities and
lifetimes. Implement the smallest vertical slice that can be rendered, operated
and listened to; revise its concept when receiving contradicts an assumption.
Follow the [standing design loop](README.md#standing-two-way-design-loop).

## Evidence that shapes the proposal

Phase12 native Windows operator receiving exercised restart, pause, FLOW admission
while paused at tick456, application after resume at tick486 and a later pause at
tick891. Its 1026x607 screenshot exposed tiny HUD body text (approximately6px) and
an edge-constrained FLOW toolbar. This is useful actual input/rendering
evidence; it is not a participant accessibility, balance or comprehension result.
After the user's fullscreen suggestion, a maximized2048x1280 comparison roughly
doubled body text size and was easier for the operator to read. This supports
the fullscreen candidate; maximized windowed rendering does not receive
borderless fullscreen startup or its lifecycle/toggle behavior.
Link the exact captures and provenance from the completed Phase12 review at
dispatch, rather than treating this summary as a new acceptance receipt.

The live desktop reported D3D11. Physical Vulkan offscreen readback passed a
different receiver; that does not select Vulkan for the interactive world/HUD or
prove live renderer equivalence. SDL audio is currently not initialized. There is
no native audio or human listening acceptance to inherit.

Phase12 measured three replaceable C++ allocations/96 requested bytes per
integrated boundary at the received scales. Preserve that explicit baseline and
measurement limits; no zero-allocation, all-heap or peak-memory claim follows.
Diagnostics have a fixed 64KiB session image, counted drops and a cold decoder.
Neither audio nor HUD policy should depend on diagnostic delivery succeeding.

## Scope, trade-offs and division

| Owner | Bounded package | Division decision |
|---|---|---|
| Architect | Consumer concept, shared desktop/runtime contracts, integration, native receiving, participant protocol and publication | Retain shared wiring; serial builds and uncontended measurements |
| Presentation/input worker | Readable objective/toolbar/status and common drawing/hit-test layout, resize/DPI and queued/applied visual states | Split around owned presentation geometry; notify architect before DesktopApp edits |
| Sound worker | Concrete SDL audio candidate with startup-owned clips, bounded cue/background playback and silence/failure lifecycle | Replace completed diagnostics implementation stream with one real consumer; no mixer framework |

P13-01 consolidates architect and presentation/input under root; no workers are
dispatched. P13-02 is deferred: current DesktopDependencies explicitly builds
SDL_AUDIO OFF. Readability/input has a production caller and can be received
independently before introducing an asynchronous audio lifetime.

Use at most the architect plus two workers. If the audio prerequisite or consumer
contract is not ready, consolidate the readable/input core and record the sound
spike as blocked/deferred with its receiving gate. Do not dispatch three unrelated
frameworks. Claim exact disjoint paths and shared DesktopApp/CMake changes before
implementation. Retain Simulation/game policy locally; freeze no speculative
sound/event/executor APIs.

The primary trade-off is readability and coherent feedback within a bounded
desktop slice, versus material fidelity and a larger renderer/audio system.
Choose clear primitive visuals and a few short sounds first. Keep the current
desktop renderer while its player-facing slice is received. A live Vulkan world
plus HUD comparator is stretch only after presentation/input/audio core gates;
it has independent upload, retirement, compatibility and frame-budget gates.

## Consumed contracts to freeze at dispatch

- Presentation geometry must be shared by drawing and hit testing. Define a
  supported fullscreen/window/DPI range and a minimum usable window behavior
  from native evidence. Receive fullscreen startup, returning to a window,
  focus changes and restored window geometry before selecting the default.
  Provide a visible way to leave fullscreen and a usable fallback if it fails;
  preserve camera/tool, command receipts and mission/pause state across toggles.
  A starting target is body text at least12 physical pixels at the
  received 1026x607 window, subject to concept/actual capture review. Fit or wrap
  controls; never silently clip a clickable label. Keep coordinate conversion
  authoritative across resize and display scaling.
- A receipt acknowledges admission; only a completed trace/frame acknowledges
  application. Visual/sound feedback must preserve that distinction while paused,
  after rejection, on restart and at terminal closure. Use owned numeric state,
  not renderer/ECS borrows retained across a boundary. Continuous hold/progress
  feedback must not emit a new cue every tick.
- For the sound spike, begin with one bounded background layer and one transient
  cue voice rather than a configurable general mixer. Choose and document
  replace/drop/coalescing behavior when cues collide, and a startup clip-memory
  bound from the actual assets. Queue samples/commands with owned lifetimes;
  audio consumption must not allocate or read mutable simulation state. Any
  cross-thread path requires an explicit synchronization and joined shutdown
  contract. A device may consume asynchronously even with one simulation owner.
- Freeze pause/resume/background behavior, audible levels, mute control and
  restart cancellation before coding. Sound is optional feedback: unavailable
  device/output must retain playable silent behavior and visible status. Startup
  assets need documented provenance/licensing; generated or synthesized spike
  assets remain clearly identified. No unconsumed settings or event-bus layer.
- Keep the exact received simulation path, command order, ledger and replay
  outcomes. Measure any new per-boundary cost against the Phase12 baseline;
  audio/render costs have separate bounds and do not redefine fixed ticks.

## Acceptance and useful evidence

1. **Consumer concept reconciled:** record selected layout and sound patterns,
   rejected alternatives, supported sizes and feedback/lifecycle policy. Map
   every proposed requirement to an existing capability or a consumed local seam.
2. **Readable real input:** compare native fullscreen and windowed launch,
   including before/after screenshots at1026x607 and the actual monitor size,
   with reported renderer, window/output sizes
   and scaling. Exercise toolbar and keyboard FLOW, queued pause, resume/application,
   refusal, fuse/shatter, terminal result and restart. Verify actual drawing and
   hit testing agree; native resize/DPI receiving is separate from software pixels.
3. **Bounded actual sound:** receive device creation, cue/background output,
   mute, pause/resume, restart, saturation/drop policy and joined teardown.
   A loopback/recorded waveform can establish produced output; operator listening
   establishes heard output on the named device. Neither alone proves user
   comprehension. Keep an explicit silent fallback test for device failure.
4. **Behavior and lifetime:** meaningful focused fixtures plus full-state
   diagnostics-on/off and direct/integrated receiving preserve command order,
   mission/ledger and replay. Exercise repeated close/restart, stale cue cancellation,
   source destruction and asynchronous audio teardown where used. Run cpp-review,
   supported Debug/Release and normal sanitizer acceptance before publication.
5. **Measured limits:** serial uncontended Release receiving records actual
   whole-frame CPU/presentation work and p95/p99 at the current2048 samples,
   sound on/off, and per-boundary allocations. Preserve raw runs, source/pins,
   toolchain/hardware/background load and measurement overhead. Distinguish
   CPU timings, callback/queue timing and observed audio/input response from
   hardware input-to-photon/ear latency; no 100K/150K or full-game60FPS inference.
6. **Participant gate:** if a participant is available, use a short bounded
   task protocol: find the objective/tool, distinguish queued/applied, secure
   the relay and explain the terminal result. Record observations and resulting
   decisions. If absent, leave human/accessibility/balance acceptance open;
   automated operator work cannot close it. Supply personal steps/expected results
   only after matching automated behavior checks pass; use computer control mainly
   to diagnose reported manual failures. Review root docs from a fresh viewer's
   perspective with a short, complete feature/usage checklist.

Retain screenshots, interaction receipts, sound recordings where applicable and
raw measurement output with source revision/dirty state, scenario/seed, tick,
commands, backend/device, dimensions/scaling and inspection notes. Generated
concepts, software exports, physical offscreen readback, native operator sessions
and participant evidence must be labeled separately. Complete the sprint review,
exact-head CI, pushed PR, merge and verified main baseline before phase close.

## Reuse and carried follow-ups

Use [the catalog](../reuse/README.md) and [owned-library roadmap](../reuse/sub0-roadmap.md)
at start/close. R07 receipt/frame, HUD layout and sound cadence are Crucible policy.
R04 diagnostics retain their existing bounded contract. Improve Pub/Pipeline/Log
only when a minimal product-neutral reproduction demonstrates a consumed gap;
include exact-source evidence, ownership/capacity/failure behavior and independent
upstream receiving before changing a full pin. Audio by itself does not justify
new paging/cache consumption, a generic exchange or a copied scheduler.

Reconcile P11-F01/F02 against the completed Phase12 dispositions; accept measured
limits rather than assuming optimization/extraction is required. Continue the
readability/input and human gates P04-F02/P05-F01/P09-F01. Physical Vulkan
offscreen evidence only partially advances P05-F02/P06-F02: live world/HUD,
second backend, complete frame budgets and safe physical fault receiving remain
distinct. Carry P11-F03 concurrency, P01-F03/F04 growth/scale, P06-F01 factions/combat,
P04-F01/F03 iOS, P02-F02 observation and P02-F03/P03-F01/HX-07 world/geometry with
their existing owners/gates. Dedicated hardware availability does not dispatch
those systems. The final Phase12 review owns any new stable follow-up IDs.

Defer broad unit/terrain fidelity, world growth, multi-agent ECS mutation,
general audio authoring/streaming, music system, device fault injection and new
platform/backend promises. Advance only the bounded player task whose concept
and actual backend limits meet in this slice.
