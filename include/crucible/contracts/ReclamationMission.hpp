#pragma once
#include <cstdint>

namespace crucible {
/// Reference challenge tuning; deadlines count completed fixed ticks, never wall time.
struct ReclamationMissionSettings {
    std::uint64_t target_reclaimed{1780};
    std::uint64_t deadline_ticks{900};
};
enum class ReclamationMissionOutcome { active, won, lost };
/// Owned display value evaluated after a completed tick; terminal outcomes are latched.
struct ReclamationMissionProgress {
    ReclamationMissionSettings settings{};
    std::uint64_t reclaimed{}, completed_tick{};
    ReclamationMissionOutcome outcome{ReclamationMissionOutcome::active};
};
}
