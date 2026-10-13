#pragma once

#include <array>
#include <cstddef>
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
    Frame,      ///< Rails, hubs and plain face bars around open windows onto the glass housing and heart crystal.
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

/// Radius of the sphere, about the compact unit's centre, used to measure projected size.
inline constexpr float naniteBoundingRadius = 0.62F;
/// Fraction below a rung's start size that a unit must shrink before it steps down again.
inline constexpr float naniteLodHysteresis = 0.15F;

/// One rung of the distance ladder.
struct NaniteLodStep
{
    NaniteDetail detail_;
    float minPixels_{};  ///< Projected bounding-sphere diameter, in pixels, at which this rung starts.
    bool crossFade_{};   ///< Entering this rung changes the image visibly (measured): blend the switch.
};

/** The distance ladder, lowest rung first. Every rung adds detail on at least one axis and
 * never removes any, so a cap cuts the ladder at a prefix. Switch sizes come from the lab's
 * pixel comparison of neighbouring rungs (docs/workstreams/presentation/phase15-nanite-model.md).
 * @return Static storage, valid for the program's lifetime.
 */
[[nodiscard]] std::span<const NaniteLodStep> naniteLodLadder() noexcept;

/** Chooses a unit's rung from its projected size, with hysteresis against flicker.
 * @param pixels Projected diameter of the bounding sphere (naniteBoundingRadius), in pixels.
 * @param cap Most detail allowed, per axis (a hardware class's hero or swarm cap).
 * @param current Rung shown last frame, or any value past the ladder for a new unit.
 * @return Rung index: steps up as soon as the size reaches a rung, steps down only once the
 *         size falls naniteLodHysteresis below the current rung's start.
 */
[[nodiscard]] std::size_t selectNaniteLod(float pixels, NaniteDetail cap, std::size_t current) noexcept;

/// Proposed detail caps per hardware class: the most detail any unit on that class may use.
struct NaniteDevicePreset
{
    std::string_view name_;
    NaniteDetail hero_;  ///< Cap for a unit filling a large part of the view.
    NaniteDetail swarm_; ///< Cap for the bulk of a dense swarm.
};

/** Proposed caps, lowest class first, each on the distance ladder; tuned against measured
 * triangle counts, not device profiling.
 * @return Static storage, valid for the program's lifetime.
 */
[[nodiscard]] std::span<const NaniteDevicePreset> naniteDevicePresets() noexcept;
}
