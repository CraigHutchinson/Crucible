#pragma once

#include <cstdint>

namespace crucible {
/// Conserved biomass buckets and cumulative work diagnostics at a completed tick.
struct BiomassLedger {
    std::uint64_t initial_total{}; ///< Startup source, equal to stock + mobile + reserve + structure + loss.
    std::uint64_t remaining_stock{}; ///< Sum of all finite substrate stock, independent of infection.
    std::uint64_t mobile_mass{}; ///< Mass of participating mobile samples.
    std::uint64_t reserve{}; ///< Initial reserve plus material actually harvested.
    std::uint64_t harvested{}; ///< Cumulative harvested quanta; diagnostic, not another conserved bucket.
    std::uint64_t work_actions{}; ///< Cumulative successful actions, including zero-credit clearing.

    std::uint64_t structure_mass{}; ///< Mobile biomass anchored in the occupied lattice.
    std::uint64_t lost_mass{}; ///< Cumulative irreversible shatter loss.

    /// Compares all conserved quantities and diagnostics for exact replay.
    [[nodiscard]] constexpr bool operator==(const BiomassLedger&) const noexcept = default;
};
}
