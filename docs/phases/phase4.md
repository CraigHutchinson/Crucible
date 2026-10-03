# Phase 4: interactive inspection and spatial control

Status: implementation delivered; publication validation in progress, 2026-10-03. Base main `647f536b0a37b48f08b1b58b0c791e2ec850bf4b`.
[Phase 3 review](../sprint-reviews/phase-03.md) recommends this increment;
[rendering decision](../decisions/phase4-rendering.md) freezes the consumed boundary.
Architect plus two workers maximum. User authorized continued iteration and changing
stream boundaries; existing push/merge authorization remains in effect.

## Outcome and deliberate workstream structure

Make the existing finite-reclamation simulation observable and controllable in a
native window: live nanites/infection/stock, committed fields, conservation, command
feedback, pan/zoom, pause/resume and fresh restart. This is an interactive prototype,
not the complete relay mission or a measured 100K/150K renderer.

| Package / owner | Scope / real consumer | Exclusive writable paths |
|---|---|---|
| P4-01 / input worker | Pure Camera2D and FieldTool→FieldEdit; desktop event coordinator and painter consume them | Named Camera2D/FieldTool include/src/test files and camera-input.md; no shared manifests |
| P4-02 / drawing worker | Concrete startup-bounded ScenePainter, two geometry batches, field/preview/HUD layers; native/software callers | presentation/desktop include/src/test subtrees and drawing.md; SceneUi header reserved to architect |
| P4-00/03 / architect | Backend/data/lifecycle contracts, InspectorSession, desktop callbacks/events, root wiring/pins/presets/CI and combined proof | Runtime InspectorSession, src/desktop, desktop/integration tests, shared SceneUi, central files/docs/manifests |

Consolidate camera and discrete tool intent because both define window-to-world input.
Split drawing because palette, batch capacity and retained-frame ownership have
independent pixel oracles. Keep event/admission/session/restart wiring with integration
because it owns coordinator lifetime and the production caller. Retain module folders;
do not rename them to match temporary agent names. Defer separate ECS, resource,
spatial/hierarchy, fields, telemetry, scheduler and concurrent-exchange workers:
existing domain policy and owned frame already supply the required consumer.

## Frozen contracts

Canvas: 1280x720 logical render coordinates; world viewport {24,96,1232,520}.
SDL letterboxing/window-to-render conversion handles native pixels and DPI first.
Camera owns validated GridConfig, a finite positive ScreenRect, center and relative
zoom1..16. Fit preserves aspect ratio; center constraints keep visible zoomed axes
inside the world. Pan logical drag moves camera oppositely. Cursor zoom preserves
anchor until edge constraints intervene. Half-open viewport input rejects exterior,
HUD, letterbox/world exterior and nonfinite values; never clamp exterior clicks into
field placement. Invalid pan/zoom preserves state. ResetFit supports F and restart.
The fixed canvas has no current caller for mutable viewport configuration.

Use ScreenPoint/ScreenRect doubles, Camera2D(GridConfig,ScreenRect), ResetFit,
TryPan(ScreenPoint), TryZoom(ScreenPoint,double), TryToWorld(ScreenPoint),
TryToScreen(Position), GetViewport/GetScale/GetConfig cheap observers. FieldTool
{attract,repel,remove} and settings {tool,slot,radius,magnitude} build existing FieldEdit
through TryBuildFieldEdit(settings,Position,capacity). Attract/repel require positive
finite radius/magnitude and explicit valid slot; remove yields canonical remove.
The backend uses existing CommandIngress::AdmissionStatus, never a second input protocol.

Prototype controls: 1/2/3 select attract/repel/erase; Tab selects one of four slots;
left-click world submits one edit, Delete removes selected; middle-drag pans; wheel
zooms; F fits; Space pauses/resumes; R restarts; Escape cancels preview; close quits.
UI buttons provide tool/slot/pause/restart actions too. Attract/repel start radius8,
magnitude4. No continuous motion-to-command admission or guessing free slots from
stale frames. Preview and queued feedback are distinct from committed rings.
Accepted edits confirm only after successful boundary trace and owned capture;
paused edits stay queued until resume. Rejected input retains editable preview.

InspectorSession owns Simulation, its borrowing HeadlessSession/ClockDriver and
owned ScenarioSnapshot in destruction-safe order. Restart constructs a fresh complete
run before retiring the previous run, then resets UI sequences, camera and time
baseline. Queued edits never cross runs; failure constructing replacement preserves
old run. Events/update/draw/teardown belong to one coordinator/main thread.
SDL callbacks return each iteration to the platform event loop. Background/minimize
suppresses elapsed catch-up and world input; foreground restores a fresh baseline,
preserving deliberate pause. No producer thread or asynchronous ECS observation.

## Rendering and platform scope

SDL3 stable3.4.18 full pin829a65d769d935c4852f8159e964312c0957260a,
optional CRUCIBLE_BUILD_DESKTOP=OFF. Plain headless builds neither fetch nor link SDL.
Existing ScenarioSnapshot is the owned ECS extraction boundary. Draw contiguous
infection/stock quads and sample quads in two reused SDL_RenderGeometry submissions;
no per-entity virtual dispatch, IRenderer or ECS-owned device/window components.
One concrete ScenePainter owns checked startup scratch and borrows inputs only for
a draw call. Host owns SDL init, window/surface, renderer, present and teardown.
SDL failure stops successful presentation. Prototype ASCII HUD is explicitly bounded
initial typography; no extra font/asset stack. Drawing may allocate inside SDL.

Public architecture targets Windows/Linux/macOS and iOS. Validate desktop packages
where runners exist; iOS packaging/touch/device lifecycle remains a separate actual
Apple-toolchain gate. Preserve a future contiguous GPU-instance upload path using
owned data; do not implement shaders, GPU retirement, a second backend or a generic
render graph before their consumer.

## Acceptance and cooperation

Workers read cpp-write/cpp-review and shared references, work from isolated branches,
claim exact paths and send shared requests; architect alone reserves heavy CPU.
Pure camera independent affine examples, invalid-preservation and slot/sign/canonical
command checks. Runtime queued/paused/admission overflow, stopped trace capacity,
restart fresh tick0/no old commands, close and full owned replay. Same painter software
fixtures inspect independently chosen pixels, clipping, layers, capacity rejection and
retained frame immutability; export actual frame for visual QA. Native SDL event smoke
exercises production mapping, controls, resize/minimize and close. Synthetic events
and software pixels do not replace device playtesting or claim iOS support.

Unfiltered Debug/Release/sanitizer gates preserve historical18 tests and legacy225000
checksum; optional desktop builds test pure/runtime/software/event consumers. Hosted
Linux/Windows and macOS desktop compile/tests gate actual supported claims. Capture
actual commands/results/platform limits, independent review and [phase4 review](../sprint-reviews/phase-04.md)
before publishing and merging the completed increment. Do not broaden into resource
growth/fusion, hierarchy, navigation or a relay outcome.

Carry P01-F04 (first input/display) into this delivered foundation; full playable gate
remains partial. P01-F03 fullW6, P01-F05 structural/concurrent/measured scale, P02-F02
concurrent exchange, P02-F03 terrain/world and P03-F01 measured fallback costs remain
with their existing accountable owners/gates. HX-07 stays a reproducible geometry pack.
