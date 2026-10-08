#include <algorithm>
#include <array>
#include <cfenv>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

#include "crucible/spatial/Grid.hpp"

namespace
{
using namespace crucible;

void check(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

std::vector<SampleId> reference(std::span<const spatial::SpatialSample> samples, Position center, float radius)
{
    center = {std::clamp(center.x, 0.0F, 8.0F), std::clamp(center.y, 0.0F, 6.0F)};
    std::vector<SampleId> result;
    for (const auto& sample : samples) {
        const double x = static_cast<double>(std::clamp(sample.position.x, 0.0F, 8.0F)) - center.x;
        const double y = static_cast<double>(std::clamp(sample.position.y, 0.0F, 6.0F)) - center.y;
        if (x * x + y * y <= static_cast<double>(radius) * radius) result.push_back(sample.id);
    }
    std::ranges::sort(result);
    return result;
}
}

int main()
{
    using namespace crucible;
    constexpr std::size_t count = 129;
    std::array<spatial::SpatialSample, count> samples{};
    for (std::size_t row = 0; row < count; ++row)
        samples[row] = {{10007 - row * 7}, row < 65 ? Position{4, 3} :
            Position{static_cast<float>(row % 11) - 1, static_cast<float>((row * 3) % 9) - 1}};
    spatial::Grid grid{{8, 6, 1}, count};
    const auto& immutable = grid;
    const SampleId sentinel{999999};
    std::array<SampleId, count + 2> first{}, second{};
    const auto firstPointer = first.data();
    const int originalMode = std::fegetround();
    for (const int mode : {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
        check(std::fesetround(mode) == 0, "rounding mode unavailable");
        check(grid.TryRebuild(samples), "caller-scratch rebuild rejected");
        for (const auto center : {Position{4, 3}, Position{-10, -10}, Position{20, 20}, Position{0, 6}}) {
            for (const float radius : {0.0F, .01F, 1.0F, 2.01F, std::numeric_limits<float>::max()}) {
                first.fill(sentinel);
                const auto expected = reference(samples, center, radius);
                const auto result = immutable.tryQuery(center, radius, first);
                check(result && std::ranges::equal(*result, expected), "caller query differs from independent complete scan");
                check(result->data() == firstPointer && std::ranges::all_of(std::span{first}.subspan(result->size()),
                    [&](SampleId id) { return id == sentinel; }), "query changed pointer or unused scratch tail");
                const auto retained = first;
                check(immutable.tryQuery({0, 0}, 1, second).has_value(), "second scratch query rejected");
                check(first == retained, "independent scratch invalidated retained result");
                const auto owned = grid.TryQuery(center, radius);
                check(owned && std::ranges::equal(*owned, expected) && first == retained,
                    "legacy query changed caller storage or result parity");
            }
        }
        first.fill(sentinel);
        for (const auto radius : {-1.0F, std::numeric_limits<float>::infinity(),
                                  std::numeric_limits<float>::quiet_NaN()})
            check(!immutable.tryQuery({4, 3}, radius, first) &&
                std::ranges::all_of(first, [&](SampleId id) { return id == sentinel; }), "invalid radius wrote scratch");
        check(!immutable.tryQuery({std::numeric_limits<float>::quiet_NaN(), 3}, 1, first), "invalid center accepted");
        check(!immutable.tryQuery({4, 3}, 0, std::span{first}.first(count - 1)) &&
            std::ranges::all_of(first, [&](SampleId id) { return id == sentinel; }),
            "insufficient complete scratch changed values or silently truncated");
        // An index built in nearest mode must still support the exact-scan query fallback.
        check(std::fesetround(FE_UPWARD) == 0, "fallback mode unavailable");
        const auto fallback = immutable.tryQuery({4, 3}, 1, first);
        check(fallback && std::ranges::equal(*fallback, reference(samples, {4, 3}, 1)), "query rounding fallback differs");
    }
    check(std::fesetround(originalMode) == 0, "rounding mode restore failed");
    check(grid.TryRebuild({}), "empty rebuild rejected");
    const auto empty = immutable.tryQuery({0, 0}, 0, {});
    check(empty && empty->empty(), "empty index needs nonempty query scratch");
}
