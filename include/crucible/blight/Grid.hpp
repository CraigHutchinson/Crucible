#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <crucible/contracts/GridConfig.hpp>

namespace crucible::blight {
/** Bounded infection grid for the cardinal-spread prototype.
 * Owns its state; callers coordinate mutations and observations sequentially.
 * See docs/workstreams/blight/decisions.md for the prototype rule.
 */
class Grid {
  public:
    /** Exclusive pending infection step; destruction abandons its changes.
     * Borrows its Grid, which must outlive it. All access is sequential.
     */
    class PreparedStep {
      public:
        /** Transfers the pending-step lease and leaves the source inactive.
         * @param[in,out] other Lease whose pending step is transferred; may be inactive.
         */
        PreparedStep(PreparedStep &&other) noexcept;
        PreparedStep(const PreparedStep &) = delete;
        PreparedStep &operator=(const PreparedStep &) = delete;
        PreparedStep &operator=(PreparedStep &&) = delete;
        /// Releases an uncommitted lease without changing current infection.
        ~PreparedStep();

        /** Observes proposed infection by row-major index.
         * @param[in] index Cell offset.
         * @return False for an inactive lease, out-of-bounds or clear cell.
         */
        [[nodiscard]] bool IsInfected(std::size_t index) const noexcept;
        /** Clears infection only in the pending step.
         * @param[in] index Cell offset.
         * @return False for inactive/out-of-bounds; otherwise true, including already clear.
         */
        [[nodiscard]] bool TryClear(std::size_t index) noexcept;
        /** Publishes a live step without allocation; consumes its lease.
         * @return True on commit; false if already committed or moved from.
         */
        [[nodiscard]] bool Commit() && noexcept;

      private:
        friend class Grid;
        explicit PreparedStep(Grid &owner) noexcept;
        Grid *owner_; // non-owning; the Grid outlives the exclusive lease
    };
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
     * @return False out of bounds or during a prepared lease, with no mutation; true also when already infected.
     */
    [[nodiscard]] bool TrySeed(std::size_t column, std::size_t row) noexcept;

    /** Advances and commits one cardinal-neighbor tick without allocation.
     * @throws std::logic_error A prepared-step lease is outstanding.
     */
    void Step();

    /** Prepares cardinal spread without changing committed infection.
     * @return Exclusive pending step, or nullopt when another lease is outstanding.
     */
    [[nodiscard]] std::optional<PreparedStep> TryPrepareStep() noexcept;

    /// Returns the physical geometry used for resource contact validation.
    [[nodiscard]] constexpr GridConfig GetConfig() const noexcept { return config_; }

    /// Returns the current infected cell count in constant time.
    [[nodiscard]] std::size_t GetInfectedCount() const noexcept;

    /** Observes a current cell without exposing borrowed storage.
     * @param[in] column Zero-based column.
     * @param[in] row Zero-based row.
     * @return Current infection state, or false out of bounds.
     */
    [[nodiscard]] bool IsInfected(std::size_t column, std::size_t row) const noexcept;

  private:
    GridConfig config_{};
    std::vector<std::uint8_t> current_;
    std::vector<std::uint8_t> next_;
    std::size_t infected_count_{};
    std::size_t next_count_{};
    bool prepared_{};
};
} // namespace crucible::blight
