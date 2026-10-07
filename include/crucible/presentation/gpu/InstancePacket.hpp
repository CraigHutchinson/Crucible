#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>
#include <crucible/contracts/GridConfig.hpp>
namespace crucible::presentation { class Camera2D; class ScenarioSnapshot; }
namespace crucible::presentation::gpu {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
/// Opaque normalized sRGB channels; affiliation is never inferred from color.
using OpaqueColor = std::array<float, 4>;
/// Shader vertex attributes: normalized center/half extents, then opaque sRGB.
/// Marker half extents are zero; their camera-dependent logical size is a uniform.
struct alignas(16) Instance {
    std::array<float, 4> geometry{};
    OpaqueColor color{};
};
static_assert(sizeof(Instance) == 32 && offsetof(Instance, color) == 16);
static_assert(std::is_standard_layout_v<Instance> && std::is_trivially_copyable_v<Instance>);
/// Two std140 float4 camera uniforms; logical origin/extent and canvas/marker size/mode.
struct alignas(16) Projection {
    std::array<float, 4> origin_extent{};
    std::array<float, 4> canvas_marker{1280, 720, 2, 0};
};
static_assert(sizeof(Projection) == 32);

/** Startup-owned instancing input, independent of device or ECS storage.
 * Single coordinator. Observers expire on successful capture or destruction.
 * Normalization occurs in double before narrowing, preserving tiny physical cells.
 */
class InstancePacket {
public:
    /// Allocates both bounded ranges; checked total bytes fit SDL Uint32 buffer sizes.
    InstancePacket(std::size_t sample_capacity, std::size_t cell_capacity);
    InstancePacket(const InstancePacket&) = delete;
    InstancePacket& operator=(const InstancePacket&) = delete;
    InstancePacket(InstancePacket&&) = delete;
    InstancePacket& operator=(InstancePacket&&) = delete;
    /** Packs an owned captured frame; false preserves every previous record/metadata.
     * @param[in] frame Completed snapshot; no borrow survives the call.
     * @param[in] cells Optional exact-length opaque colors, copied during this call.
     * @param[in] markers Optional colors for every captured identity, copied for mobile records only.
     * Anchored/lost identities are omitted from markers without changing their snapshot IDs.
     * Empty color spans use the production palette. Nonfinite/out-of-range RGB or
     * alpha other than one is rejected. Colors describe presentation, not factions.
     */
    [[nodiscard]] bool TryCapture(const ScenarioSnapshot& frame,
        std::span<const OpaqueColor> cells = {}, std::span<const OpaqueColor> markers = {}) noexcept;
    [[nodiscard]] std::span<const Instance> GetCells() const noexcept { return {cells_.data(), cell_count_}; }
    [[nodiscard]] std::span<const Instance> GetMarkers() const noexcept { return {markers_.data(), sample_count_}; }
    [[nodiscard]] std::optional<GridConfig> GetGrid() const noexcept { return grid_; }
    [[nodiscard]] std::uint64_t GetTick() const noexcept { return tick_; }
private:
    std::vector<Instance> cells_, markers_;
    std::size_t cell_count_{}, sample_count_{};
    std::optional<GridConfig> grid_;
    std::uint64_t tick_{};
};
/// Validates matching geometry/fixed viewport and narrows only bounded logical values.
[[nodiscard]] std::optional<Projection> TryMakeProjection(const InstancePacket& packet,
                                                         const Camera2D& camera) noexcept;
}
