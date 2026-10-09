#pragma once

#include "crucible/application/frontend_view.hpp"

namespace crucible::application
{
/// Mission-boundary continuation and preferences; contains no live mission state.
struct ProgressProfile
{
    bool hasContinuation_{};
    MissionId nextMission_{MissionId::reclaimFront};
    bool relayUnlocked_{};
    FrontendOptions options_{};
};
}
