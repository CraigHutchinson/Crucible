# Phase 6 concrete offscreen instancing

The optional Vulkan/SPIR-V receiver consumes owned InstancePacket values extracted
from ScenarioSnapshot. It does not replace the SDL interactive desktop or add an
ECS query, faction model, renderer interface or native swapchain. Root owns device
creation, shader compilation/provenance, diagnostic callers and build/CI execution.
This document states the frozen contract; integration records executed evidence.

## Numeric and shader contract

Instance is 32 bytes: normalized center/half extents in float4 at offset 0, opaque
normalized sRGB RGBA in float4 at offset 16. Normalization divides physical values
in double before narrowing; dividing cell size by extent before halving preserves
physical denorm_min cells. Markers store normalized centers and zero halves because
camera-dependent marker size is a separate uniform, clamped to 2..5 logical pixels.
Camera origin and full-world projected extent are narrowed only after finite bounded
logical projection. Camera input rejection and physical geometry are unchanged.

Vertex locations 0/1/2 are unit-quad float2, geometry float4, color float4. Quad stride
is 8; instance stride is 32, input rate INSTANCE and reserved step rate zero. There
are four static quad vertices and six uint16 indices. Cells draw first, then markers,
with binding offsets for each contiguous range and base instance zero. Shader IDs
never represent stable entities. GLSL450 vertex uniforms use SPIR-V set1/binding0;
fragment has no resources. Pixel corners clamp to {24,96,1232,520} before conversion
to a 1280x720 clip space with +Y up; viewport coordinates remain +Y down.

The offscreen RGBA8_UNORM target stores the supplied encoded sRGB channels directly;
there is no implicit sRGB target conversion, transparency or blending. Arbitrary
finite RGB in [0,1] with alpha exactly one is accepted per record. Empty color spans
use the established infection/stock/nanite palette; supplied spans must match their
range exactly and are copied. Three cell and three marker colors form a synthetic
presentation fixture, not multiple playable factions. Cell/marker ranges describe
geometry policy, not teams.

Frozen before GPU readback: shader logical vertices must remain within 0.01 logical
pixels of independent double production projection after clipping. Interior readback
channels allow at most one byte difference from the actual software painter. A
one-logical-pixel mask around independently derived cell/marker edges accounts for
rasterization rules; it is not derived from observed mismatches. Tests require at
least one quarter of world pixels remain unmasked. These criteria are not loosened
after observing a driver result. Unsupported conversion explicitly rejects before
packet mutation; the existing SDL path preserves production geometry support.

## Ownership and receipt lifecycle

InstancePacket allocates cell/marker vectors once; successful capture owns all
records, grid and tick. Rejection preserves the previous packet. Borrowed observers
expire on successful capture/destruction, and copying/moving the owner is deleted.
Counts and combined byte arithmetic fit SDL Uint32 buffer sizes before allocation.

OffscreenRenderer borrows a device that must outlive it and owns pipeline/shaders,
static geometry, and exactly three slots. Every slot owns upload transfer storage,
instance device storage, a fixed color target, download transfer storage and fence.
All cycling flags are false. Each submission copies both ranges into owned transfer
memory, unmaps, encodes upload, indexed instance draws and download, then obtains a
submission fence. Input/shader/camera or mapped driver pointer never escapes or
remains borrowed from the caller.

Submitted receipts are distinct from completed readbacks. Query all occupied fences;
when all remain busy, return busy without changing slot data/ownership. A completed
slot may be reused, expiring its earlier ID. Startup-owned shared identity with weak
receipt identity rejects other renderer contexts and expires after destruction, even
if addresses are reused. No per-submission identity allocation is needed. Capacity,
camera or short-destination rejection occurs before mutation; pending/expired reads
leave destination bytes unchanged. PollReadback copies RGBA bytes only after fence
completion. Device failure latches the renderer unusable and never publishes a
failed submission as a frame.

TryDrain is a blocking shutdown/replacement barrier; there is no SDL fence timeout.
A hung driver is bounded externally by the process/test timeout, not a promise of
responsive teardown. Destructor drains before any release; if drain fails it logs
and terminates rather than releasing resources while submitted work can still use
them. Constructor partial creation uses the same ownership cleanup, including startup
quad submission. Replacing fixed output constructs another concrete receiver and
drains the old one; no unused mutable-size API is introduced. Native acquisition/
cancellation, public hardware receivers and GPU/SDL window interop remain outside
this increment.

## Tests and receiving evidence

Portable tests require no SDL: independent nonbinary/denorm/maxfloat/fit/zoom/pan
projection fixtures, arbitrary color ownership, rejected capture retention, checked
capacity arithmetic, and deterministic three-slot delay/busy/reuse/identity checks.
The optional executing Vulkan fixture requires compiled vertex/fragment SPIR-V paths;
a missing device is a failure, not a skipped success. The pinned SDL Vulkan loader
requires an initialized video backend even for an offscreen device; hosted Linux
uses X11 under Xvfb, without a window or swapchain. It reads arbitrary palette
interiors, tests retained packets after later snapshot capture, short/foreign/expired
receipts, malformed-shader construction cleanup and context replacement. Initial and
evolved 2048-sample world frames, nonbinary geometry, tiny/large worlds and camera
changes compare against the actual production software painter with the frozen masks.

Root's diagnostic consumes this same API and records tick, mission/ledger, commands,
shader/device provenance and actual capture. Software Vulkan proves shader/API/upload/
fence execution, not physical-device performance or Metal/D3D12/iOS acceptance. Full
readback occurs every diagnostic submission and is not a production performance path.
Dirty ranges, compressed records, fused extraction, GPU overlay passes and concurrent
frame exchange remain deferred. Combined validation and exact commands are recorded
by integration; this worker does not claim unexecuted builds or device results.
