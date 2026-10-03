# Concrete desktop drawing

Phase 4 adds a consumed SDL3 painter for the owned ScenarioSnapshot. Native callbacks
and a software pixel fixture call the same ScenePainter; the host owns SDL setup,
logical presentation, renderer/window/surface lifetime and present. Calls require
exclusive access on the renderer's main thread. No world, camera, renderer or message
borrow survives TryDraw. Painter destruction releases only its CPU scratch.

ScenePainter(sample_capacity,cell_capacity) allocates four vertices and six indices
per maximum of the two capacities, reusing that storage for cells then samples.
Checked startup counts fit SDL int counts, vector lengths and combined byte arithmetic.
Capacity and frame/camera geometry checks precede any drawing. SDL operation failure
can leave a partial canvas; the host discards that frame. SDL owns its command and
prototype glyph storage and may allocate internally; no allocation-free SDL or scale
performance claim is made.

The canvas is 1280x720 logical units. World viewport is {24,96,1232,520}, matching the
pure camera and input coordinator. World cells and fixed visual nanite markers use
one untextured indexed geometry submission each. Double projections are clipped to
the viewport before float narrowing, including after zoom. Each committed field and
one optional preview uses a bounded 64-segment ring geometry submission. Out-of-world
finite field centers remain supported, and zero-radius legacy fields have no visible
ring. Rings are a visual approximation; field sampling still uses the original exact
world parameters. Selected committed rings are thicker. Preview is white and dashed,
distinct from committed green attraction and amber repulsion, and never changes state.

| Visible state | Color |
|---|---|
| Background / fit letterbox | #0b1320 |
| Clear substrate with stock (or legacy clear) | #162637 |
| Infected substrate with stock (or legacy infected) | #9f454d |
| Infected substrate with no stock | #6c562f |
| Clear substrate with no stock | #213b37 |
| Nanite marker | #77ddff |

The HUD shows completed tick, population, paused/blocked state, selected slot and the
initial = stock + mobile + reserve conservation buckets. It labels the cell palette,
committed/preview distinction and controls. Seven buttons occupy y632..664 and
x24/172/320/468/616/764/912 with width136: attract, repel, erase, slot, pause/resume,
restart, fit. Their hit testing belongs to the coordinator. ASCII debug typography
is intentional prototype scope, not the final accessible/localized UI. Message text
is copied into bounded call-local storage; unsupported characters become question
marks, and length is limited to the logical canvas.

The optional scene_painter_software test reads independent software surface pixels,
using analytically known fitted world positions rather than painter helper functions.
It covers stock/infection distinctions, nanite overlay, retained capture after live
mutation, capacity/uncaptured/camera/preview rejection without canvas changes, fit and
zoom clipping, committed versus preview rings, finite external centers and tiny-world
geometry. It does not assert cross-GPU image identity. An optional executable argument
exports a BMP of a 2048-sample, sixty-tick owned frame after the tests, outside their
verified loop. Root integration records executed build/test evidence.
