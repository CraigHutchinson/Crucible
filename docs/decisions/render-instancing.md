# Instancing after the owned-packet spike

2026-10-03. Design recommendation, not an implemented GPU pipeline. Consumes
[the interim spike](../../spikes/rendering/README.md) merged in
[PR 11](https://github.com/CraigHutchinson/Crucible/pull/11) at `0bc9731`.

## Findings carried into the architecture

Keep simulation as exclusive ECS owner. Extract contiguous owned values at completed
boundaries; adapters consume these without live ECS queries. Keep camera outside
world records. Retained packets survive ECS/snapshot changes, and camera-only redraw
does not repack world data. No entity-owned graphics resources, `Renderable` vtables
or generic `IRenderer` are justified by the existing consumers.

Production already submits two SDL world geometry batches. The spike **still expands**
records into vertices and indices. Neither implements GPU instancing. Instancing
targets CPU expansion and geometry transfer, rather than reducing hypothetical
150K draw calls: those per-entity calls do not exist in production.

At 150K samples plus 2,048 cells the candidate payload is 4,865,536 bytes versus
23,111,296 bytes of expanded quad submissions (4.75× arithmetic ratio). Both SDL
painters reuse scratch sized to the larger range; this is not resident memory or
measured bandwidth. Five process medians show an extra 0.779–0.791 ms packing after
1.924–1.962 ms capture on the shared Linux host. Timings use stationary tick-0 data
and exclude evolving simulation and all drawing/GPU work. See retained raw evidence.

## First real instanced consumer: proposed shape

One optional concrete GPU target owns a static unit quad (four vertices/six indices),
shared by instances. Bind per-instance world center/half extents and normalized sRGB
color. Issue separate cell and marker draws, with camera uniforms, world scissor and
marker half extents clamped to 2–5 logical pixels in the shader. Fields, HUD, mission
feedback and text remain explicit later passes. Phase 5 needs no new world records.

SDL's [indexed GPU draw](https://wiki.libsdl.org/SDL3/SDL_DrawGPUIndexedPrimitives)
accepts instance counts. Its [vertex-buffer description](https://wiki.libsdl.org/SDL3/SDL_GPUVertexBufferDescription)
selects vertex/instance addressing; reserved `instance_step_rate` must be zero.
Start with instance-rate attributes, zero draw base instance and explicit buffer
binding offsets for each range. SDL warns that nonzero base parameters interact
differently with built-in shader IDs across APIs; do not treat shader instance ID
as stable ECS identity. Add explicit IDs only for a consumed picking requirement.
These upstream capabilities are not Crucible device/shader acceptance.

The C++ 32-byte record (two float4s at offsets 0/16) is a candidate, not a shader ABI.
Compile real shaders and validate stride/formats, output encoding, sRGB conversion,
blend/order and geometry. Do not reinterpret normalized sRGB as linear. Graphics
layout stays out of Core/ECS. If packing matters, compare adapter packing with fused
owned extraction on the complete workload before adding another extraction surface.

Cell positions are mostly static while infection/stock color changes. Start with
a full bounded upload baseline. Split static geometry, compress colors or add dirty
ranges only after measured need. Swarm overdraw and screen-space density treatment
are separate costs that instancing alone cannot remove.

## Promotion gates

| Gate | Required consumed evidence |
|---|---|
| Upload ownership | Startup-bounded transfer/device slots; no overwrite until submitted work retires; explicit fence ownership/reuse |
| Saturation | Bounded wait/drop/backpressure policy; preserve last valid frame and report skipped publication; justify slot count |
| Shutdown/resize | Retire borrowed buffers before release, join future producers, handle failed/minimized output and replacement resources |
| Shader/ABI | Actual compiled shader, verified attribute layout and color/output/order fixtures |
| Numeric domain | Nonbinary, tiny/large worlds, fitted endpoints, strict exterior rejection, zoom/pan/clipping fixtures; stated tolerance or exact production behavior |
| Workload/device | Actual receiving frames on relevant Metal/Vulkan/D3D12 devices; input-to-present and evolving tick/capture/pack/upload/present p95/p99, memory/allocations at 2K/100K/150K |

SDL [uploads](https://wiki.libsdl.org/SDL3/SDL_UploadToGPUBuffer) execute in GPU timeline
order and offer cycling; cycling does not establish bounded application retirement.
[Fence waits](https://wiki.libsdl.org/SDL3/SDL_WaitForGPUFences) provide completion.
Choose slot reuse around submitted work, not CPU packet lifetime. None of this GPU
synchronization ran in the interim spike.

Fp32 half-cell size underflows at `denorm_min`, a valid world supported by production's
double projection. Center/half rounding can also differ from direct edges. Investigate
normalization/origin rebasing or another representation before promotion; never silently
narrow valid `GridConfig` support. Keep production's numerical oracle and SDL fallback.

## Sprint and platform division

Phase 5 delivers a playable reclamation challenge using the verified SDL world pass.
It does not promote the extra compact copy. A subsequent rendering foundation sprint
can consolidate R1 real instancing and R2 numerical/interaction validation under one
owner. Split R3 public-platform receivers when the consumed shader/resource contract
is stable; do not create several empty backend implementations or permanent platform
agents. Build-target selection is sufficient until real implementations require a
shared runtime protocol.

Public priorities are iOS/macOS, Windows and Linux. Apple packaging/signing/touch and
lifecycle require actual receiving/device tests. Future restricted platform adapters
preserve capability and ownership contracts; SDK details stay outside public artifacts.
Carry P04-F01/F02/F03 and P02-F02. A compiled shader/software fixture does not close
device scale, human readability or mission difficulty tuning.

## Faction extensibility checkpoint

Follow [faction/material/control separation](faction-extensibility.md) at the first
GPU contract: arbitrary per-instance palette values; cell/marker ranges represent
geometry behavior, not exactly two factions. Future allegiance/relations/authority
stay in simulation policy and owned observation, not shaders or renderer ECS queries.
