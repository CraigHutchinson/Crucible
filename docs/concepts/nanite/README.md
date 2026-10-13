# Nanite design space

2026-10-13 · Detailed single-unit study and future presentation groundwork.

Start here for nanite form, materials, articulation and close-up identity. These
are generated design references and modelling requirements, not implemented
meshes or evidence of a working rig. The swarm prototype renders a low-poly
[tetrahedral proxy](../phase15-experience/nanite-swarm-design.md#the-nanites-body-in-the-prototype-s0j-s0k)
of this direction (S0k), replacing PR43's dart; it is not the approved rig.

![Reactor and faction study: open-shell nanite with tetrahedral housing and inner crystal; cyan, amber, violet and lime instances](assets/reactor-factions-01.png)

*Current direction: [reactor and faction study](reactor-and-factions.md). Shell and form:
[core-corrected form study 02](assets/form-study-02-core-corrected.png). All four studies,
with exact prompts and hashes: [provenance](provenance.md).*

## Current direction

A compact articulated silver machine surrounds a **stable tetrahedral containment
housing**. One luminous crystal lives inside that housing. The shell opens and
makes contact; the internal crystal may rotate or reshape during close-up actions.
The housing retains its four triangular faces. Underside construction continues
the same volumetric language rather than becoming a broad flat slab.

| Design question | Authoritative study in this space |
| --- | --- |
| What was missing from PR43, and how should one unit be modelled? | [Single-unit form and articulation](single-unit-study.md) |
| What is inside the tetrahedron? How do faction and action affect it? | [Reactor and multi-faction groundwork](reactor-and-factions.md) — latest core revision |
| Which image is current, and how was it generated? | [Asset provenance and inspection](provenance.md), with exact prompts |
| What should the next modelling session build and verify? | [Single-unit acceptance and intro handoff](single-unit-study.md#bounded-next-modelling-pass), plus reactor gates below |

## Design precedence and status

The corrected form study establishes the shell direction. The subsequent reactor
study supersedes its solid opaque core material with a containment housing and
separate inner crystal. Preserve both: they answer different design questions.
The original [unit sheet](../units-v1.png) and [intro hero](../phase15-experience/assets/intro-hero-v1.png)
remain historical references. The earlier low asymmetric dart language is not
the final specification for this new single-unit exploration.

The [swarm behaviour prototype](../phase15-experience/nanite-swarm-design.md)
owns assembly/choreography; this space owns detailed visual studies. Link new
revisions here, retain prompts and inspection notes, and explicitly mark any
superseded direction. Do not copy competing body specifications into both places.

Status, M1 (2026-10-13): a [procedural model](../../workstreams/presentation/phase15-nanite-model.md)
builds this direction at four structure and four surface levels (shell, rails, hubs, shields,
housing, heart; cradle struts and jointed rails at the top level), articulates stretch, open
and heart turn around a rigid core, and replaces PR48's wedge in the production renderer. It
is a blockout for review, not the approved rig: no contact mechanism or docking yet.

Next increment: core/housing/shell blockout, underside turntable, one contact
mechanism and close-up reactor animation. Two-unit docking and a small title
patch follow single-unit review. Full intro replacement is a separate receiving
step; no gap-free packing, physical climbing or runtime performance is established
by these images.
