# Phase15 production rendering and UI boundary

Decision: production boundary selected; framework candidate qualification,
2026-10-09, baseline `13642fe`. This records user steering and source feasibility,
not completed hardware proof. The initial DX11-only proposal is superseded by
the requested DX12/Vulkan/macOS/iOS scope.

## Selected direction

Keep `crucible_desktop`, ScenePainter and existing SDL software/native fixtures as
the concept2D/dev-test platform. Add a separate `crucible_game` production consumer.
It owns one window throughout intro, menu and later missions. SDL supplies window
and event handling, not production SDL_Renderer drawing. A concrete production
adapter owns the selected graphics framework, scene/material resources and frame
submission. Unsupported production capability is reported explicitly;
there is no automatic switch to the prototype renderer.

This replaces the earlier proposed D3D11-to-SDL-target interop route. Pinned SDL
binds its vertex shader/input layout/constants at initialization; flush cache
invalidation does not itself restore every modified native state. An independent
production device also gives a clear lifetime and migration boundary.

The optional existing Vulkan receiver remains its own offscreen world receiving
tool. Its flat shaders and fixed readback contract do not establish a production
cinematic backend. Windows/Linux Vulkan and macOS/iOS Metal are now first-phase
targets. DX12 remains explicitly scoped qualification, not an inferred capability.
The [framework qualification](phase15-framework-qualification.md) selects
Filament1.77.3 as the first fidelity candidate and defines alternatives/stop gates.
Do not author a new virtual hardware hierarchy for all candidates.

## UI reuse

Use one matched Dear ImGui core plus the SDL3 **platform** backend and the selected
framework's reviewed graphics adapter. Do not compile an SDL_Renderer backend
into production.
The upstream [backend guidance](https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md)
separates platform input from graphics rendering.
Its [MIT license](https://github.com/ocornut/imgui/blob/master/LICENSE.txt) is retained.
The release-pinned Filament candidate bundles ImGui1.92.5. Use that matching core
for its adapted UI helper and platform backend. The earlier fetched1.92.6 source
is an audit alternative; linking both would create an ABI/duplicate-symbol hazard.

Root owns ImGui context, event forwarding, frame sequencing and screen transitions.
A adapts the selected framework UI renderer and consumes generated draw data
after the 3D scene. B authors the Crucible theme, layout, visible focus,
licensed fonts and typed intents. Use one viewport, keyboard navigation and stable
controls; no docking, debug/demo windows, automatic cursor movement or hidden ini
persistence in the player flow. The themed UI must resemble the approved menu's
composition and material identity rather than a default development panel.

ImGui1.92 supports dynamic font work. Startup font loading is not evidence of
allocation-free later glyph baking. Freeze finite label/scale choices, receive
actual UI warm states and record CPU/upload/memory costs separately from the
simulation's strict hot-path allocation gate.

## Ownership and contracts

Only the application coordinator touches ImGui, SDL window and production adapter.
The framework's own render/driver threads are isolated from Runtime/ECS; callbacks
use owned bounded storage and publish receipts to the coordinator.
Root's window and ImGui context outlive renderer/backend teardown. The renderer
retains owned native resources; no caller pose span or draw-data borrow survives
a render call. Asynchronous descriptors retain adapter-owned staging until their
completion callback. Join/purge before storage destruction or replacement.
Scheduling, GPU completion, native presentation and physical scanout are distinct
receipts. A public API that only schedules a frame must report that honestly.

Root defines the shared scene/frame and frontend view/intent values; A and B
review them before dependent implementation. Choreography owns startup-sized
decorative poses (at most4096), finite camera values and exact authored CRUCIBLE
placement. Renderer validates finite input and capacity before upload/draw. No
ECS handle, Runtime borrow or gameplay policy enters that packet. The same native
renderer later receives a bounded adapter from owned mission snapshots.

The frontend reads synchronous copied application values and returns at most one
intent per frame. Root validates the screen/transition identity before applying
it. Intro skip consumes its event; it cannot activate a menu control in the same
transition. Animation never delays ready menu input. Only active play pumps Runtime.

## First receiving package

R0 demonstrates one beveled silver wedge, cyan emissive core, finite perspective
camera/depth and themed native controls on the same production device. Then add
bounded instancing, industrial floor, restrained bloom/tone mapping and actual
twelve-second choreography. Compare macro, converging lanes, readable assembly,
hero hold and menu captures with the approved art; revise based on those images.

Required lifecycle coverage includes initialization/resource failure, capacity and
invalid input, minimize/zero output, resize/DPI/fullscreen, focus/skip, qualified
submission/drain/capture receipts and close while work is outstanding. Any native
handoff or GPU completion claim needs separate supported evidence; see the
framework qualification's release-pinned limits. Software-device fixtures
may receive logic/lifecycle but cannot claim physical visual/performance acceptance.
Shader/tool/device versions, source/pins, dimensions and captures accompany evidence.
