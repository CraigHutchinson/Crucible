#pragma once

#include <type_traits>

#include <crucible/contracts/Position.hpp>
#include <crucible/contracts/SampleId.hpp>
#include <crucible/contracts/Velocity.hpp>

namespace crucible {
enum class SampleActivity { mobile, anchored, lost };
/// Owned fixed-population sample; IDs are values, never ECS row offsets.
/// Steering reads strictly ascending IDs from one immutable tick-start gather.
struct SampleState {
    SampleId id{};
    Position position{};
    Velocity velocity{};
    SampleActivity activity{SampleActivity::mobile};
};
static_assert(std::is_trivially_copyable_v<SampleState>);
}
