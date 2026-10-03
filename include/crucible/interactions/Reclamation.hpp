#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <crucible/contracts/BiomassLedger.hpp>
#include <crucible/contracts/GridConfig.hpp>
#include <crucible/contracts/ResourceSettings.hpp>
#include <crucible/contracts/SampleState.hpp>

namespace crucible::blight { class Grid; }
namespace crucible::interactions {
/** Reclaims finite substrate into reserve while preserving biomass conservation.
 * Owns startup-sized state and scratch; Simulation coordinates all access sequentially.
 * See docs/decisions/phase3-resource-rules.md for the numerical rule.
 */
class Reclamation {
public:
    /** Initializes uniform substrate and externally supplied mobile/reserve mass.
     * @param[in] config Physical rectangular contact geometry.
     * @param[in] sample_capacity Fixed active population, with IDs exactly 1..capacity.
     * @param[in] settings Integer quanta and successful work budget.
     * @throws std::invalid_argument Invalid geometry or zero mass_per_sample.
     * @throws std::overflow_error Startup products or totals exceed uint64.
     * @throws std::length_error Storage exceeds vector limits.
     * @throws std::bad_alloc Startup storage allocation fails.
     */
    Reclamation(GridConfig config, std::size_t sample_capacity, ResourceSettings settings);
    Reclamation(const Reclamation&) = delete;
    Reclamation& operator=(const Reclamation&) = delete;
    Reclamation(Reclamation&&) = delete;
    Reclamation& operator=(Reclamation&&) = delete;

    /** Atomically publishes proposed spread, bounded clearing/harvest and its ledger.
     * @param[in,out] blight Matching geometry, with no outstanding prepared lease.
     * @param[in] samples Exactly the fixed active IDs, unique; finite post-move positions clamp to world bounds.
     * @return False on invalid inputs/geometry, busy Blight or arithmetic exhaustion, preserving committed state.
     * @note No allocation; no input borrows survive the call. This is not whole-tick ECS rollback.
     */
    [[nodiscard]] bool TryStep(blight::Grid& blight, std::span<const SampleState> samples) noexcept;

    /// Returns all conserved buckets and cumulative diagnostics by value.
    [[nodiscard]] constexpr BiomassLedger GetLedger() const noexcept { return ledger_; }
    /** Borrows row-major committed substrate quanta.
     * @return Stock values; the borrow expires at the next successful step or destruction.
     */
    [[nodiscard]] constexpr std::span<const std::uint64_t> GetStocks() const noexcept { return stocks_; }

private:
    struct Proposal {
        std::size_t cell{};
        SampleId id{};
        auto operator<=>(const Proposal&) const = default;
    };
    [[nodiscard]] std::size_t CellIndex(Position position) const noexcept;

    GridConfig config_{};
    GridExtent extent_{};
    ResourceSettings settings_{};
    BiomassLedger ledger_{};
    std::vector<std::uint64_t> stocks_, pending_stocks_;
    std::vector<Proposal> proposals_;
    std::vector<std::uint8_t> seen_;
};
}
