# Rendering decision: owned ECS frames and concrete batched adapters

Decision2026-10-03; receiving consumer is phase4 interactive inspection/control.
Public platform requirements are iOS/macOS, Windows and Linux.

## Data architecture

Simulation remains the exclusive owner of ECS storage. At a completed boundary,
ScenarioSnapshot copies sorted samples, field slots, infection, stock and the ledger
into owned contiguous storage. Drawing reads that frame, never ECS queries/views.
This is the existing consumed extraction boundary; a second generic RenderFrame or
per-entity Renderable/Draw interface is unnecessary. Camera/UI data remains separate
from world/game state. All initial capture/events/drawing are sequential.

The renderer converts contiguous frame values into reusable batches. Cells and sample
markers use two untextured geometry submissions, with field rings and compact HUD
layers. ScenePainter is concrete because it owns bounded vertex/index scratch and
capacity invariants, not because the design needs runtime polymorphism. The native
window and software fixture consume the same painter. No IRenderer, platform vtable,
component-owned GPU resource, abstract render graph or per-nanite draw dispatch.

## Backend choice and alternatives

| Choice | Fit now | Consequence |
|---|---|---|
| SDL3 2D rendering/events, chosen | Native public desktop platforms, software pixel fixtures, geometry batching, DPI mapping and app callbacks | Minimal live consumer; prototype visuals/typography, no measured scale claim |
| SDL3 GPU instance pipeline, later candidate | Metal/Vulkan/D3D12 and contiguous data uploads | Requires consumed shaders/formats, device features, transfer lifetime/retirement and controlled full workload |
| Direct native graphics APIs | Maximum backend control | Several implementations/toolchains before a demonstrated requirement |
| Generic renderer abstraction now | No second concrete consumer | Extra interfaces/protocols without current ownership or testing benefit |

SDL3 stable3.4.18 is pinned at829a65d769d935c4852f8159e964312c0957260a.
Optional desktop targets contain SDL declarations; ordinary Core/Runtime/snapshot
and headless builds stay backend independent. A future concrete implementation can
be chosen by its build target and consume the same world-space values. Runtime
selection/interfaces are considered only after two real implementations establish
which operations actually vary. SDL 2D is a prototype adapter, not a commitment to
expand it into a custom low-level GPU API.

Official sources: [release](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18),
[geometry](https://wiki.libsdl.org/SDL3/SDL_RenderGeometry),
[software renderer](https://wiki.libsdl.org/SDL3/SDL_CreateSoftwareRenderer),
[high DPI](https://wiki.libsdl.org/SDL3/README-highdpi),
[logical presentation](https://wiki.libsdl.org/SDL3/SDL_SetRenderLogicalPresentation),
[app callbacks](https://wiki.libsdl.org/SDL3/README-main-functions),
[GPU architecture](https://wiki.libsdl.org/SDL3/CategoryGPU) and
[iOS source guidance](https://github.com/libsdl-org/SDL/blob/829a65d769d935c4852f8159e964312c0957260a/docs/README-ios.md).
Upstream capability is not a claim that Crucible's package has passed on every device.

## Coordinates, lifetime and platform progression

Use a fixed1280x720 logical canvas, letterboxing and one window-to-render conversion
before pure camera inversion. Camera preserves world aspect ratio, pan/zoom center
and input rejection; high DPI changes native presentation, not command world units.
World viewport {24,96,1232,520} leaves separate HUD/tool strips. Minimized/zero output
suppresses world commands/drawing; timer baselines prevent accumulated resume ticks.

App callbacks yield each frame to the platform. On iOS this fits system lifecycle
better than an endless desktop-owned loop; actual bundle/signing/touch/device tests
are still required. SDL_AppEvent can be concurrent when another thread pushes events:
this application introduces no producer thread and guards callbacks before touching
coordinator state. Future producers require bounded owned ingress and joined lifetime,
not arbitrary callback access to ECS or the renderer.

Host destruction order is renderer, backing window/surface, then SDL subsystem.
Painter scratch contains no retained renderer/frame pointers. GPU command/resource
retirement and concurrent frame exchange are separate future ownership gates; buffers
must never be recycled while a device or reader still borrows them. Public desktop
CI, software fixtures and window smokes each establish different evidence. Touch
controls, mobile packaging, suspension on real devices and accessible final typography
remain explicitly unfinished until consumed and validated.
