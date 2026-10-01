#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <crucible/contracts/GridConfig.hpp>

namespace crucible::blight {
/** Bounded infection grid for the cardinal-spread prototype.
 * Owns its state; callers coordinate mutations and observations sequentially.
 * See docs/workstreams/blight/decisions.md for the prototype rule.
 */
class Grid {
  public:
    /** Constructs an initially clear grid with storage fixed at startup.
     * @param[in] config Finite rectangular geometry validated before allocation.
     * @throws std::invalid_argument Invalid geometry.
     * @throws std::length_error Cell count exceeds either buffer's max_size.
     * @throws std::bad_alloc Startup storage allocation failed.
     */
    explicit Grid(GridConfig config);
    Grid(const Grid &) = delete;
    Grid &operator=(const Grid &) = delete;
    Grid(Grid &&) = delete;
    Grid &operator=(Grid &&) = delete;

    /** Infects one current cell at a coordinator boundary.
     * @param[in] column Zero-based column.
     * @param[in] row Zero-based row.
     * @return False out of bounds with no mutation; true also when already infected.
     */
    [[nodiscard]] bool TrySeed(std::size_t column, std::size_t row) noexcept;

    /// Advances one cardinal-neighbor tick without allocation, then commits the complete grid.
    void Step() noexcept;

    /// Returns the current infected cell count in constant time.
    [[nodiscard]] std::size_t GetInfectedCount() const noexcept;

    /** Observes a current cell without exposing borrowed storage.
     * @param[in] column Zero-based column.
     * @param[in] row Zero-based row.
     * @return Current infection state, or false out of bounds.
     */
    [[nodiscard]] bool IsInfected(std::size_t column, std::size_t row) const noexcept;

  private:
    std::size_t columns_{};
    std::size_t rows_{};
    std::vector<std::uint8_t> current_;
    std::vector<std::uint8_t> next_;
    std::size_t infected_count_{};
};
} // namespace crucible::blight
