#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <thread>
#include <vector>

#include "crucible/fields/FieldSet.hpp"
#include "crucible/spatial/Grid.hpp"
#include "crucible/swarm/Steering.hpp"

namespace
{
using namespace crucible;

void check(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

bool equalBits(const SampleState& left, const SampleState& right) noexcept
{
    const auto equalFloat = [](float a, float b) noexcept {
        return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
    };
    return left.id == right.id && left.activity == right.activity &&
        equalFloat(left.position.x, right.position.x) && equalFloat(left.position.y, right.position.y) &&
        equalFloat(left.velocity.x, right.velocity.x) && equalFloat(left.velocity.y, right.velocity.y);
}

std::vector<spatial::SpatialSample> gather(std::span<const SampleState> input)
{
    std::vector<spatial::SpatialSample> result;
    result.reserve(input.size());
    for (const auto& sample : input) result.push_back({sample.id, sample.position});
    return result;
}

void receivePartitions(std::size_t count)
{
    const GridConfig config{8, 8, 1};
    std::vector<SampleState> input(count), expected(count), pending(count);
    for (std::size_t row = 0; row < count; ++row) {
        // Sparse IDs and a coincident cluster cross every proposed partition boundary.
        input[row] = {{1000001 + row * 997},
            row < count / 2 ? Position{4, 4} : Position{static_cast<float>(row % 9), static_cast<float>((row * 7) % 9)},
            {static_cast<float>(row % 3) - 1, static_cast<float>(row % 5) - 2}};
    }
    spatial::Grid grid{config, count};
    swarm::Steering steering{config, {1.5F, 6, 16, 4}, count};
    fields::FieldSet fields{2};
    check(fields.TryApplyEdit({FieldEditKind::set, 0, {4, 4}, 6, -9}) == fields::EditResult::applied,
        "radial fixture rejected");
    check(fields.TryApplyEdit({FieldEditKind::set_flow, 1, {1, 4}, 3, 8, {7, 4}}) == fields::EditResult::applied,
        "flow fixture rejected");
    const auto spatialInput = gather(input);
    const auto& immutableGrid = grid;
    const auto& immutableSteering = steering;
    const int originalMode = std::fegetround();
    for (const int mode : {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
        check(std::fesetround(mode) == 0, "rounding mode unavailable");
        check(grid.TryRebuild(spatialInput), "partition fixture rebuild failed");
        check(steering.TryCompute(input, fields, grid, expected), "sequential fixture failed");
        for (const std::size_t workers : {1U, 2U, 7U}) {
            std::vector<std::vector<SampleId>> scratch(workers, std::vector<SampleId>(count));
            std::vector<int> success(workers);
            std::vector<std::jthread> tasks;
            tasks.reserve(workers);
            for (std::size_t partition = 0; partition < workers; ++partition) {
                const auto first = count * partition / workers;
                const auto last = count * (partition + 1) / workers;
                tasks.emplace_back([&, partition, first, last, mode] {
                    const int savedMode = std::fegetround();
                    const bool configured = std::fesetround(mode) == 0;
                    success[partition] = configured && immutableSteering.tryComputeRows(input, fields,
                        immutableGrid, first, std::span{pending}.subspan(first, last - first), scratch[partition]);
                    if (savedMode != -1) static_cast<void>(std::fesetround(savedMode));
                });
            }
            tasks.clear();
            check(std::ranges::all_of(success, [](int value) { return value != 0; }), "partition failed");
            check(std::ranges::equal(pending, expected, equalBits), "partition arithmetic/order differs bitwise");
        }
    }
    check(std::fesetround(originalMode) == 0, "original rounding mode restore failed");
}

void receiveRejections()
{
    const GridConfig config{8, 8, 1};
    const std::array<SampleState, 3> input{{{{7}, {1, 1}, {}}, {{99}, {4, 4}, {}}, {{7001}, {7, 7}, {}}}};
    spatial::Grid grid{config, 4};
    swarm::Steering steering{config, {1, 6, 16, 4}, 3};
    fields::FieldSet fields{0};
    check(grid.TryRebuild(gather(input)), "rejection rebuild failed");
    const SampleState sentinel{{99999}, {2, 3}, {4, 5}, SampleActivity::lost};
    std::array<SampleState, 4> pending{sentinel, sentinel, sentinel, sentinel};
    std::array<SampleId, 4> scratch{};
    const auto unchanged = [&] {
        return std::ranges::all_of(pending, [&](const auto& value) { return equalBits(value, sentinel); });
    };
    for (const auto first : std::array<std::size_t, 2>{4, std::numeric_limits<std::size_t>::max()})
        check(!steering.tryComputeRows(input, fields, grid, first, std::span{pending}.first(1), scratch) && unchanged(),
            "invalid row offset wrote staging");
    check(!steering.tryComputeRows(input, fields, grid, 2, std::span{pending}.first(2), scratch) && unchanged(),
        "excess row count wrote staging");
    check(!steering.tryComputeRows(input, fields, grid, 0, std::span{pending}.first(3), std::span{scratch}.first(2)) && unchanged(),
        "short scratch wrote staging");
    auto invalid = input;
    invalid.back().velocity.y = std::numeric_limits<float>::infinity();
    check(!steering.tryComputeRows(invalid, fields, grid, 0, std::span{pending}.first(1), scratch) && unchanged(),
        "nonfinite row outside assigned range escaped complete-input validation");
    invalid = input;
    invalid.back().id = input.front().id;
    check(!steering.tryComputeRows(invalid, fields, grid, 0, std::span{pending}.first(1), scratch) && unchanged(),
        "unsorted input outside assigned range wrote staging");
    auto aliased = input;
    check(!steering.tryComputeRows(aliased, fields, grid, 0, std::span{aliased}.first(1), scratch) &&
        std::ranges::equal(aliased, input, equalBits), "input/output overlap accepted");
    check(steering.tryComputeRows(input, fields, grid, input.size(), {}, scratch) && unchanged(), "empty tail range failed");

    auto wrongGrid = gather(input);
    wrongGrid.push_back({{7002}, input.back().position});
    check(grid.TryRebuild(wrongGrid), "unknown-neighbor fixture failed");
    check(!steering.tryComputeRows(input, fields, grid, 0, std::span{pending}.first(3), scratch),
        "late unknown neighbor accepted");
    check(!equalBits(pending.front(), sentinel) && equalBits(pending[2], sentinel),
        "partial staging contract not exercised");
    pending.fill(sentinel);
    check(!steering.TryCompute(input, fields, grid, pending) && unchanged(), "legacy failure published partial staging");
    check(grid.TryRebuild(gather(input)), "valid grid restore failed");
    auto inPlace = input;
    check(steering.TryCompute(inPlace, fields, grid, inPlace), "legacy in-place computation failed");
    check(steering.tryComputeRows(input, fields, grid, 0, std::span{pending}.first(3), scratch), "valid row recovery failed");
    check(std::ranges::equal(std::span{pending}.first(3), inPlace, equalBits) && equalBits(pending.back(), sentinel),
        "success changed staging tail or legacy overlap result");
}
}

int main()
{
    for (const std::size_t count : {0U, 1U, 23U, 257U}) receivePartitions(count);
    receiveRejections();
}
