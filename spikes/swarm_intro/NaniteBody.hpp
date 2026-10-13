#pragma once

namespace crucible::spike::swarm
{
/** The nanite's 3D body, the single source for the simulation's stacking heights and the
 *  renderer's mesh (director brief: "an engineered low asymmetrical triangular body, with
 *  distinct beveled planes, a recessed protected cyan core and dark mechanical seams";
 *  letter bodies and foreground units share the same construction).
 *
 *  Footprint: a triangle whose inscribed circle is one diameter, so a side is sqrt(3)
 *  diameters, the title's tile. A locked unit's footprint is its morphed tile. Side walls
 *  rise vertically from the footprint, so any tiling of footprints stays flush and gap-free
 *  in 3D; a chamfer then runs from the walls' shoulder to an inset top face. The body is a
 *  wedge: the top slopes from the rear edge down to the nose (the corner it points with),
 *  and the nose chamfer is wider than the rear ones, so the silhouette reads its heading.
 *  The core sits in a recess on the top face, toward the rear, where the body is thickest.
 *  All lengths are in nanite diameters. */
struct NaniteBody
{
    float height = 0.55F;       ///< Rear height: one stacking layer (climbers flowing over the rest layer, depth layers).
    float noseHeight = 0.30F;   ///< Height at the nose: the wedge's ramp.
    float shoulder = 0.55F;     ///< Fraction of the local height where the vertical wall meets the chamfer.
    float bevel = 0.16F;        ///< Rear corners' chamfer: top corner inset toward the centroid, fraction of its distance.
    float noseBevel = 0.34F;    ///< Nose chamfer, wider: the asymmetry that points the body.
    float coreRadius = 0.15F;   ///< Recessed core radius.
    float coreOffset = 0.12F;   ///< Core shift from the top face's centroid toward the rear edge.
};
} // namespace crucible::spike::swarm
