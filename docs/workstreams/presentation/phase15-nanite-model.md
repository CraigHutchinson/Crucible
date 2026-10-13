# Procedural nanite model: two detail axes (M1)

2026-10-13. A procedural, articulable nanite for the Filament production renderer, with
**structure** (which parts exist) and **surface** (how finely each part is shaped) as
independent detail axes, so each hardware class can pick a point on both. The form follows
the [nanite design space](../../concepts/nanite/README.md): a tetrahedral silver shell
around a rigid containment housing with one heart crystal inside.

![Detail matrix: rows are structure levels, columns surface levels, triangle count under each](phase15-nanite-model/matrix.png)

*Detail matrix from the [lab](../../../spikes/nanite_lab/README.md): rows Silhouette, Frame,
Shielded, Articulated; columns Flat, Chamfer, Bevel, Hero; same posture (open 0.35) in every
cell. Headless SwiftShader render with three.js materials, not the Filament renderer.*

## What was built

| Piece | Where | Consumer |
| --- | --- | --- |
| Generator `buildNanite(NaniteDetail, NanitePosture)` and device presets | `include/crucible/presentation/production/nanite_model.hpp`, `src/presentation/production/nanite_model.cpp` | Production renderer; web lab |
| Production mesh: replaces the PR48 wedge for every cinematic instance | `production_renderer.cpp` `makeNanite()`, `materials/nanite.mat` (roles in uv0.x) | `crucible_game` |
| Contract test (closure, winding, axes, rigidity, clearance, determinism) | `tests/presentation/production/test_nanite_model.cpp` (`production_nanite_model`) | CI on every platform |
| WebAssembly lab and headless capture | `spikes/nanite_lab/` | Review on any device; this evidence |

### The two axes

| Structure | Parts | | Surface | Shaping |
| --- | --- | --- | --- | --- |
| Silhouette | One closed tetrahedron | | Flat | Sharp planar parts |
| Frame | 6 silver edge rails, 4 graphite corner hubs, one solid luminous core | | Chamfer | One flat chamfer on every part edge |
| Shielded | + 12 recessed shield plates (3 per face around a window), glass housing, separate heart crystal | | Bevel | Rounded edges and corners, smooth normals (3 steps) |
| Articulated | Plates split and hinged; rails split with graphite joints; cradle struts; housing edges | | Hero | 6-step rounding, more crystal facets |

Every part is a convex polyhedron whose vertices join three faces (tetrahedra, prisms). One
routine rounds any of them by a radius without changing its outer extent: faces are offset
inward, edges become cylindrical strips and corners spherical patches. The surface axis only
chooses the step count, so it never changes which parts exist, and the structure axis never
changes how parts are shaped. Every level stays inside the silhouette tetrahedron, so a
renderer stepping between levels keeps its outline.

Triangles per unit (vertices after welding), from the contract test:

| Structure \ Surface | Flat | Chamfer | Bevel | Hero |
| --- | ---: | ---: | ---: | ---: |
| Silhouette | 4 (12) | 20 (48) | 76 (40) | 220 (112) |
| Frame | 92 (204) | 364 (826) | 1,260 (690) | 3,564 (1,840) |
| Shielded | 246 (474) | 902 (2,008) | 3,180 (1,696) | 8,948 (4,600) |
| Articulated | 510 (1,050) | 1,870 (4,132) | 5,972 (3,140) | 15,196 (7,772) |

The lowest point on both axes is exactly a (stretched) tetrahedron; the highest is a
concept-grade hero unit. Every level fits 16-bit indices.

### Posture

`stretch` elongates the shell toward the nose by moving only the nose corner, so every shell
part follows the stretched tetrahedron while the core, housing and heart stay rigid (tested
byte-identical). `open` lifts the shields (Shielded) or lifts and hinges the plates
(Articulated); the base face carries the unit and never opens. `heartTurn` rotates the
crystal inside the housing, which stays fixed; the crystal stays within 80% of the housing's
inscribed sphere for any turn.

![Articulated, Hero surface, open 0.6: the heart crystal visible through a face window](phase15-nanite-model/unit-hero.png)

| Closed (Articulated, Bevel) | Underside | Stretched (Shielded, Bevel) |
| --- | --- | --- |
| ![Closed articulated unit](phase15-nanite-model/unit-closed-bevel.png) | ![Underside showing the base face shields, hubs and the fourth housing vertex](phase15-nanite-model/underside.png) | ![Stretched shell, rigid core](phase15-nanite-model/unit-stretched.png) |

[Turntable video](phase15-nanite-model/turntable.mp4) (8 s, 30 fps): Articulated/Hero through
the travel (stretch out and back) and contact (shields open and settle) cycle.

### Hardware classes

Proposed caps, `naniteDevicePresets()`; the most detail a unit may use on that class. They
are set from the triangle counts above, not from device profiling:

| Class | Hero cap | Hero tris | Swarm cap | Swarm tris | 4096-unit swarm |
| --- | --- | ---: | --- | ---: | ---: |
| Mobile (Android / WebGL) | Shielded, Chamfer | 902 | Silhouette, Flat | 4 | 16K |
| Handheld console (Switch class) | Shielded, Bevel | 3,180 | Frame, Flat | 92 | 0.38M |
| High-end mobile (iOS A15+ class) | Articulated, Bevel | 5,972 | Frame, Flat | 92 | 0.38M |
| Desktop (PC / console) | Articulated, Bevel | 5,972 | Frame, Chamfer | 364 | 1.49M |
| Desktop 4K hero | Articulated, Hero | 15,196 | Shielded, Chamfer | 902 | 3.69M |

![A small swarm at the desktop class: three hero units in front, the rest at the swarm cap](phase15-nanite-model/swarm.png)

## Platform for iterating: web first for form, Filament for acceptance

Recommendation: keep **one C++ generator** and iterate its form and detail levels in the
**WebAssembly lab**, and accept materials and performance in **Filament native**.

- **Turnaround.** The same `nanite_model.cpp` compiles to a 59 KB WebAssembly module in about
  a second and rebuilds a hero unit in 4 ms natively (3–15 ms in the browser), so sliders on
  both axes and the posture are live. A Filament change needs the qualified Windows toolchain
  and a GPU: this session's container and the hosted CI have neither, so every native visual
  check is a round trip through the RTX5070 host.
- **Device reach.** One link runs the lab on real phones, tablets and PCs, so the mobile and
  handheld presets can be judged on representative GPUs before any native port exists.
- **Parity limit.** The lab uses three.js materials, an approximation. Filament's own web
  build would give material parity, but the npm `filament` package is 1.53.4 while
  Crucible pins 1.77.3 (`d852e34`); parity needs a from-source Filament web build and `matc`
  at the pin. That is worth doing when materials (glass housing, heart emission, bloom)
  become the bottleneck, which is the next step below.
- **Acceptance stays native.** The production renderer consumes the generator now, so the
  next native capture shows the real mesh in the real Vulkan path.

## Production consumer

`makeNanite()` builds the desktop class's hero level (Articulated, Bevel: 5,972 triangles)
once at startup and every cinematic instance draws it, replacing the PR48 wedge. The
material reads the generator's role from uv0.x: silver, graphite, and the heart, whose
emission keeps the per-instance `cores` intensity and `glow`. Housing panes are left out of
the production mesh: the lit opaque material would hide the heart, and transparency is not
yet received natively. With 1,251 wordmark instances that is about 7.5M triangles before
shadows. Not yet done natively: per-instance detail selection (all instances share one
level), the glass housing, and a capture. Hosted CI compiles the renderer and the material
(`production` job); this session could not run it.

## Verification

- `production_nanite_model` (all 16 levels x 4 postures): finite unit normals; every part
  closed and wound outward (position-welded directed edges pair up; positive volume); no
  normal opposes its triangle; nothing below the ground; Silhouette/Flat is 4 triangles;
  each axis adds triangles whatever the other holds; materials by level; core, housing and
  heart rigid under stretch and open; heart clearance for any turn; housing inside the shell;
  deterministic bytes; nonfinite postures build the rest pose; presets ordered.
- Release preset: 69/69 CTest cases pass; `swarm_intro_layers2_climb` failed once while the
  lab video rendered in parallel and passed when rerun alone (it does not link the model).
  ASan/UBSan: see the PR.
- Lab stills and video: `capture.mjs`, headless Chromium with SwiftShader.

## Next

1. Native capture of the new mesh (`receive_native.py`) on the RTX host; compare with the
   reactor study under neutral light, bloom off first.
2. Per-instance detail in the renderer: one batch per (structure, surface) level, chosen by
   projected size and capped by the hardware class.
3. Glass housing natively (transparent pass, or opaque smoked windows with a contained
   emissive heart as the design space allows).
4. Filament web build at the pin for material parity in the lab, if three.js shading
   misleads review.
5. Rig: one contact mechanism (edge finger reach) and per-part transforms as bones, so
   posture runs on the GPU instead of rebuilding the mesh.

## Evidence files

All in [phase15-nanite-model/](phase15-nanite-model/): `matrix.png`, `unit-hero.png`,
`unit-closed-bevel.png`, `underside.png`, `unit-stretched.png`, `swarm.png`, `lab-panel.png`
(the lab with its controls), `turntable.mp4`, and `triangles.json` (the triangle table and
presets as the lab read them from the WebAssembly build). Rendered by
`spikes/nanite_lab/capture.mjs` from this commit's generator with Emscripten 6.0.12, three.js
0.170.0 and headless Chromium (SwiftShader).
