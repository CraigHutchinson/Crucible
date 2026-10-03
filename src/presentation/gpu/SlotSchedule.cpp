#include "SlotSchedule.hpp"
#include <limits>
namespace crucible::presentation::gpu::detail {
std::optional<std::size_t> SlotSchedule::FindAvailable() const noexcept {
    for (std::size_t i = 0; i < Count; ++i)
        if (!entries_[i].submitted || entries_[i].complete) return i;
    return std::nullopt;
}
bool SlotSchedule::CanSubmit() const noexcept { return next_id_ != std::numeric_limits<std::uint64_t>::max(); }
std::uint64_t SlotSchedule::Submit(std::size_t slot, std::uint64_t tick) noexcept {
    const auto id = next_id_++;
    entries_[slot] = {id, tick, true, false};
    return id;
}
void SlotSchedule::Complete(std::size_t slot) noexcept { entries_[slot].complete = true; }
bool SlotSchedule::Matches(std::size_t slot, std::uint64_t id) const noexcept {
    return slot < Count && id != 0 && entries_[slot].submitted && entries_[slot].id == id;
}
}
