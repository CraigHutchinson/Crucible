#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <limits>
#include <span>
#include <type_traits>
#include <vector>
#include <crucible/contracts/GridConfig.hpp>
namespace crucible::presentation { class ScenarioSnapshot; }
namespace crucible::spike {
/// Candidate instance: world center/half extents followed by normalized sRGB RGBA; not a validated shader ABI.
struct alignas(16) WorldInstance {
    std::array<float, 4> geometry{};
    std::array<float, 4> color{};
};
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
static_assert(sizeof(WorldInstance) == 32 && offsetof(WorldInstance, color) == 16);
static_assert(std::is_trivially_copyable_v<WorldInstance> && std::is_standard_layout_v<WorldInstance>);
/** Experiment-only owned world pass, with all storage allocated at startup.
 * One coordinator packs; observers borrow until next successful pack/destruction.
 * Cells and markers have different sizing policies and remain separate draw ranges.
 */
class WorldPacket {
public:
    /// Allocates fixed capacities; unrepresentable capacities/allocation errors throw.
    WorldPacket(std::size_t samples, std::size_t cells);
    WorldPacket(const WorldPacket&) = delete;
    WorldPacket& operator=(const WorldPacket&) = delete;
    WorldPacket(WorldPacket&&) = delete;
    WorldPacket& operator=(WorldPacket&&) = delete;
    /** Copies a completed owned frame without retaining its storage.
     * @param[in] frame Captured rectangular scenario; fields/HUD intentionally excluded.
     * @return False before mutation on missing/capacity/geometry failure, including fp32 half-size underflow.
     */
    [[nodiscard]] bool TryPack(const presentation::ScenarioSnapshot& frame) noexcept;
    [[nodiscard]] std::span<const WorldInstance> GetCells() const noexcept { return {cells_.data(), cell_count_}; }
    [[nodiscard]] std::span<const WorldInstance> GetMarkers() const noexcept { return {markers_.data(), sample_count_}; }
    [[nodiscard]] std::optional<GridConfig> GetGrid() const noexcept { return grid_; }
    [[nodiscard]] std::uint64_t GetTick() const noexcept { return tick_; }
private:
    std::vector<WorldInstance> cells_, markers_;
    std::size_t cell_count_{}, sample_count_{};
    std::optional<GridConfig> grid_;
    std::uint64_t tick_{};
};
}
