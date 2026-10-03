# Interim rendering architecture spike

2026-10-03. Experiment on main baseline `1d47fa6`; this is guidance for later work,
not a production renderer migration or the next gameplay sprint.

## Decision

Keep ECS simulation, owned extraction and concrete backend consumers. Do not add
per-entity drawing interfaces or an `IRenderer` hierarchy. The existing
`ScenarioSnapshot` is the authoritative consumed frame boundary. A world-space
instance packet is a plausible **GPU upload representation**, but adding this
second copy to the SDL path has a cost and no demonstrated rendering benefit.
Leave production `ScenePainter`, `Camera2D` and the inspector unchanged.

The executable supplies a second concrete SDL consumer to test the candidate
data boundary. It proves CPU ownership and selected raster behavior, not a second
graphics backend. Build-target selection remains sufficient until real consumers
establish a need for runtime dispatch. See the [production decision](../../docs/decisions/phase4-rendering.md).

## Consumed experiment

`WorldPacket` owns startup-sized cell and marker arrays. Each 32-byte, 16-byte-aligned
record contains float4 center/half extents and float4 normalized sRGB RGBA. Layout
assertions establish the C++ record, **not shader ABI compatibility**. A GPU shader
must explicitly choose color conversion/output encoding and validate its layout.

`TryPack` copies a completed snapshot, retains no snapshot/ECS references and
rejects invalid geometry/capacity before mutation. Borrowed record spans expire
at the next successful pack or destruction. Copy/move are deleted so ownership
cannot leave moved-from metadata describing missing storage. All packing storage
is allocated at startup. There is one sequential coordinator, no frame exchange.

Camera state stays outside the packet. `PacketPainter` projects a retained packet,
clamps marker half extents to 2–5 screen pixels and expands each range into reusable
SDL vertices/indices. Cells and markers remain two geometry submissions. It draws
only the world region of the fixed logical canvas; production fields, HUD, picking,
window events and presentation remain outside this experiment. The packet carries
no stable entity IDs and cannot yet serve a picking consumer.

Verification compares every RGBA byte in the world viewport with production
`ScenePainter`: unit-cell 64×32 grid, 2,048 samples at ticks 0 and 20, fit view,
then zoom ×2 and pan (37, −19). A retained tick-0 packet still draws the original
pixels after simulation advance and snapshot recapture. Camera-only redraw does
not change packet data. Tests also cover uncaptured/capacity rejection, unchanged
packet on half-size underflow, mismatched camera rejection before canvas mutation,
and painter capacity overflow. Measurement cases additionally compare the initial
150,000-sample fit view. This is fixture equivalence, not all valid grid/camera inputs.

## Measurements

Five independent Release processes, GCC 13.3, `-O3 -DNDEBUG`, shared Linux Xeon
8573C host; 20 warmups and 200 timed repetitions per case. The simulation stays
at tick 0, fields disabled, 64×32 unit grid. Timings cover snapshot capture and
subsequent packing separately. They exclude simulation, projection, SDL expansion,
draw, upload, present, GPU synchronization and full-frame latency. No affinity,
frequency control, regression threshold or 60 FPS conclusion. Raw process output,
compiler/cache/pins, source hashes and host metadata are in [evidence](evidence/metadata.json).

| Samples | Capture process median range | Pack process median range | Pack p95 range |
|---:|---:|---:|---:|
| 2,048 | 19.60–20.25 µs | 13.65–14.08 µs | 13.85–17.59 µs |
| 150,000 | 1.924–1.962 ms | 0.779–0.791 ms | 1.073–1.315 ms |

| Samples + 2,048 cells | Candidate record payload | Expanded vertex/index payload |
|---:|---:|---:|
| 4,096 records | 131,072 bytes | 622,592 bytes |
| 152,048 records | 4,865,536 bytes | 23,111,296 bytes |

The 4.75× payload ratio is arithmetic: 32 bytes versus
`4*sizeof(SDL_Vertex) + 6*sizeof(int)` = 152 bytes per quad on this toolchain.
Expanded totals sum both submitted ranges; production and spike painters reuse
scratch sized to the **maximum** range. These totals are neither resident-memory
measurements nor measured transfer bandwidth. The spike adds retained records
alongside the existing snapshot and SDL scratch. Only an actual instanced GPU
consumer can establish whether avoiding expansion/upload offsets that copy.

## Findings and limits

| Finding | Consequence for later work |
|---|---|
| Owned packets survive live ECS/snapshot mutation | Keep explicit extraction ownership; this establishes CPU lifetime only |
| Camera redraw needs no repacking | Keep camera uniforms/state separate from world instances |
| Two contiguous ranges reproduce tested world pixels | Prefer batches over per-entity virtual drawing |
| Packing adds roughly 0.8 ms median at 150K on this host | Measure fused extraction versus the extra copy with the real GPU consumer |
| `denorm_min` cell half-size underflows in float | Candidate rejects an otherwise valid tiny world; cannot replace production geometry wholesale |
| Float centers/half extents round differently from direct double edge projection | Add nonbinary, large/small world, edge and zoom fixtures before promotion; consider normalized coordinates/origin rebasing |
| Fields/HUD/IDs are absent; glyphs and overlays have other needs | Keep separate passes; let actual consumers determine shared records/contracts |
| No device, shader, transfer or fence ran | No GPU, iOS or console acceptance claim; upstream API support is insufficient |

## Next work packages

Consolidate the renderer experiment under one owner while the contract is unsettled;
keep gameplay implementation independent. Split platform receiving work after a
real backend produces a frame, rather than creating several empty backend interfaces.

| Package | Owner / consumed result | Promotion gate |
|---|---|---|
| R1: real instanced backend | Rendering owner; one concrete GPU target draws this world fixture using camera uniforms and an actual shader | Shader layout/color, clipping/precision and overlay order fixtures; bounded upload storage and explicit in-flight retirement; measure complete frame and extra-copy versus fused extraction |
| R2: numeric and interaction contract | Architect with renderer; expand edge/nonbinary/extreme grids and camera tests; add IDs only if consumed by picking | Specify acceptable geometric tolerance or preserve exact production behavior; never silently shrink valid world support |
| R3: public platform receivers | Platform owner after R1; macOS/iOS, Windows and Linux builds/devices | Actual device frame and lifecycle/resize/resume, features/fallback, signed mobile package/touch and distributable runtime evidence |

R1/R2 can share one initial workstream. R3 can split by toolchain when the consumed
pipeline is stable. Capability-driven backend seams can accommodate future platforms;
restricted platform integration and SDK details belong outside public artifacts.
Carry existing P04-F01/F02/F03 and P02-F02 unchanged. This spike does not close
packaging, typography/accessibility, full-tick budgets or device follow-ups.

## Reproduce and acceptance

```sh
cmake --preset spike-release
cmake --build --preset spike-release --parallel 4
ctest --preset spike-release
build/spike-release/spikes/rendering/crucible_render_spike --measure
# Optional world-only BMP, distinct from the production inspector:
build/spike-release/spikes/rendering/crucible_render_spike --export world.bmp
```

`spike-debug` and `spike-sanitize` are also available. On macOS use the established
`macos-debug`/`macos-release` presets and configure with
`-DCRUCIBLE_BUILD_RENDER_SPIKE=ON` to select the complete C++23 toolchain. The option
defaults off and requires `CRUCIBLE_BUILD_DESKTOP=ON`; headless graphs stay SDL-free.
CI opts in on desktop and sanitizer jobs. Local Release/Debug full suites and the
focused ASan/UBSan spike check are recorded with the PR checks. Local leak scanning
is disabled because the managed host blocks process inspection; hosted sanitizer
CI retains its normal configuration. A green software fixture on each public
desktop OS is distinct from hardware/device validation.
