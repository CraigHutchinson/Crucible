#pragma once

#include <cstdint>
#include <compare>
#include <type_traits>

namespace crucible {
/// Stable application sample identity; never an ECS row or borrowed pointer.
struct SampleId {
    std::uint64_t value{};
    auto operator<=>(const SampleId&) const = default;
};
static_assert(std::is_trivially_copyable_v<SampleId>);
}
