#pragma once

#include <crucible/runtime/HeadlessSession.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace crucible::runtime {

/** Drives bounded fixed-step boundaries from caller-supplied elapsed time.
 * Borrows HeadlessSession exclusively; session and Simulation must outlive this
 * driver. Only ingress operations may run concurrently. Exact replay uses the
 * session directly after this driver is no longer in use.
 */
class ClockDriver {
public:
    /// Blocked is latched until close; invalid_elapsed leaves all state unchanged.
    enum class Status { running, paused, closed, blocked, invalid_elapsed };
    /// Owned completed-boundary observations; discarded units are 1/60 nanosecond.
    struct Summary {
        std::uint64_t completed_tick{};
        std::size_t applied_commands{}, ingress_pending{};
        std::uint64_t ingress_accepted{}, ingress_rejected{}, discarded_scaled_nanoseconds{};
    };
    /// boundary_status is populated only when this pump attempted a boundary.
    struct PumpResult {
        Status status{};
        std::size_t advanced_ticks{};
        std::optional<HeadlessSession::StepStatus> boundary_status;
        Summary summary{};
    };

    explicit ClockDriver(HeadlessSession& session) noexcept;
    ClockDriver(const ClockDriver&) = delete;
    ClockDriver& operator=(const ClockDriver&) = delete;

    /** Runs at most four ticks at 60 Hz, then discards whole overdue ticks.
     * Fractional time carries exactly. Negative elapsed or scaled addition overflow
     * is rejected before mutation. A rejected boundary latches blocked without
     * consuming its time; a throwing boundary latches blocked and rethrows.
     */
    [[nodiscard]] PumpResult TryPump(std::chrono::nanoseconds elapsed);
    /// Discards the carried fraction; paused pumps discard valid elapsed time.
    void Pause() noexcept;
    /// Resumes with an empty time baseline. Caller supplies only time since resume.
    void Resume() noexcept;
    /// Idempotently closes ingress, discards backlog and stops future boundaries.
    void Close();
    /// Returns the latched lifecycle state without pumping time.
    [[nodiscard]] constexpr Status GetStatus() const noexcept { return status_; }
    [[nodiscard]] Summary GetSummary() const;

private:
    void DiscardTime(std::uint64_t scaled_nanoseconds) noexcept;

    HeadlessSession& session_; // non-owning; exclusive coordinator borrow
    Status status_{Status::running};
    std::uint64_t remainder_{}, discarded_{};
};

}
