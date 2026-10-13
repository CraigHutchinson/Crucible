# Crucible — single nanite design brief
13 October 2026 · Proposed art direction for review · No runtime change

> **Current revision:** the [reactor and faction groundwork](reactor-and-factions.md) refines the solid cyan tetrahedron below into a stable tetrahedral containment housing with a separate living crystal inside. Keep the shell/articulation findings; use that newer note for core materials and motion.

## Recommended direction

Artwork: [download the four-study bundle](https://chatgpt.com/api/library/files/libfile_2ff8ecf595c48191a40b1f8f52fd5a74/download).
Access requires the owning ChatGPT account. Git LFS import is pending; see
[publication status](provenance.md#publication-validation).

A nanite is a compact articulated machine built around one rigid cyan tetrahedral core. Silver faceted shell pieces surround the core on all sides. Dark joints separate the pieces and let short edge mechanisms unfold to make contact, then retract into a coherent protective silhouette. It is a complete three-dimensional object: turning it over reveals more of the same construction, not a flat chassis plate.

The corrected compact/open study is the recommended visual target. Its open pose explains the core; its closed pose establishes the unit's identity. This is a proposal for Craig's review, not an approved replacement asset or a mechanically validated rig.

## Review of PR #43

Reviewed PR https://github.com/CraigHutchinson/Crucible/pull/43 at head dc33dfe867f1c4043c43fc2de038945d4046007e. Read NaniteBody.hpp, nanite_mesh.py, the director brief, concept README and swarm design; visually inspected units-v1.png, intro-hero-v1.png and body-macro.png. This was a form/design review, not a full code or CI review.

| Finding | Consequence for the next model |
| --- | --- |
| Original unit art wraps silver structures around cyan openings visible from several sides. The hero title repeats this open, layered material language. | Preserve the silver/cyan identity and spatial gaps, not merely the triangular outline. |
| PR43 builds a dart with a raised spine, vertical perimeter walls and five roof facets. Its base vertices are coplanar. | The unit reads as a roof on a tile, especially in the macro evidence. More lighting detail alone will not correct its massing. |
| The cyan diamond is constructed on two roof facets; it is not a separate volumetric core. | Model a separate tetrahedron, with four triangular faces and a visible shared edge between adjacent faces. |
| FACES includes walls and roof but no underside closure. | A turned-over inspection must become an explicit acceptance view; underside shell geometry needs real thickness. |
| Free and locked units share a footprint-driven construction designed for coincident tile walls. | That is useful prototype assembly logic, but it constrains the hero silhouette. The new free body must not be designed solely as an extrusion of a title cell. |
| Stacking and depth-layer spacing consume body height. | Changing visible volume has contact and clearance implications. A model replacement must not silently inherit 0.55-diameter stacking as if already validated. |

The earlier director brief explicitly requested a low asymmetric wedge. The tetrahedral direction is a deliberate revision prompted by this session, not a claim that the earlier implementation ignored a settled tetrahedral specification.

## Geometry and parts

**Core.** Exactly one tetrahedron: four vertices, six edges, four planar triangular faces. It remains rigid and constant in volume through every pose. Start with a regular tetrahedron; visual asymmetry should come from shell pose before deforming the core. Use flat shaded cyan faces, with controlled emissive intensity. Avoid a double pyramid, octahedron, faceted gemstone, sphere or separate glowing inserts in every opening.

**Cradle.** A small graphite internal structure holds the core. It should look mechanically retained rather than levitating inside disconnected armour. It must leave useful sight lines to the core from several directions.

**Outer shell.** A proposed blockout inventory is four vertex junctions, six edge assemblies and four face-shield regions. These counts define the modelling exploration; the generated art does not prove exact count consistency. Start with broad silver forms, strong bevels and narrow dark separations. Face shields can be subdivided only where a visible motion requires it.

**Edge articulation.** Short split rails or fingers nest into the six shell edges. A root hinge and a distal contact segment are a useful initial test. Adjacent face plates can slide or hinge to provide clearance. Do not independently animate both ends of a closed edge linkage without a defined joint/slider: test one edge mechanism before replicating it around the shell.

**Underside.** Continue the same faceted cage, protected core and recess vocabulary beneath the unit. Small contact pads may be planar; the objection is a broad continuous slab base. A three-point resting pose must have real support and keep the core clear of the surface.

**Proportions.** Prioritise a balanced volumetric form over the current long dart. As a nonbinding blockout starting point, try a core edge around half the compact shell span. Compare at equal projected size before choosing dimensions. No real-world scale or simulation diameter change is prescribed here.

## Motion vocabulary

| Pose/action | Readable behaviour |
| --- | --- |
| Rest | Shell compact, stable contacts, cyan core recessed. |
| Wake | A restrained core pulse precedes a small opening/settling of the nearest shield. |
| Inspect/reach | One short edge mechanism extends toward a potential contact; the rest remain compact. |
| Contact/transfer | Establish a visible new contact before releasing an old one. Shell movement gives purpose to the change in body pose. |
| Dock | Edge fingers retract or align, shields settle into the available envelope, then contact locks. |
| Release | Reverse the contact sequence, preserving part identity and core volume. |

These are animation proposals. The current swarm glide/climb simulation is not evidence that this rig can physically climb. Do not add flight, legs, thrusters or new gameplay abilities by implication.

## Material and presentation

Silver outer planes, graphite recesses and one cyan core remain the primary hierarchy. Use broad soft white light, a restrained cyan contribution and contact shadows. Retain the existing silver roughness range as a starting point; tune against the actual renderer. Review without bloom before adding bloom. The core should retain readable face values instead of clipping to white.

Present the single unit before returning to the swarm: one three-quarter compact view, a side view, an underside view and an open pose. Use a simple neutral stage and matched scale. A slow turntable and one short compact–contact–compact cycle should expose problems more effectively than another dramatic swarm still. Reduced motion should use the compact still.

At small sizes preserve silhouette and core visibility; omit joints and tiny seams. The existing 6/10/14-pixel detail thresholds are prototype values and need reevaluation with this silhouette.

## Artwork iteration and limitations

1. Form study 01 restored wraparound silver volume and provided underside/open studies. The mechanisms were busy and cyan openings could imply multiple cores.
2. Form study 02 made the compact/open comparison stronger. The exposed core still resembled a multi-faceted diamond and was rejected as a topology reference.
3. Corrected study 02 replaced that gem with a simple tetrahedron. It is the recommended appearance reference. The image's label remains “FORM STUDY 02”; the final generation is the core-corrected revision.

All three were generated with the built-in image tool, with opaque backgrounds and no CLI fallback. The original unit sheet and intro hero were supplied as visual references for study 01; each subsequent call used the immediately preceding study. These are generated proposals, not screenshots of the executable model. Exact prompts are appended below.

Visual inspection confirms the final board's useful core silhouette, compact/open contrast and faceted silver identity. It does not establish a consistent four-hub/six-edge rig across views, collision-free movement, actual three-point support or exact shell topology. The solid cyan material is also deliberately simpler than the earlier luminous crystal interpretation.

## Bounded next modelling pass

1. Build the exact rigid core, internal cradle and a complete underside. Use simple grey shell pieces first.
2. Prove one short edge mechanism with attached shield movement; choose joint limits from clearance testing.
3. Replicate and reconcile that mechanism around the shell. Capture compact, open, side and underside views at matched scale.
4. Animate one contact transfer. Check core/shell and shell/shell intersections throughout the motion.
5. Compare the approved blockout with the final study under neutral lighting, then add silver/cyan materials.
6. Only then test two neighbouring units and a small title patch.

Acceptance: one core in every view; recognisable volume below the midline; no broad slab underside; parts stay attached; a readable stable resting pose; core remains protected; joints do not intersect; silhouette survives bloom-off and small-size review.

## Intro integration handoff

Keep single-unit approval separate from replacing the PR43 body. The current prototype can continue to demonstrate choreography.

A tetrahedral core with articulated armour does not automatically fill the existing arbitrary morphed triangular title cells. Do not assume it can tessellate gap-free. Prototype a compact docking envelope using rigid core placement plus bounded shell adjustment; measure remaining voids and intersections before committing to the existing title thickness. Preserve open letter counters and title readability. Any silhouette proxy used at distance must be disclosed and shown to transition without obvious popping.

Follow-up scope: one single-unit rig, two-unit docking, a small upright title patch, then full introduction receiving. Revisit stacking height, core clearance, footprint morphing, bounds and detail tiers together. Decorative joints stay view-owned; any change to authoritative contact rules or heights needs its own fixtures and review. No gameplay, resource, population or performance claims follow from this art pass.
