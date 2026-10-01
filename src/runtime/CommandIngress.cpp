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

CommandIngress::Admission CommandIngress::TryAdmit(std::span<const FieldEdit> edits) {
    const std::scoped_lock lock{mutex_};
    if (closed_) return RejectAdmission(AdmissionStatus::closed);
    if (edits.empty() || !std::ranges::all_of(edits, [this](const FieldEdit& edit) {
            return edit.IsValid(field_capacity_);
        })) return RejectAdmission(AdmissionStatus::invalid);
    if (edits.size() > ring_.size() - pending_) return RejectAdmission(AdmissionStatus::full);
    if (edits.size() > std::numeric_limits<std::uint64_t>::max() - last_sequence_)
        return RejectAdmission(AdmissionStatus::sequence_exhausted);

    const auto first_sequence = last_sequence_ + 1;
    for (const auto& edit : edits) {
        // Subtract before adding so even capacities near size_t's limit cannot wrap.
        const auto tail = pending_ < ring_.size() - head_
            ? head_ + pending_ : pending_ - (ring_.size() - head_);
        ring_[tail] = {edit, ++last_sequence_};
        ++pending_;
    }
    return {AdmissionStatus::accepted, first_sequence, last_sequence_};
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
