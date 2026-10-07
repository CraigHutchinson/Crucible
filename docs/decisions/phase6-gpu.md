# Phase 6 GPU receiving contract

2026-10-03. Frozen before shader/readback observation; implementation receiving
results belong in the [Phase 6 review](../sprint-reviews/phase-06.md).

Vulkan/SPIR-V, offscreen 1280x720 RGBA8_UNORM. One static four-vertex/six-index
quad, two material ranges (cells/markers), instance-rate attributes; no faction
branch. Owned 32-byte record: normalized world center/half extents then opaque
normalized sRGB float4 at offsets 0/16. Normalize in double before narrowing;
marker half comes from camera uniform clamp(scale*.08,2,5). Cells' normalized half
avoids physical denorm half underflow. Strict exterior coordinates reject before
mutation. Unsupported conversion keeps the established SDL renderer available.

Camera is separate: logical origin/world projected extent plus canvas/marker mode
in two std140 float4s. Clipped vertex error against independent double geometry
is at most .01 logical pixel. Readback interior channels differ by at most one
byte; exclude only analytic one-pixel geometry boundary masks. These criteria may
not be loosened after results. Opaque colors have finite RGB[0,1], alpha1; shader
writes encoded channels to UNORM without implicit sRGB conversion. Transparency
is future explicit blending/order work. Synthetic three-marker/three-cell palettes
are presentation tests, not faction gameplay.

Exactly three startup-sized slots own upload/device buffer, output texture,
download and completion fence. Query fences; all-busy returns a skipped receipt
without overwriting occupied data. A successful submit does not mean readback or
presentation completed. Receipt identity includes the renderer lifetime, slot
generation and tick; expired/foreign/reused receipts cannot read another frame.
No packet, camera, shader or mapped-buffer borrow survives calls. Device caller
outlives renderer. Blocking drain is teardown only; failure refuses live resource
release and a hung driver needs an external process timeout. Core has no swapchain.

Tool provenance: local Ubuntu snapshot packages glslang15.1.0 and Mesa25.2.8,
llvmpipe Vulkan1.4.318/LLVM20.1.2. Packages downloaded/extracted in task workspace
when managed system cache/uid operations were unavailable. Checked GPU loader
execution with vulkaninfo. GLSL450 compiled using glslangValidator -V --target-env
vulkan1.0, entry main; source shaders are versioned and SPIR-V generated at build.
CMake retains compiler identity; receiving metadata retains exact compiled hashes
and device details. CI provisions explicit tools and requires a software Vulkan
receiver; missing driver/readback fails acceptance.

No physical GPU, Metal/D3D12/iOS package, human playtest, networking, density pass,
100K/150K complete frame or FPS result is implied. The optional diagnostic uses
production InspectorSession snapshots; default interactive SDL/headless remain.
