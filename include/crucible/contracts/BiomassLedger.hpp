#pragma once

#include <cstdint>

namespace crucible {
/// Conserved biomass buckets and cumulative work diagnostics at a completed tick.
struct BiomassLedger {
    std::uint64_t initial_total{}; ///< Startup source, equal to remaining_stock + mobile_mass + reserve.
    std::uint64_t remaining_stock{}; ///< Sum of all finite substrate stock, independent of infection.
    std::uint64_t mobile_mass{}; ///< Constant mass of the fixed population in this increment.
    std::uint64_t reserve{}; ///< Initial reserve plus material actually harvested.
    std::uint64_t harvested{}; ///< Cumulative harvested quanta; diagnostic, not another conserved bucket.
    std::uint64_t work_actions{}; ///< Cumulative successful actions, including zero-credit clearing.

    /// Compares all conserved quantities and diagnostics for exact replay.
    [[nodiscard]] constexpr bool operator==(const BiomassLedger&) const noexcept = default;
};
}
