# Crucible visual concepts

Three generated concept renders accompany the [game design](../game-design.md).
They explore the proposed swarm/Blight/fusion identity and a readable RTS screen.
They are not executable screenshots, final assets, a renderer selection or measured
visual quality/performance. See [exact prompts and provenance](prompts.md).

## World and frontier

![Flowing swarm meets cellular Blight on an industrial substrate](world-v1.png)

Silver/cyan ribbons communicate mobile mass; the red/magenta cellular frontier shows
infection; amber lattices show anchored mass. Vector arcs and circular overlays
explain player intent. The high-oblique camera and matte tiled surface establish a
tactical-scale world without requiring a 3D simulation.

This image emphasizes material atmosphere. Production display should simplify
detail and bloom when zoomed out. Raised machinery and choke points are exploratory:
the first reference arena is flat and obstacle-free until traversal is specified.

## Nanites, fused lattice and Blight

![Unit and material sheet for nanites, fused structures and infected tiles](units-v1.png)

The single nanite is a compact silver wedge with a cyan core. A collective ribbon
uses the same motif. The lattice visibly interlocks related pieces and breaks back
into the swarm, making fusion/shatter understandable. Blight uses contiguous crust,
cracked tiles and a spreading edge rather than another robot faction.

The hovering-looking wedge silhouette is a visual study, not a proposed airborne
unit class. The central lattice study is lower and more modular than the tall world
render: favor that readable functional silhouette for the first structure. The
gallery is a palette/form exploration rather than a finished asset specification.

## Proposed RTS screen

![Concept gameplay screen with fields and tool strip](screen-v1.png)

The battlefield occupies most of the display. The top strip shows biomass,
reclamation and pause; the objective card explains the mission; the bottom strip
exposes Flow, Attract, Repel, Fuse and Shatter. A minimap shows the same frontier.
Attract and Repel differ by both shape and color. Add mobile mass, a selected-tool
inspector and precise confirmation/rejection feedback in the interactive prototype.

Text labels were visually checked and the images inspected for the intended world,
units, palette and UI composition. Numbers such as 680 and 42% are illustrative and
do not define a scenario. The chasms and tall structures are visual exploration;
production geometry and movement rules must agree before such terrain becomes playable.

## Actual implemented state

[Phase 2 SVG and rendered preview](phase2-state.md) show a real owned tick-20
simulation frame, with reproduction and visual validation. Keep it distinct from
the generated concepts above: resource/fusion/mission behavior is still pending.

[Phase 4 live SDL frame](exports/phase4-live-frame.png) is an actual software-rendered
owned tick-60 frame, with 2048 samples, applied attract/repel fields and a dashed
uncommitted preview. Reproduce it with `crucible_scene_painter_test output.bmp`
after building a desktop preset. Its ledger is 10240 = 6439 stock + 2048 mobile +
1753 reserve. The field controls change sample positions; no relay/fusion objective
or performance claim is implied by this image.

[Phase 5 mission frame](exports/phase5-mission-frame.png) is actual software output
from the production InspectorSession and ScenePainter at completed tick 60, with
2,048 samples and two admitted/applied fields. It shows ACTIVE, 1,753/1,780 reclaimed
and 840 ticks left; ledger 10,240 = 6,439 stock + 2,048 mobile + 1,753 reserve.
Reproduce via `crucible_desktop_event_test --export-mission output.bmp`, then convert
BMP to PNG without changing the content. This pair of fields is a displayed fixture,
distinct from the CLI's winning swept-attractor route. Typography and human mission
tuning remain acceptance gates; neither this image nor software CI proves GPU scale.
