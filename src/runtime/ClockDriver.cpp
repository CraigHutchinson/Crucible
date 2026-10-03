#include <crucible/runtime/ClockDriver.hpp>

#include <limits>
#include <utility>

namespace crucible::runtime {

ClockDriver::ClockDriver(HeadlessSession& session, std::function<bool()> stop_after_boundary) noexcept
    : session_(session), stop_after_boundary_(std::move(stop_after_boundary)) {}

void ClockDriver::DiscardTime(std::uint64_t scaled_nanoseconds) noexcept {
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    discarded_ += scaled_nanoseconds > maximum - discarded_
        ? maximum - discarded_ : scaled_nanoseconds;
}

ClockDriver::Summary ClockDriver::GetSummary() const {
    const auto ingress = session_.GetIngress().GetStatistics();
    return {session_.GetCompletedTick(), session_.GetTrace().size(), ingress.pending,
        ingress.accepted, ingress.rejected, discarded_};
}

ClockDriver::PumpResult ClockDriver::TryPump(std::chrono::nanoseconds elapsed) {
    constexpr std::uint64_t frequency = 60;
    constexpr std::uint64_t tick_units = 1'000'000'000;
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (elapsed.count() < 0 ||
        static_cast<std::uint64_t>(elapsed.count()) > (maximum - remainder_) / frequency)
        return {Status::invalid_elapsed, 0, std::nullopt, GetSummary()};

    if (status_ == Status::closed || status_ == Status::blocked)
        return {status_, 0, std::nullopt, GetSummary()};
    const auto scaled = static_cast<std::uint64_t>(elapsed.count()) * frequency;
    if (status_ == Status::paused) {
        DiscardTime(scaled);
        return {status_, 0, std::nullopt, GetSummary()};
    }

    remainder_ += scaled;
    std::size_t advanced{};
    std::optional<HeadlessSession::StepStatus> boundary;
    while (remainder_ >= tick_units && advanced < 4) {
        HeadlessSession::StepResult result;
        try {
            result = session_.TryStep();
        } catch (...) {
            status_ = Status::blocked;
            throw;
        }
        boundary = result.status;
        if (result.status != HeadlessSession::StepStatus::advanced) {
            status_ = result.status == HeadlessSession::StepStatus::closed
                ? Status::closed : Status::blocked;
            return {status_, advanced, boundary, GetSummary()};
        }
        remainder_ -= tick_units;
        ++advanced;
        try {
            if (stop_after_boundary_ && stop_after_boundary_()) {
                Close();
                return {status_, advanced, boundary, GetSummary()};
            }
        } catch (...) {
            status_ = Status::blocked;
            throw;
        }
    }
    const auto excess = remainder_ - remainder_ % tick_units;
    DiscardTime(excess);
    remainder_ -= excess;
    return {status_, advanced, boundary, GetSummary()};
}

void ClockDriver::Pause() noexcept {
    if (status_ != Status::running) return;
    DiscardTime(remainder_);
    remainder_ = 0;
    status_ = Status::paused;
    session_.Pause();
}

void ClockDriver::Resume() noexcept {
    if (status_ != Status::paused) return;
    status_ = Status::running;
    session_.Resume();
}

void ClockDriver::Close() {
    session_.GetIngress().Close();
    DiscardTime(remainder_);
    remainder_ = 0;
    status_ = Status::closed;
}

}
