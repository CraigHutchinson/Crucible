# Phase15 first native frame and frontend freeze

Portable contract proposal reviewed by A/B,2026-10-09, baseline `13642fe`.
This specializes the [production decision](phase15-production-rendering.md).
Framework-specific sequencing/receipt APIs require the
[candidate qualification](phase15-framework-qualification.md) before freeze.
The initial DX11-specific API proposal is superseded. No worker implements
dependent code until root records the reviewed checkpoint.

## Scene producer and consumer

Root defines `presentation/production/cinematic_frame.hpp`. Its primary
`CinematicFrame` couples a synchronous `span<const NanitePose>`, camera and
consumed key-light/core-glow values. `NanitePose` is position `array<float,3>`,
unit quaternion XYZW `array<float,4>`, positive uniform scale and nonnegative
coreIntensity. Camera is eye/target `array<float,3>` and verticalFovRadians.
Consumed light/glow values are keyIntensity within[0,8] and glowIntensity within[0,4].
No custom vector/span/matrix framework is introduced.

Scene coordinates are right-handed,+Y up; the canonical wedge faces+Z and has
unit width/length before positive scale. A owns exact beveled mesh/substrate,
fixed silver/cyan materials, clipping and lighting implementation. Camera eye and
target are distinct and may not have a vertical-only sight line; FOV is finite
and within(0.1,2.8) radians. Positions/camera coordinates have magnitude at most512
per component; scale is within(0,16]. Pose arrays are finite; quaternion squared
length differs from1 by at most0.002; coreIntensity is within[0,4]. At most4096 instances
are accepted. Sight-line length must be at least0.1 and the normalized sight line's
cross product with+Y must have length at least0.01. Clipping is0.1..2048 scene units;
validate finite derived camera/instance transforms before upload. Renderer validates
the entire packet before native mutation, rather than accepting finiteness alone.

B's `CinematicChoreography` owns startup-sized pose storage and evaluates from
integer elapsed milliseconds clamped to0..12000, preserving count/order. Its
returned frame/pose borrow expires on next evaluation/destruction. Renderer reads
and uploads synchronously and retains no caller span. Root's clock selects the
settled12000ms composition for reduced motion/skip. Decorative poses do not enter
Simulation. R0 first renders the same packet with one lit wedge before full art.

## Production framework owner and receipts

A owns `presentation/production/production_renderer.hpp/.cpp`, material resources,
reviewed UI/upload adapter, local manifest, owned tests and presentation brief.
One concrete ProductionRenderer borrows root's window/context and owns the selected
framework/resources. Do not expose native backend objects, a virtual RHI,
SDL_Renderer or an automatic prototype fallback. The Filament candidate uses the
exact release's ImGui core and an attributed helper adaptation.

Startup instance capacity is4096. Desktop starts1280x864, minimum1024x607; mobile
layout instead receives safe logical content with short edge at least320 and long
edge at least480, in either orientation. Smaller safe areas are explicitly
unsupported. Controls remain clipped to that rectangle; content that exceeds it
scrolls and preserves focused-control visibility after rotation. Drawable bounds
are4096 per axis with checked total resource bytes.
Quality tiers initially target1600x900 desktop and1280x720 mobile scene buffers,
with projection/composition using the actual drawable aspect; UI uses actual
logical DisplaySize and FramebufferScale. Receive supported DPI0.5..4.0. Font
sizes14..96 are logical pixels after the selected text scale is applied once;
framebuffer density then gives raster sizes7..384. Reject unsupported combinations
before rebuilding resources, and receive each finite scale/DPI warm state.
Zero output suspends submission. Oversized or
unsupported output is explicit refusal; no unbounded allocation or integer wrap.

Root prepares platform input/NewFrame, B emits UI, root finalizes ImGui draw data,
then the adapter consumes the validated scene and UI. All public calls are on the
coordinator. Framework-internal render threads touch no Runtime/ECS state.
Pose/UI data is copied into bounded adapter-owned storage before its call ends;
asynchronous uploads retain that storage until completion. Preflight all draw
counts/byte totals/resources and reject unsupported custom UI callbacks before
accepting the framework frame. True Filament beginFrame obliges endFrame; failure
sequencing cannot inherit the earlier arbitrary DX11 refuse-Present contract.

Before dependent renderer dispatch, root and A must record numerical ceilings for
UI vertices/indices, texture dimensions and aggregate bytes, per-frame texture
updates, outstanding upload/capture slots and total staging bytes at the exact
framework checkpoint. Scene/drawable bounds alone do not establish these limits.
Reject budget overflow before beginFrame. A timeout never releases storage still
owned by an outstanding callback; retain it until settlement or qualified teardown.
These budgets are pending qualification, not an allocation-bound claim today.

Receipts identify application frame and surface generation, distinguishing
scheduled,skipped,suspended,rejected,failed and qualified drain. Scheduling is not
native handoff or GPU success. Root consumes copied backend/adapter metadata and
cold completion/capture operations; no always-on readback. Resize invalidates old
surface receipts and joins/purges owned work before resource/storage replacement.
Device errors stop production execution with explicit failure; process-fatal
framework paths are not disguised as recoverable status returns.

Filament Fence alone proves render-thread progress. Its pinned Vulkan finish
ignores queue-wait results, so flushAndWait alone does not prove checked GPU
success under loss. Freeze the qualified receipt or receive a minimal attributed
adaptation before making that claim. Cold captures include scene+UI, use supported
readback formats and retain callback storage until settled. Physical handoff,
scanout and timing remain independently observed gates.

Root owns ImGui context and platform backend. The adapted graphics helper must
preserve that ownership, theme and disabled ini policy. Destruction drains native
uploads/captures, destroys renderer/UI adapter, shuts platform backend, destroys
context/window, then SDL. One viewport; no docking/platform windows.

## Frontend product values

Root defines a product-local `application/frontend_view.hpp`, including:

- `MissionId`: reclaimFront and secureRelay only.
- `FrontendScreen`: intro,menu,missions,briefing,playing,paused,options,results,
  saveFailed and quitConfirmation.
- `FrontendOptions`: reducedMotion,fullscreen and finite supported text scale
  (standard,large,extraLarge). No audio slider before real sound consumption.
- `MissionCard`: ID,title,objective,enabled and disabledReason, synchronous strings.
- `FrontendView`: screen,transitionSerial,selectedMission,at most2 cards,
  canContinue/continueReason,options,status,error and optional copied completed
  `ReclamationMissionProgress`, plus root-supplied logical safe-content rectangle.
  Intro/menu composition is independent of play state.
- `FrontendIntent`: originating screen/transitionSerial and a variant containing
  navigation action,mission selection or the particular changed-setting value.
  Unrelated mission/setting payloads are not carried by every action. At most one
  optional intent per draw; root validates origin and current screen before applying.
  Repeated completion is idempotent.

Actions are Continue/New Run/Missions/Options/Back/Begin/Resume/Retry/Next,
mission selection, explicit Quit confirmation/cancel, reduced-motion/fullscreen/
text-scale changes and Replay Intro. The values do not serialize Runtime/ECS.
Skip Intro is explicit and consumes its triggering input. Card titles are at most
96UTF-8 bytes; objectives/reasons256, status/error512, all valid UTF-8 with no embedded
null. Reject invalid views before drawing; root formats/truncates only at code-point
boundaries. The first label repertoire is finite English; localization needs a new
font/resource receiving contract. Strings/spans expire after draw; B retains
only stable IDs/focus metadata, never view borrows or authoritative state.

B owns `application/frontend.hpp/.cpp`, its frontend fixtures/assets/owned brief,
and `presentation/production/cinematic_choreography.hpp/.cpp` with owned fixtures.
Frontend consumes the root-created active ImGui frame, applies authored theme/
licensed fonts at cold construction, and returns the optional intent. It does not
call platform backend/NewFrame/Render, feed events, mutate Runtime or save files.
Root creates context before font/theme initialization and binds native resources
afterward. A owns framework texture/upload lifetime. Use upstream ImGui controls/
draw lists, stable IDs and transition-triggered default focus, not a parallel UI
framework. Root supplies the native hero texture only if a consumed static mode
needs it; the baked menu board is reference art, never a functioning screen.

Only active play pumps Runtime. Root owns attempt identity distinct from each
InspectorSession's session-local run ID, destroys/replaces sessions only after a
complete new construction, clears stale previews/captures and resets clock debt
on transitions. Pause/results/save/menu cannot advance tick outcomes.

## Root wiring, receiving and reviews

Root owns the new production app/main, common values, progress/profile, assets
packaging, dependency pins, shared CMake/CI and central docs. Both workers review
these contracts before dispatch. Profile/mission migration freezes follow from
the actual application receiving caller; they do not broaden renderer APIs now.

Read-only cpp-review plan pass checks L0 named callers, L1 ownership/dependency
direction and L2 bounds/failure API. Provider handoffs require source review and
meaningful lifecycle/invalid-input fixtures. Root runs combined builds/tests and
native capture, then records actual concepts-versus-executable findings. A/B
implementation and heavy CPU/GPU use require root dispatch/resource handoff;
standing user authorization already covers the work.
