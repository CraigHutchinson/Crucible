#pragma once

namespace crucible::spike::swarm
{
/** The nanite's body in the prototype, the single source for the simulation's stacking
 *  height and the renderer's mesh (nanite_mesh.py); frame captures carry these values.
 *
 *  The unit's form is owned by docs/concepts/nanite/: a compact silver tetrahedral shell
 *  around one rigid tetrahedral core. This is a low-poly proxy of that direction, not its
 *  specification: a tetrahedral shell whose base is the unit's footprint and whose apex
 *  stands one stacking layer high, leaned toward the nose. A free unit's footprint is an
 *  equilateral triangle; a locked unit's is its (morphed) title tile, a disclosed docking
 *  envelope (the design space does not claim the articulated unit tessellates). A moving
 *  unit's shell elongates with speed, its nose reaching ahead, while its core stays rigid. All
 *  lengths are in nanite diameters. */
struct NaniteBody
{
    float height = 0.55F;   ///< Shell apex height: one stacking layer (climbers over the rest layer, depth layers).
    float span = 1.10F;     ///< Free unit footprint: side of its equilateral base.
    float lean = 0.16F;     ///< Apex offset from the base centroid toward the nose, as a fraction of that distance.
    float core = 0.40F;     ///< Core tetrahedron edge as a fraction of the shell's mean base edge (clamped to fit).
    float stretch = 0.90F;  ///< A moving shell's extra length at full speed, as a fraction of `span` (the core never changes).
    float stretchSpeed = 24.0F;   ///< Speed (diameters per choreography second) of full stretch.
};
} // namespace crucible::spike::swarm
