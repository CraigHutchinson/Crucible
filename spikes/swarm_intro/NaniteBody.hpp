#pragma once

namespace crucible::spike::swarm
{
/** The nanite's 3D body, the single source for the simulation's stacking heights and the
 *  renderer's mesh (nanite_mesh.py). The concepts (docs/concepts/units-v1.png, the phase 15
 *  hero) show a slender arrowhead dart: a raised spine from the rear to the nose, faceted
 *  plates falling away from it, a steep rear facet and a cyan core set into the spine. The
 *  director brief: "an engineered low asymmetrical triangular body, with distinct beveled
 *  planes, a recessed protected cyan core and dark mechanical seams"; letter bodies and
 *  foreground units share the same construction.
 *
 *  The construction works on any triangular footprint with a nose corner: vertical walls to
 *  a low shoulder, a spine from just behind the nose (low) to just inside the rear edge
 *  (high), and five roof facets from the shoulder ring to the spine. A free unit's footprint
 *  is a dart; a locked unit's is its (morphed) title tile, so the walls of neighbouring tiles
 *  coincide and a tessellated letter stays gap-free in 3D. All lengths are in nanite
 *  diameters; fractions are of the named distance. */
struct NaniteBody
{
    float height = 0.55F;       ///< Spine height at the rear: one stacking layer (climbers over the rest layer, depth layers).
    float noseHeight = 0.22F;   ///< Spine height at its front: the wedge's ramp.
    float shoulder = 0.40F;     ///< Wall height as a fraction of the local spine height.
    float spineFront = 0.28F;   ///< Spine front: from the nose toward the centroid.
    float spineRear = 0.14F;    ///< Spine rear: from the rear edge's midpoint toward the centroid.
    float dartLength = 1.90F;   ///< Free unit footprint: nose to rear edge (the concept dart is about twice as long as wide).
    float dartWidth = 0.90F;    ///< Free unit footprint: rear edge width.
    float coreLength = 0.36F;   ///< Core diamond length along the spine.
};
} // namespace crucible::spike::swarm
