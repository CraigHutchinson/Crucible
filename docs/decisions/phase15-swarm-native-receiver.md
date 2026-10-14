# PR47 native swarm receiver: consumed R1 contract

2026-10-13. Builds on the separately published R0 production foundation (PR48)
and received PR47 source `6597f26f`, merge `1b9747b`. This package receives that
actual prototype through the real renderer; it does not redesign its assembly
or claim the later articulated rig/crystal studies are implemented.

## Named consumers and scope

`crucible_game` owns a separate decorative intro controller, then returns to the
existing GameJourney screens and mission world. The controller owns SwarmIntro,
preallocated RenderRow snapshots and a bounded CPU mesh producer. The renderer
copies complete mesh/UI inputs before return into callback-owned upload storage.
Neither decorative world nor renderer can mutate mission Runtime.

The default is the implemented Scaffold build,1536 units, original pace/close-up
pace and fixed1/60 presented-second ticks. Receive full sequence and menu hold,
immediate Skip, Replay, reduced-motion settled view, pause on suspension and
bounded catch-up. A receiving fixed-time option advances actual presented ticks;
it does not reinterpret milliseconds as choreography seconds. Actual drawable
aspect/width feed the prototype's camera/LOD at an explicit tick boundary.

## Camera and geometry

Retain the prototype's camera operator and rig. The reference `render_swarm.py`
projects oblique vertical position as `z + viewTilt * depth`, with yaw about z.
Convert z-up prototype coordinates to the renderer's right-handed y-up coordinates
as `(x,z,-y)`. For oblique view, camera target is `(cx,cy,0)`, eye offset follows
`(-sin(yaw),viewTilt,cos(yaw))`; use a finite distance inside current coordinate
bounds. Orthographic half-width is the actual camera halfWidth; half-height is
`halfWidth/aspect/sqrt(1+viewTilt^2)`. This preserves the reference affine screen
mapping rather than substituting a perspective approximation. Add an optional
orthographic extent to the existing camera, consumed by the concrete renderer.
Original perspective callers remain meaningful. Unsupported camera values refuse.

The CPU producer follows actual `NaniteBody.hpp` and `nanite_mesh.py`: morphed
triangular docking footprint, leaned shell apex, speed elongating only the nose,
one compact regular tetrahedral core clamped inside the shell, four shell faces
including the underside. Preserve headings/plane hinges and actual act metadata.
Render rails/shields/hubs at bounded detail; simpler shell windows still reveal
the same core at distance. Do not use the R0 wedge as the received body.

Mesh input is a synchronous triangle-list span: position3, outward unit normal3,
role (shield,rail,hub,core) and bounded intensity. Define one immutable role enum;
no caller-supplied shader/material handles. Renderer converts normals to its
native tangent representation and applies product-local silver/graphite/cyan PBR.
All positions/normals/intensity/counts are validated before beginFrame/upload.
The mesh producer owns reusable storage and reports capacity refusal explicitly.

## Frozen first capacities and memory accounting

At most4096 units; default1536 and two-layer3072 fit. Low detail is84 triangle
vertices/unit; detailed body228. At most128 detailed units, so at most362496
vertices (`4096*84 + 128*144`). Full-scene delivery uses32-bit identity indices,
one mesh draw with shader role selection, and an immutable startup index buffer.
No render-parts multiplier silently expands the nanite cap. Primitive normals
and nondegenerate triangles have independent fixtures/reference comparisons.

Three mesh upload slots reserve at most15MiB each including the existing R0
scene/UI staging (2MiB) plus native36-byte mesh vertices (12.4453125MiB). Native
vertices contain position3, tangent quaternion4 and role/intensity2 floats. No free
slot skips the whole combined frame before accepting beginFrame. Existing UI
vertex/list/command/texture action caps stay unchanged. One64MiB texture upload
batch and one64MiB readback remain the cold maxima. Allow16MiB additional adapter
metadata/immutable indices/cold environment staging. External renderer CPU
staging bound is192MiB; it excludes producer/ECS storage and Filament's internal
heap/GPU allocation. Record actual retained capacities separately from residency.

An environment/reflection receiving pass may improve metal readability using
bounded owned studio-light resources; missing reflective fidelity is a visible
open finding, not proof the single-unit concept has been matched.

## Ownership and gates

Root owns this contract, common headers/camera, manifests/pins/CI, app coordinator,
prototype receiving fixes, capture/evidence and serial CPU/GPU. Graphics owns
concrete mesh uploads/materials/native normal packing/validation and its fixtures.
Consumer owns proxy geometry/intro controller adapter and reference fixtures.
Each worker reviews the opposite surface; neither runs CPU/device work without
root handoff. Use an isolated `codex/production-swarm` worktree while PR48 runs CI.

Require independent plan review before dependent code, then exact-source mesh
oracle/property tests, both clocks and camera mapping fixtures, actual native
body/assembly/hero/menu captures, UI lifecycle/skip/replay receiving, source review,
exact-head platform/production CI and merge. Preserve original failed evidence.
Prototype smoothness receiving must reject nonfinite samples/derivatives; describe
the spring's coefficients without implying a no-overshoot proof.

## Plan-review freeze

Both workers independently verified the camera mapping and84/228 vertex arithmetic.
Replace/enlarge the existing three combined slots; do not add another three15MiB
slots beside them. Orthographic half-width is finite in(0.01,1024], half-height in
(0.01,32768]; coordinate bounds do not cap projection extents. Default Scaffold
uses oblique projection; lying-camera equivalence is outside this first receiver.

Triangle count is divisible by3; all three vertices share one valid role and
intensity. Unit-normal squared-length tolerance is0.002. Compute cross products
in double from the supplied float positions: cross squared length must be at least
1e-16 and its normalized dot with each supplied outward normal at least0.995.
Reject bad geometry before staging rather than fabricating a tiny-normal fallback.

Prototype viewport changes validate aspect[1/32,32], pixelsWide[1,4096] and apply
at the coordinator boundary, persisting through updateCamera before LOD work.
Interactive intro catches up at most4 fixed1/60 presented-second ticks per pass,
retains sub-tick debt up to250ms, and counts discarded decorative presentation time.
Focus suspension and replay reset debt; mission Runtime policy is unchanged.
Receiving-only cold seek is explicit, limited to45 presented seconds/2700 ticks,
and reports actual integral ticks and actual choreography time.

Skip never fast-forwards synchronously. Reduced motion/menu settling advances at
most4 hidden prototype ticks per coordinator pass behind an already-usable menu,
with no animated swarm/camera displayed; show a static floor until the complete
settled mesh is ready. Report preparing/ready state. A cold receiving seek may
prepare the settled view explicitly; do not relabel that as immediate interactive
startup or promise unmeasured latency. Bound default settling at4096 ticks and
report failure if the prototype has not finished by then.

Detailed candidates require projected diameter at least14 pixels and visible
centers within a two-body margin. Select at most128 by normalized projected
distance to camera center, breaking ties by stable identity; all others use the
same low-detail core/window geometry. Reuse selection scratch. This is visual
detail selection, not a change to the prototype's decision-LOD authority.

PBR intensity preserves reference act hierarchy via BT.709 luminance ratios:
shell dormant/reserve uses SILVER_DIM/SILVER; climbing/raised moving units use
the reference +30-byte highlight ratio; layered locked/docking shells multiply by
`1-0.22*(deepest-layer)/max(1,deepest)`. Cores use CYAN_DIM/CYAN while dormant,
reserve switches after strictly more than one choreography second, locked/moored
use CORE_SETTLED/CYAN, and other acts use1. Values stay in[0,4]. Do not claim
RGB or reference software-lighting parity from this material reinterpretation.

Reference corpus covers free rest/full-speed, morphed nose2 with heading ignored,
hinges0/pi4/pi2, both detail counts, all roles, core containment/rigidity, winding,
normal tolerances, act boundary cases and camera mapping. Check in exact-source
oracle values and generator/source hashes; C++ receiving never skips for NumPy.

Apple/Linux/DX12, audio, final rig/contact mechanics, crystal animation, participant
feedback and large-world performance remain separate gates. This slice balances
the demonstrated authored swarm with measured renderer bounds; revise both based
on actual received images rather than declaring the concept matched by source.

## Received graphics semantics and fidelity limits

The mesh uses UV0 as numeric role/intensity metadata. The pinned material compiler
has `flipUV=true` by default (`MaterialBuilder.h`, `surface_material_inputs.vs`),
which changes its y component to `1-y`. Explicit `flipUV:false` is required here:
otherwise settled shield intensity1 becomes0 on the GPU despite green CPU mesh
fixtures. The ImGui texture material separately reverses that ordinary UV flip.
The native receiver keeps the actual material hash and scheduled input-role log.

The default engine evaluates only the strongest directional light. Use one startup
primary key, aimed from the cinematic camera for mesh frames; restore world key
color/direction and shadows for pose frames. The studio is owned procedural radiance
convolved by exact cmgen into six32x32 cube faces/six GGX levels plus nine pre-scaled
irradiance SH coefficients. The immutable32868-byte closure is bounded and validated;
it is not a captured HDR environment or a generic KTX reader. Sub-unit shadow bias
and ambient occlusion radius remain unqualified and disabled for this mesh receiver.
Lit metal, depth-tested core windows and bloom are received separately from final
rig/crystal fidelity, contact shadows, depth of field, audio or participant review.
