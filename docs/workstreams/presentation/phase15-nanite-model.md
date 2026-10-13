# Procedural nanite model: two detail axes (M1–M3)

2026-10-13. A procedural, articulable nanite for the Filament production renderer, with
**structure** (which parts exist) and **surface** (how finely each part is shaped) as
independent detail axes, so each hardware class can pick a point on both. The form follows
the [nanite design space](../../concepts/nanite/README.md): a tetrahedral silver shell
around a rigid containment housing with one heart crystal inside. M2 (same day, after
user review of M1) gives the low levels opaque inner detail and breaks up M1's regular,
"safe" structure with seeded irregularity; see [M2](#m2-inner-detail-without-transparency-and-intricacy).
M3 measures whether switching levels with distance pops, and adds a measured distance
ladder with hysteresis and a cross-fade; see [M3](#m3-distance-consistency).

![Detail matrix: rows are structure levels, columns surface levels, triangle count under each](phase15-nanite-model/matrix.png)

*Detail matrix from the [lab](../../../spikes/nanite_lab/README.md): rows Silhouette, Frame,
Shielded, Articulated; columns Flat, Chamfer, Bevel, Hero; same posture (open 0.35) in every
cell. Headless SwiftShader render with three.js materials, not the Filament renderer.*

## What was built

| Piece | Where | Consumer |
| --- | --- | --- |
| Generator `buildNanite(NaniteDetail, NanitePosture, variant)` and device presets | `include/crucible/presentation/production/nanite_model.hpp`, `src/presentation/production/nanite_model.cpp` | Production renderer; web lab |
| Production mesh: replaces the PR48 wedge for every cinematic instance | `production_renderer.cpp` `makeNanite()`, `materials/nanite.mat` (roles in uv0.x) | `crucible_game` |
| Contract test (closure, winding, axes, rigidity, clearance, determinism) | `tests/presentation/production/test_nanite_model.cpp` (`production_nanite_model`) | CI on every platform |
| WebAssembly lab and headless capture | `spikes/nanite_lab/` | Review on any device; this evidence |

### The two axes

| Structure | Parts | | Surface | Shaping |
| --- | --- | --- | --- | --- |
| Silhouette | One closed tetrahedron, each face windowed down to a luminous facet, with graphite corner caps | | Flat | Sharp planar parts |
| Frame | 6 silver edge rails, 4 graphite corner hubs, 12 plain face bars (the shields' band) around open windows onto the glass housing and heart | | Chamfer | One flat chamfer on every part edge |
| Shielded | + 12 uneven, clipped, tilted shield plates (3 per face around a window), glass housing, separate heart crystal | | Bevel | Rounded edges and corners, smooth normals (3 steps) |
| Articulated | 24 split, hinged plates with 9 stacked armour plates; rails split at an uneven joint with two collars each; cradle struts; housing edges | | Hero | 6-step rounding, more crystal facets |

Every part is a convex polyhedron whose vertices join three faces (tetrahedra, prisms). One
routine rounds any of them by a radius without changing its outer extent: faces are offset
inward, edges become cylindrical strips and corners spherical patches. The surface axis only
chooses the step count, so it never changes which parts exist, and the structure axis never
changes how parts are shaped. Every level stays inside the silhouette tetrahedron, so a
renderer stepping between levels keeps its outline.

Triangles per unit (vertices after welding), from the contract test:

| Structure \ Surface | Flat | Chamfer | Bevel | Hero |
| --- | ---: | ---: | ---: | ---: |
| Silhouette | 76 (156) | 116 (240) | 220 (232) | 436 (376) |
| Frame | 246 (474) | 902 (2,004) | 3,180 (1,700) | 8,948 (4,609) |
| Shielded | 342 (666) | 1,190 (2,582) | 4,140 (2,180) | 11,636 (5,948) |
| Articulated | 1,170 (2,238) | 4,114 (8,894) | 11,552 (6,879) | 27,688 (14,973) |

The lowest point on both axes is a (stretched) tetrahedron of 76 triangles, its windows
and corner caps included; the highest is a concept-grade hero unit. Every level fits 16-bit indices, and
every variant of a level has the same triangle count.

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
| Mobile (Android / WebGL) | Frame, Chamfer | 902 | Silhouette, Flat | 76 | 0.31M |
| Handheld console (Switch class) | Frame, Bevel | 3,180 | Silhouette, Flat | 76 | 0.31M |
| High-end mobile (iOS A15+ class) | Shielded, Bevel | 4,140 | Frame, Flat | 246 | 1.01M |
| Desktop (PC / console) | Articulated, Bevel | 11,552 | Frame, Chamfer | 902 | 3.69M |
| Desktop 4K hero | Articulated, Hero | 27,688 | Frame, Bevel | 3,180 | 13.0M |

Every cap is a rung of the [distance ladder](#m3-distance-consistency), so a unit's detail is
the ladder's choice for its size, cut at the cap. The swarm column is the cap for the bulk of
a dense swarm; with the ladder most of those units are far smaller than its first switch.

![A small swarm at the desktop class: three hero units in front, the rest at the swarm cap](phase15-nanite-model/swarm.png)

## M2: inner detail without transparency, and intricacy

User review of M1: the silhouette and other low levels lost all internal detail, and the
structure read as "very safe", missing the natural, slightly jagged character of the concept
renders.

**Opaque inner detail.** Every face of the Silhouette level now carries a window: a silver
rim, walls sloping into the body (graphite in M2, silver since M3) and, at the bottom, an inverted luminous facet
(`emitWindowedFace`). The heart reads from any side with no transparency. (M3 then matched
the window to the shielded level's and added corner caps: 76 triangles.) The Frame level's solid cyan core became a dark housing whose faces opened onto luminous facets (M3 replaced it with the shielded levels' housing and heart). Both were ordinary opaque triangles, so the
Filament material and any mobile renderer draw them unchanged.

| Silhouette, Flat (76 triangles) | Silhouette, Bevel (220) |
| --- | --- |
| ![Silhouette level: an opaque window on each face down to a glowing facet](phase15-nanite-model/unit-silhouette.png) | ![Bevelled silhouette with windows](phase15-nanite-model/unit-silhouette-bevel.png) |

**Intricacy.** A `variant` seed drives per-feature irregularity, drawn in a fixed order so a
variant always builds the same unit:

- Shield plates reach unevenly toward the window at each end (a jagged window outline),
  vary in rim, thickness (0.032–0.052) and inward tilt (up to 5°), and have their outer
  corners clipped by uneven cuts (faceted, still convex).
- Articulated plates split in two at an uneven point; the longer piece carries a smaller
  stacked armour plate, kept under the face plane.
- Rails vary in width; Articulated rails split at an uneven joint and carry two graphite
  collars.
- The heart crystal's facets vary more strongly and per variant.

Two geometric guards keep the silhouette exact: a plate starts far enough from its edge for
that edge's dihedral angle (on a stretched nose the sharp dihedral otherwise lets a thick
plate pass through the neighbouring face), and rounding is limited to half a part's shortest
edge so clipped corners stay closed. The lab's swarm gives each unit its own variant; the
production renderer uses variant 0 for every instance until per-instance meshes exist.

## M3: distance consistency

User direction: check that detail changes with distance without a visible pop or change.

**Method.** The lab renders a single unit at an exact projected size (the diameter, in
pixels, of a 0.62-radius bounding sphere), 4x4 supersampled, over the stage background with
bloom off, and compares two renders pixel by pixel: mean colour difference over the unit's
pixels and coverage overlap, averaged over four views (`lod_study.mjs`). A dolly then sweeps
a turning unit from 6 to 150 px and back over 10 s and, per frame, measures the **jump** a
switch causes (what is shown, against what the previous frame's choice would show at the
same pose) for a hard cut and for a 0.25 s cross-fade, next to the change the unit's own
motion makes (`lod_dolly.mjs`). Small sizes exaggerate the metric: at 8 px the unit is a
few dozen pixels, so one changed pixel is a few percent.

**What the measurement found and changed.**

- *Frame was thin and see-through* (narrow rails around a core) while the silhouette's rim
  was a wide solid band: different shapes at the same scale (user review). Frame now has
  plain face bars over exactly the band the shields and the silhouette's rim cover, around
  the same window. A second review found its window black: its own dark core showed where
  the other levels show light glass and crystal. Frame now shares the housing and heart, so
  Frame→Shielded is 1.1–2% at 10–32 px and Silhouette→Frame 5.6% at 10 px, and the
  cheaper Frame levels carry the middle of the ladder.

  ![Silhouette, Frame, Shielded (Flat and Bevel) and Articulated at 16 and 96 px: one outline, band and window](phase15-nanite-model/shape-strip.png)
- *The silhouette popped* against the shielded level by 8–15% at 6–16 px. Its window was
  larger and darker, it lacked the dark corner hubs, and it covered about 15% more pixels,
  because the higher levels' sunk rails and rounded hubs inset their visible outline. The
  window now matches the shielded window (0.40 of the face), graphite corner caps sit where
  hubs are (the edge strips split there, so no T-junctions), and the silhouette is drawn at
  0.93 scale toward the shell centre, with its window 0.43 of that inset face so the opening
  is the same size as the higher levels' on the full shell. User review then found the rim
  still wider than the others: an earlier parameter sweep had left the window at 0.26. With
  the window corrected, silver walls instead of graphite (through the higher levels' window
  you see light plates and crystal, not a dark well) keep the pixel difference down:
  Silhouette→Shielded is 5.9% at 10 px (from 8–9% before M3) and Silhouette→Frame 5.6%.
- *Surface steps are small*; Shielded→Articulated adds parts and differs by 6–9% at any size.

**The ladder** (`naniteLodLadder`, `selectNaniteLod`): seven rungs, each adding detail on at
least one axis and never removing any, so a hardware cap cuts it at a prefix. A rung starts
where its neighbour's difference is at most 3%, except the two steps that add parts, which
fade:

| Rung | Level | Starts at | Difference entering (mean) | Fade flag |
| --- | --- | ---: | ---: | --- |
| 0 | Silhouette, Flat | 0 px | | |
| 1 | Frame, Flat | 10 px | 5.6% | yes |
| 2 | Frame, Chamfer | 14 px | 3.0% | |
| 3 | Frame, Bevel | 40 px | 2.8% | |
| 4 | Shielded, Bevel | 48 px | 2.4% | |
| 5 | Articulated, Bevel | 64 px | 7.6% | yes |
| 6 | Articulated, Hero | 160 px | 0.5% | |

The Shielded rung splits Frame→Articulated (8.0% in one step at 64 px) into 2.4% and 7.6%.

A unit steps up as soon as its size reaches a rung and steps down only 15% below the rung's
start, so a size jittering around a boundary does not flicker (tested: at most one switch in
200 frames of ±7% jitter). The lab fades **every** switch over 0.25 s with dithered
(alpha-hashed) materials and lets a switch wait until a running fade finishes; the flag says
which switches a renderer must fade if it can only afford some.

![Every rung at each switch size (10, 14, 40, 48, 64 px), real pixels enlarged](phase15-nanite-model/ladder-strip.png)

*Rows: the seven rungs at 10, 14, 40, 48 and 64 px (the switch sizes), real pixels enlarged.*

![Dolly chart: size against the ladder; per-frame switch jump for hard cut and cross-fade against the unit's own motion](phase15-nanite-model/dolly-chart.png)

| Dolly, 6–150 px, 300 frames | Hard cut | 0.25 s cross-fade |
| --- | ---: | ---: |
| Largest switch jump in one frame | 6.5% | 0.85% |
| 99th percentile jump | 4.8% | 0.83% |
| Unit's own motion per frame (reference) | mean 3.4%, up to 6.7% | |
| Difference from always-highest detail | mean 4.3%, worst 10.5% (at 8 px) | mean 4.2%, worst 10.0% |

With the fade, every switch adds less change per frame than the motion that causes it; the
remaining difference from always-highest detail is the cost of the simplification at small
sizes, which the dolly shows changing smoothly rather than in steps.
[Dolly video](phase15-nanite-model/dolly.mp4): reference | hard cut | cross-fade, real pixels
enlarged 2x.

**Lab.** The Distance view drives one unit through the ladder from its on-screen size
(slider or auto dolly), capped by the selected hardware class, with the cross-fade on or off.

**Not yet native.** The production renderer still draws one level for every instance. Next:
one instance batch per rung chosen by `selectNaniteLod`, and the fade as a dithered discard in
`nanite.mat` with a per-instance fade value.

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

`makeNanite()` builds the desktop class's hero level (Articulated, Bevel: 11,552 triangles)
once at startup and every cinematic instance draws it, replacing the PR48 wedge. The
material reads the generator's role from uv0.x: silver, graphite, and the heart, whose
emission keeps the per-instance `cores` intensity and `glow`. Housing panes are left out of
the production mesh: the lit opaque material would hide the heart, and transparency is not
yet received natively. With 1,251 wordmark instances that is about 14.5M triangles before
shadows, which makes per-instance detail (next, 2) the first native performance step. Not yet done natively: per-instance detail selection (all instances share one
level), the glass housing, and a capture. Hosted CI compiles the renderer and the material
(`production` job); this session could not run it.

## Verification

- `production_nanite_model` (all 16 levels x 4 postures): finite unit normals; every part
  closed and wound outward (position-welded directed edges pair up; positive volume); no
  normal opposes its triangle; nothing below the ground; every level at rest, compact or
  stretched, stays inside the silhouette tetrahedron for three variants (2 mm tolerance);
  variants differ but keep their triangle count; Silhouette/Flat stays a few dozen
  triangles; each axis adds triangles whatever the other holds; materials by level; core,
  housing and heart rigid under stretch and open; heart clearance for any turn; housing
  inside the shell; deterministic bytes; nonfinite postures build the rest pose; presets
  ordered by hero cost.
- The ladder test: monotone rungs at rising sizes and cost, every preset cap on the ladder,
  step-up at a rung's start, hold inside and step down below the hysteresis band, at most
  one switch for a size jittering ±7% around each boundary, caps cut the ladder, nonfinite
  sizes give the silhouette.
- The model test passes in Release, Debug and ASan/UBSan locally; CI runs every platform,
  the sanitizer job and the Windows production build of the renderer and material.
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

All in [phase15-nanite-model/](phase15-nanite-model/), regenerated at M3 (the turntable
video is from M2; the Articulated levels it shows did not change): `matrix.png`,
`unit-hero.png`, `unit-silhouette.png`, `unit-silhouette-bevel.png`,
`unit-closed-bevel.png`, `underside.png`, `unit-stretched.png`, `swarm.png`, `lab-panel.png`
(the lab with its controls), `turntable.mp4`, `triangles.json` (the triangle table and
presets as the lab read them from the WebAssembly build), and for M3 `ladder-strip.png`,
`pop-study.json` (every neighbouring pair on the grid, 6–128 px), `pop-study-silhouette.json` (Silhouette to Frame and to Shielded), `dolly.json`, `dolly-chart.png`,
`dolly.mp4` and `shape-strip.png`. Rendered by
`spikes/nanite_lab/capture.mjs` from this commit's generator with Emscripten 6.0.12, three.js
0.170.0 and headless Chromium (SwiftShader).
