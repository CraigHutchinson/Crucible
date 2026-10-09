# Phase15 graphics framework qualification

Status: source-reviewed candidate selection,2026-10-09. No framework was built,
installed or executed on the GPU in this investigation. User approval covers
production implementation; this gate resolves the newly requested platform scope
before dependent renderer code is dispatched.

## Architect decision

Use an existing rendering framework. Select **Filament1.77.3** as the first bounded
fidelity receiving candidate, with **Vulkan on Windows/Linux** and **Metal on
macOS/iOS**. Its material/lighting/post-processing system addresses the actual
silver/cyan nanite concept directly. This avoids making a new graphics abstraction
and a new PBR pipeline prerequisites for the intro/menu.

Keep native DX12 explicitly in scope for qualification. Filament's directly listed
backends do not include native DX12. Its WebGPU route is not evidence that this
application has received DX12. Evaluate a matching Dawn/WebGPU route, with
wgpu-native as the low-level alternative if native DX12 or the instancing/error
contracts prove incompatible. Do not call an API newer/faster without same-consumer
measurements, and do not ship multiple complete renderer frameworks by default.

Existing SDL drawing stays concept2D/dev-test only. SDL window/events remain a
possible platform provider. Production has no SDL_Renderer fallback. Dear ImGui
is the strongly themed production UI; its graphics adapter follows the selected
framework, independently of the platform input backend.

## Alternatives and evidence

| Candidate | Relevant strength | Actual qualification cost / limit | Source |
|---|---|---|---|
| Filament1.77.3 | PBR/IBL, shadows, HDR, bloom, DOF, native C++ and Apple Metal | No directly listed native DX12; exact instancing, UI ownership/upload and failure adaptations below | [Capabilities](https://github.com/google/filament), [release](https://github.com/google/filament/releases/tag/v1.77.3) |
| wgpu-native29.0.1.1 | DX12/Vulkan/Metal behind a C ABI; WGSL and observable submission/completion | Rust source toolchain or qualified binary packages; our own lighting/post-processing; matching ImGui/API revision | [Release](https://github.com/gfx-rs/wgpu-native/releases/tag/v29.0.1.1), [platform builds](https://github.com/gfx-rs/wgpu-native/blob/trunk/.github/workflows/cd.yml) |
| Dawn | C++ WebGPU, DX12/Vulkan/Metal and shader translation | Larger build/dependency closure; upstream iOS support is best effort | [Support](https://github.com/google/dawn/blob/main/docs/support.md), [CMake](https://github.com/google/dawn/blob/main/docs/quickstart-cmake.md) |
| bgfx | Mature C++ DX12/Vulkan/Metal, instancing and shader tools/examples | Custom fidelity pipeline and reviewed ImGui adapter; asynchronous/fatal model and limited native presentation observability | [Capabilities](https://github.com/bkaradzic/bgfx), [API](https://bkaradzic.github.io/bgfx/bgfx.html), [UI example](https://github.com/bkaradzic/bgfx/blob/master/examples/common/imgui/imgui.cpp) |
| SDL_GPU | Actual DX12/Vulkan/Metal GPU API, unlike the 2D painter | Still requires our material/post pipeline; acquire/submit cancellation and checked-UI seams; not selected under current production direction | [GPU documentation](https://wiki.libsdl.org/SDL3/CategoryGPU) |
| Diligent | Modern graphics abstraction with DX12/Vulkan | Native Metal availability has a commercial qualification; open-source Apple routes need separate MoltenVK/package evidence | [Core](https://github.com/DiligentGraphics/DiligentCore) |
| Separate native DX12/Vulkan/Metal adapters | Maximum direct API control | Three resource/shader/synchronization/platform implementations before receiving one polished cinematic | [ImGui native backends](https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md) |

A audited framework/platform/runtime contracts; B independently audited UI,
typography, mobile layout and capture. Root checked consequential claims against
the exact release source. The requested Sonnet/Terra research class is unavailable
in the enabled agent model catalog; the two existing workers supplied bounded
source evidence, with root assessment retained. No additional worker was created.

## Exact candidate and toolchain gate

Verified tag/source: `d852e34cd5629a6f3851d613dcd17aa0ffd025a2` (v1.77.3).
Read-only sparse source is retained in `build/phase15-sources/filament`. This pin
bundles **ImGui1.92.5**, independently verified in its header. The separately
fetched1.92.6 candidate must not be linked into the same context/backend ABI.

Configure the dependency in an isolated build. Upstream defines generic targets,
tools and directory compiler settings; do not import its entire build policy into
the game. Explicitly enable Vulkan, use MSVC, and set USE_STATIC_CRT=OFF to match
the current MD/MDd runtime. Import a reviewed runtime archive/header closure and
keep material/compiler tools separate. Exact cl19.51 configure/link is pending.
[Pinned build settings](https://github.com/google/filament/blob/d852e34cd5629a6f3851d613dcd17aa0ffd025a2/CMakeLists.txt).

Runtime closure starts with filament/backend/math/utils/filaflat/filabridge/zstd;
Vulkan includes bluevk/smol-v and required headers. Windows utilities add getopt
and Shlwapi. Resolve the actual final link rather than assuming this list is
complete. Host tools matc/cmgen/resgen must match the runtime release. Disable
unconsumed sample/test/SDL2/OpenGL/WebGPU/debug targets in the first candidate.

The release metadata advertises a Windows archive of855049603 bytes with SHA256
`e8705d817b7a884fa2d185080ceb80dd42c4ba339930d49dedc79b5761215285`.
It was not downloaded or inspected. An asset listing is not a verified executable
or runtime/tool match. Prefer the bounded isolated source/tool receiving that
preserves exact compiler/runtime flags and checks every imported artifact.

Local read-only inventory found Intel Graphics driver32.0.101.9033 and NVIDIA RTX
5070 Laptop GPU driver32.0.16.1714. VulkanSDK1.4.341.1 provides DXC/glslang tooling.
This establishes inventory only; record the actual selected adapter/backend at
native startup. macOS/iOS hardware, package signing and physical tests remain
explicit scoped gates; a Windows capture cannot close them.

## Release-pinned contract findings

1. **Instancing:** automatic transform instances are bounded by
   getMaxAutomaticInstances, usually64. Manual counts silently clamp to1..32767.
   The adapter must validate and explicitly batch; receive distinct payloads at
   1/64/65/4096, then the150K path before claiming scale. An automatic64-instance
   implementation alone cannot establish future large-swarm performance.
   [Pinned contract](https://github.com/google/filament/blob/d852e34cd5629a6f3851d613dcd17aa0ffd025a2/filament/include/filament/RenderableManager.h).
2. **UI ownership/uploads:** the helper accepts an external context but destroys
   it; it also changes style/ini policy and allocates per-list VB/IB staging. Use a
   narrow attributed adapter preserving root-owned context/theme and bounded
   reusable callback-owned upload storage. Keep the release's license/provenance.
   [Pinned helper](https://github.com/google/filament/blob/d852e34cd5629a6f3851d613dcd17aa0ffd025a2/libs/filagui/src/ImGuiHelper.cpp).
3. **Failure policy:** exception handling is gated by __EXCEPTIONS. Verify actual
   MSVC _CPPUNWIND/macro behavior before promising catchable errors. Otherwise
   document process-fatal paths, or receive a small consistent adaptation across
   the dependency closure. A build option alone is insufficient.
   [Pinned Panic](https://github.com/google/filament/blob/d852e34cd5629a6f3851d613dcd17aa0ffd025a2/libs/utils/src/Panic.cpp).
4. **Completion:** the public Fence establishes render-thread command-stream
   progress. flushAndWait adds driver.finish and purges callbacks, but the pinned
   Vulkan finish discards queue-wait results. Its true return does not establish
   error-checked GPU success under loss. Qualify drain receipts honestly or receive
   a minimal checked adaptation; do not inherit a DX11 HRESULT guarantee.
   [Pinned finish](https://github.com/google/filament/blob/d852e34cd5629a6f3851d613dcd17aa0ffd025a2/filament/backend/src/vulkan/VulkanDriver.cpp),
   [Engine](https://github.com/google/filament/blob/d852e34cd5629a6f3851d613dcd17aa0ffd025a2/filament/src/details/Engine.cpp).
5. **Frame/capture:** true beginFrame obliges endFrame; validate scene/UI/resources
   before accepting it. endFrame schedules display. Readback keeps client storage
   until callback and may need more frame pumping; use supported formats and
   settle callback/drain before release. Capture must include the composed scene
   and UI, with source/frame/surface identity. No scanout or native handoff claim
   follows from scheduling or a completed image.
   [Pinned renderer](https://github.com/google/filament/blob/d852e34cd5629a6f3851d613dcd17aa0ffd025a2/filament/include/filament/Renderer.h).

These findings are source qualifications, not induced physical faults. They are
stop/receiving conditions before production promotion, rather than arguments for
silently weakening an existing performance or ownership gate.

## First work package and Sub0 disposition

P15-R0 is a concrete framework-backed ProductionRenderer and its receiving caller:
one beveled silver/cyan wedge, industrial plate, depth, restrained bloom and themed
ImGui in one real window. Add bounded4096 instancing, actual source-backed captures,
resize/minimize/teardown and controlled failure/storage tests before full logo work.
B can author portable choreography/UI against reviewed values; root owns platform,
application/profile, package/pins, resources, hardware and publication.

The reusable boundary is bounded scene/upload ownership, resource retirement,
backend capabilities and honest completion/capture receipts. Keep that compartment
local during first consumer receiving. A later Sub0Render extraction needs actual
product-neutral consumers, independently received lifetime/failure/package tests
and a stable interface. Do not put Crucible logo, materials, menu or progression
policy in a shared library, and do not build a competing RHI merely to create one.

Stop expansion on tool/ABI mismatch, silent instance clamping, unsettled callbacks,
missing required capabilities, unsafe failure semantics or unacceptable whole-frame
cost. Compare the alternative on the same lit scene, UI, camera and instance data.
First quality/native proof precedes a qualified performance campaign. Phase14
G3-G5 remain unchanged and open; this candidate has no performance promotion.
