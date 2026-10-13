#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace crucible::presentation::production
{
/** How many distinct parts the unit is built from: the structural axis of its detail.
 * Each level keeps the previous level's silhouette and adds parts inside or onto it,
 * so a renderer can step between adjacent levels without the outline jumping.
 */
enum class NaniteStructure : std::uint8_t
{
    Silhouette, ///< One closed tetrahedral shell, each face windowed down to a luminous facet (opaque).
    Frame,      ///< Silver edge rails and graphite corner hubs around one solid luminous core.
    Shielded,   ///< Adds recessed face shields, a glass containment housing and a separate heart crystal.
    Articulated ///< Splits shields and rails into hinged plates and jointed fingers; adds cradle struts and contact pads.
};

/** How finely each part's surface is shaped: the polygon axis of its detail, independent
 * of which parts exist. Rounding never changes a part's outer extent.
 */
enum class NaniteSurface : std::uint8_t
{
    Flat,    ///< Sharp planar parts: the fewest triangles.
    Chamfer, ///< One flat chamfer on every part edge.
    Bevel,   ///< Rounded edges and corners with smooth normals.
    Hero     ///< Finer rounding and a more faceted heart for close, high-resolution views.
};

/// Rendering role of a triangle; renderers map these to their own materials.
enum class NaniteMaterial : std::uint8_t
{
    Silver,   ///< Outer shell, rails and shield plates.
    Graphite, ///< Hubs, joints, struts and pads.
    Housing,  ///< Containment housing panes; translucent where the renderer supports it.
    Heart     ///< Luminous core or heart crystal.
};

/// One point on the two independent detail axes.
struct NaniteDetail
{
    NaniteStructure structure_{NaniteStructure::Frame};
    NaniteSurface surface_{NaniteSurface::Flat};
};

/// Articulation of one unit. Every value is clamped to its documented range.
struct NanitePosture
{
    float stretch_{};   ///< [0,1] shell elongation toward the nose (+Z); core and heart never change.
    float open_{};      ///< [0,1] shields lift (Shielded), or lift and hinge outward (Articulated); the base face stays down.
    float heartTurn_{}; ///< Heart crystal rotation about +Y in radians; housing stays fixed.
};

/// One welded vertex: unit-space position (shell edge 1, base on y=0, nose +Z) and unit normal.
struct NaniteVertex
{
    std::array<float, 3> position_{};
    std::array<float, 3> normal_{};
    NaniteMaterial material_{NaniteMaterial::Silver};
};

/// Indexed triangle list, counter-clockwise when seen from outside each closed part.
struct NaniteMesh
{
    std::vector<NaniteVertex> vertices_;
    std::vector<std::uint32_t> indices_;
};

/** Builds one nanite procedurally for a detail level and posture.
 * @param detail Structure and surface levels; each axis is independent.
 * @param posture Articulation; out-of-range values are clamped.
 * @param variant Seed for the unit's irregularity (plate reach, cuts, tilt, rail widths,
 *        heart facets); every variant keeps the same parts, silhouette and triangle count.
 * @return Closed parts, each consistently wound outward; identical inputs give identical bytes.
 * @throws std::bad_alloc Mesh storage cannot be allocated.
 */
[[nodiscard]] NaniteMesh buildNanite(NaniteDetail detail, const NanitePosture& posture = {}, std::uint32_t variant = 0);

/// Proposed detail caps per hardware class: the most detail any unit on that class may use.
struct NaniteDevicePreset
{
    std::string_view name_;
    NaniteDetail hero_;  ///< Cap for a unit filling a large part of the view.
    NaniteDetail swarm_; ///< Cap for the bulk of a dense swarm.
};

/** Proposed caps, lowest class first; tuned against measured triangle counts, not device profiling.
 * @return Static storage, valid for the program's lifetime.
 */
[[nodiscard]] std::span<const NaniteDevicePreset> naniteDevicePresets() noexcept;
}
