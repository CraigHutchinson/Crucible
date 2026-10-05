#include <crucible/runtime/CommandIngress.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace crucible::runtime {
namespace {
std::size_t CheckedCapacity(std::size_t capacity) {
    if (capacity == 0) throw std::invalid_argument("command capacity must be positive");
    if (capacity > std::numeric_limits<std::size_t>::max() / sizeof(AdmittedCommand) ||
        capacity > std::vector<AdmittedCommand>{}.max_size())
        throw std::length_error("command capacity overflows storage");
    return capacity;
}
}

static_assert(std::is_trivially_copyable_v<AdmittedCommand>);

CommandIngress::CommandIngress(Limits limits)
    : ring_(CheckedCapacity(limits.commands)), field_capacity_(limits.fields) {}

CommandIngress::Admission CommandIngress::RejectAdmission(AdmissionStatus status) noexcept {
    if (rejected_ != std::numeric_limits<std::uint64_t>::max()) ++rejected_;
    return {status, 0, 0};
}

template<class Payload>
CommandIngress::Admission CommandIngress::TryAdmitLocked(std::span<const Payload> commands) {
    if (closed_) return RejectAdmission(AdmissionStatus::closed);
    if (commands.empty() || !std::ranges::all_of(commands, [this](const auto& command) {
        return command.IsValid(field_capacity_);
    })) return RejectAdmission(AdmissionStatus::invalid);
    if (commands.size() > ring_.size() - pending_) return RejectAdmission(AdmissionStatus::full);
    if (commands.size() > std::numeric_limits<std::uint64_t>::max() - last_sequence_)
        return RejectAdmission(AdmissionStatus::sequence_exhausted);
    const auto first = last_sequence_ + 1;
    for (const auto& command : commands) {
        const auto tail = pending_ < ring_.size() - head_
            ? head_ + pending_ : pending_ - (ring_.size() - head_);
        ring_[tail] = {BoundaryCommand{command}, ++last_sequence_};
        ++pending_;
    }
    return {AdmissionStatus::accepted, first, last_sequence_};
}
CommandIngress::Admission CommandIngress::TryAdmit(std::span<const FieldEdit> commands) {
    const std::scoped_lock lock{mutex_};
    return TryAdmitLocked(commands);
}
CommandIngress::Admission CommandIngress::TryAdmitCommands(std::span<const BoundaryCommand> commands) {
    const std::scoped_lock lock{mutex_};
    return TryAdmitLocked(commands);
}

CommandIngress::Cutoff CommandIngress::CaptureCutoff() const {
    const std::scoped_lock lock{mutex_};
    return {last_sequence_, closed_};
}

std::optional<std::span<const AdmittedCommand>> CommandIngress::TryDrainThrough(
    Cutoff cutoff, std::span<AdmittedCommand> output) {
    const std::scoped_lock lock{mutex_};
    std::size_t count{};
    auto index = head_;
    while (count < pending_ && ring_[index].sequence <= cutoff.sequence) {
        ++count;
        index = index + 1 == ring_.size() ? 0 : index + 1;
    }
    if (count > output.size()) return std::nullopt;
    for (auto& command : output.first(count)) {
        command = ring_[head_];
        head_ = head_ + 1 == ring_.size() ? 0 : head_ + 1;
    }
    pending_ -= count;
    return std::span<const AdmittedCommand>{output.first(count)};
}

void CommandIngress::Close() {
    const std::scoped_lock lock{mutex_};
    closed_ = true;
}

CommandIngress::Statistics CommandIngress::GetStatistics() const {
    const std::scoped_lock lock{mutex_};
    return {pending_, last_sequence_, rejected_};
}

}
