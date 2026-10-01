#include <crucible/runtime/HeadlessSession.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include <crucible/simulation.hpp>

namespace crucible::runtime {
static_assert(std::is_trivially_copyable_v<AppliedCommand>);

HeadlessSession::ValidatedLimits HeadlessSession::ValidateLimits(Limits limits) {
    if (limits.commands == 0) throw std::invalid_argument("command capacity must be positive");
    constexpr auto maximum = std::numeric_limits<std::size_t>::max();
    constexpr auto command_bytes = 2 * sizeof(AdmittedCommand);
    if (limits.commands > maximum / command_bytes ||
        limits.commands > std::vector<AdmittedCommand>{}.max_size() ||
        limits.trace > std::vector<AppliedCommand>{}.max_size() ||
        limits.trace > (maximum - limits.commands * command_bytes) / sizeof(AppliedCommand))
        throw std::length_error("session capacity overflows storage");
    return ValidatedLimits{limits};
}

HeadlessSession::HeadlessSession(Simulation& simulation, Limits limits)
    : HeadlessSession(simulation, ValidateLimits(limits)) {}

HeadlessSession::HeadlessSession(Simulation& simulation, ValidatedLimits limits)
    : simulation_(simulation), ingress_({limits.value.commands, simulation.GetFieldCapacity()}),
      boundary_(limits.value.commands), trace_(limits.value.trace) {}

CommandIngress& HeadlessSession::GetIngress() noexcept { return ingress_; }
std::uint64_t HeadlessSession::GetCompletedTick() const noexcept { return tick_; }
void HeadlessSession::Pause() noexcept { paused_ = true; }
void HeadlessSession::Resume() noexcept { paused_ = false; }

void HeadlessSession::CompleteTick() {
    try {
        simulation_.tick();
        ++tick_;
    } catch (...) {
        failed_ = true;
        ingress_.Close();
        throw;
    }
}

HeadlessSession::StepResult HeadlessSession::TryStep() {
    if (failed_) return {StepStatus::application_failed, tick_, 0};
    const auto cutoff = ingress_.CaptureCutoff();
    if (cutoff.closed) return {StepStatus::closed, tick_, 0};
    if (paused_) return {StepStatus::paused, tick_, 0};
    if (tick_ == std::numeric_limits<std::uint64_t>::max())
        return {StepStatus::tick_exhausted, tick_, 0};

    const auto available = std::min(boundary_.size(), trace_.size() - trace_size_);
    const auto drained = ingress_.TryDrainThrough(cutoff, std::span{boundary_}.first(available));
    if (!drained) return {StepStatus::trace_full, tick_, 0};
    std::size_t applied{};
    for (const auto& command : *drained) {
        if (!simulation_.TryApplyFieldEdit(command.edit)) {
            failed_ = true;
            ingress_.Close();
            return {StepStatus::application_failed, tick_, applied};
        }
        ++applied;
    }
    CompleteTick();
    for (const auto& command : *drained)
        trace_[trace_size_++] = {command.edit, command.sequence, tick_};
    return {StepStatus::advanced, tick_, applied};
}

std::span<const AppliedCommand> HeadlessSession::GetTrace() const noexcept {
    return std::span{trace_}.first(trace_size_);
}

HeadlessSession::StepResult HeadlessSession::TryReplay(
    std::span<const AppliedCommand> trace, std::uint64_t target_ticks) {
    if (failed_) return {StepStatus::application_failed, tick_, 0};
    if (paused_) return {StepStatus::paused, tick_, 0};
    if (tick_ != 0 || trace_size_ != 0 || ingress_.GetStatistics().accepted != 0)
        return {StepStatus::replay_invalid, tick_, 0};
    if (trace.size() > trace_.size()) return {StepStatus::trace_full, tick_, 0};

    std::uint64_t sequence{}, previous_tick{};
    for (const auto& command : trace) {
        if (sequence == std::numeric_limits<std::uint64_t>::max() ||
            command.sequence != sequence + 1 || command.tick == 0 ||
            command.tick < previous_tick || command.tick > target_ticks ||
            !command.edit.IsValid(simulation_.GetFieldCapacity()))
            return {StepStatus::replay_invalid, tick_, 0};
        sequence = command.sequence;
        previous_tick = command.tick;
    }

    ingress_.Close();
    auto remaining = trace;
    while (tick_ < target_ticks) {
        std::size_t boundary_count{};
        while (boundary_count < remaining.size() && remaining[boundary_count].tick == tick_ + 1) {
            if (!simulation_.TryApplyFieldEdit(remaining[boundary_count].edit)) {
                failed_ = true;
                return {StepStatus::application_failed, tick_, trace_size_ + boundary_count};
            }
            ++boundary_count;
        }
        CompleteTick();
        for (const auto& command : remaining.first(boundary_count)) trace_[trace_size_++] = command;
        remaining = remaining.subspan(boundary_count);
    }
    return {StepStatus::advanced, tick_, trace_size_};
}

}
