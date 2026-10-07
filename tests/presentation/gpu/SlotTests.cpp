#include "SlotSchedule.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
namespace {
using crucible::presentation::gpu::detail::SlotSchedule;
void Require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
void Check() {
    SlotSchedule schedule;
    std::array<std::uint64_t, 3> identities{};
    for (std::size_t i = 0; i < 3; ++i) {
        Require(schedule.FindAvailable() == i, "three bounded initial slots");
        identities[i] = schedule.Submit(i, 100 + i);
    }
    Require(!schedule.FindAvailable(), "delayed completion produces busy rather than fourth slot");
    // Busy and failed-before-submission operations make no schedule publication.
    for (std::size_t i = 0; i < 3; ++i)
        Require(schedule.Matches(i, identities[i]) && schedule.Get(i).tick == 100 + i && !schedule.Get(i).complete,
            "busy/failure retain submitted identities");
    schedule.Complete(1);
    Require(schedule.FindAvailable() == 1, "only retired slot reusable");
    const auto replacement = schedule.Submit(1, 500);
    Require(!schedule.Matches(1, identities[1]) && schedule.Matches(1, replacement), "receipt expires on slot reuse");
    Require(schedule.Matches(0, identities[0]) && !schedule.Matches(3, identities[0]) &&
        !schedule.Matches(0, 0), "bounds and identity rejection");
    for (std::size_t i = 0; i < 3; ++i) schedule.Complete(i);
    Require(schedule.FindAvailable().has_value(), "drained replacement can reuse startup slots");
}
}
int main() { try { Check(); } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
