#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <vector>

#include "crucible/fields/FieldSet.hpp"
#include "crucible/scheduling/row_partitions.hpp"
#include "crucible/spatial/Grid.hpp"
#include "crucible/swarm/Steering.hpp"

namespace
{
using namespace crucible;
using scheduling::RowPartitions;
using scheduling::RowRange;

void check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool equalMotion(const SampleState& left, const SampleState& right) noexcept
{
    const auto equalFloat = [](float a, float b)
    {
        return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
    };
    return left.activity == right.activity &&
        equalFloat(left.position.x, right.position.x) && equalFloat(left.position.y, right.position.y) &&
        equalFloat(left.velocity.x, right.velocity.x) && equalFloat(left.velocity.y, right.velocity.y);
}

std::vector<spatial::SpatialSample> gather(std::span<const SampleState> input)
{
    std::vector<spatial::SpatialSample> result;
    result.reserve(input.size());
    for (const auto& sample : input)
    {
        result.push_back({sample.id, sample.position});
    }
    return result;
}

std::vector<SampleState> makeInput(std::size_t count, std::uint64_t firstId)
{
    std::vector<SampleState> result(count);
    for (std::size_t row = 0; row < count; ++row)
    {
        result[row] = {{firstId + row}, row < count / 2 ? Position{4, 4}
            : Position{static_cast<float>(row % 9), static_cast<float>((row * 7) % 9)},
            {static_cast<float>(row % 3) - 1, static_cast<float>(row % 5) - 2}};
    }
    return result;
}

void receiveRelabeling(std::span<const SampleState> input)
{
    check(std::fesetround(FE_TONEAREST) == 0, "Relabeling startup rounding mode unavailable");
    const GridConfig config{8, 8, 1};
    const SampleState sentinel{{777}, {2, 3}, {4, 5}, SampleActivity::lost};
    std::vector<SampleState> sparse(input.begin(), input.end());
    for (std::size_t row = 0; row < sparse.size(); ++row)
    {
        // Monotonic relabeling preserves self/order and coincidence tie direction.
        sparse[row].id = {9001 + row * 31};
    }
    spatial::Grid grid{config, input.size()}, sparseGrid{config, input.size()};
    swarm::Steering steering{config, {1.5F, 6, 16, 4}, input.size()};
    fields::FieldSet fields{2};
    check(fields.TryApplyEdit({FieldEditKind::set, 0, {4, 4}, 6, -9}) == fields::EditResult::applied,
        "Relabeling radial field rejected");
    check(fields.TryApplyEdit({FieldEditKind::set_flow, 1, {1, 4}, 3, 8, {7, 4}}) == fields::EditResult::applied,
        "Relabeling flow field rejected");
    std::vector<SampleState> expected(input.size()), legacy(input.size() + 1, sentinel), pending(input.size() + 1, sentinel);
    const auto partitions = std::max(std::size_t{1}, std::min(input.size(), std::size_t{7}));
    std::vector<std::vector<SampleId>> scratch(partitions, std::vector<SampleId>(input.size()));
    for (const int mode : {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO})
    {
        check(std::fesetround(mode) == 0, "Relabeling rounding mode unavailable");
        std::vector<SampleState> denseRows(input.begin(), input.end()), sparseRows = sparse;
        for (int round = 0; round < 4; ++round)
        {
            check(grid.TryRebuild(gather(denseRows)) && sparseGrid.TryRebuild(gather(sparseRows)),
                "Relabeling index rebuild failed");
            // For more than one row this forces the unchanged sparse-search branch.
            check(steering.TryCompute(sparseRows, fields, sparseGrid, expected), "Sparse-search reference rejected");
            check(steering.TryCompute(denseRows, fields, grid, legacy), "Relabeled legacy computation rejected");
            check(std::ranges::equal(std::span{legacy}.first(input.size()), expected, equalMotion),
                "Legacy dense lookup changed bitwise motion under monotonic relabeling");
            check(equalMotion(legacy.back(), sentinel) && legacy.back().id == sentinel.id, "Legacy output tail changed");
            auto inPlace = denseRows;
            check(steering.TryCompute(inPlace, fields, grid, inPlace), "Relabeled in-place computation rejected");
            check(std::ranges::equal(inPlace, expected, equalMotion), "Dense in-place computation changed bitwise motion");
            for (const std::size_t workers : {1U, 2U, 4U})
            {
                std::ranges::fill(pending, sentinel);
                RowPartitions execution{input.size(), {workers, partitions}, [&](RowRange range)
                {
                    return steering.tryComputeRows(denseRows, fields, grid, range.firstRow,
                        std::span{pending}.subspan(range.firstRow, range.rowCount), scratch[range.partitionIndex]);
                }};
                check(execution.tryRun(input.size()) == RowPartitions::RunStatus::complete,
                    "Relabeling row graph rejected");
                check(std::ranges::equal(std::span{pending}.first(input.size()), expected, equalMotion),
                    "Dense/sparse lookup changed bitwise motion in partition execution");
                for (std::size_t row = 0; row < input.size(); ++row)
                {
                    check(pending[row].id == input[row].id && legacy[row].id == input[row].id,
                        "Relabeling changed authoritative output identity");
                }
                check(equalMotion(pending.back(), sentinel) && pending.back().id == sentinel.id,
                    "Partition output tail changed");
            }
            std::ranges::copy(std::span{legacy}.first(input.size()), denseRows.begin());
            sparseRows = expected;
        }
    }
}

void receiveUnknownIds()
{
    check(std::fesetround(FE_TONEAREST) == 0, "Unknown-ID startup rounding mode unavailable");
    const GridConfig config{8, 8, 1};
    const std::array<SampleState, 3> input{{{{100}, {4, 4}, {}}, {{101}, {4, 4}, {}}, {{102}, {4, 4}, {}}}};
    spatial::Grid grid{config, 4};
    swarm::Steering steering{config, {1.5F, 6, 16, 4}, input.size()};
    fields::FieldSet fields{0};
    const SampleState sentinel{{777}, {2, 3}, {4, 5}, SampleActivity::lost};
    std::array<SampleState, 3> pending{};
    std::array<SampleId, 4> scratch{};
    for (const int mode : {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO})
    {
        check(std::fesetround(mode) == 0, "Unknown-ID rounding mode unavailable");
        for (const std::uint64_t unknown : {99U, 103U})
        {
            auto gathered = gather(input);
            gathered.push_back({{unknown}, {4, 4}});
            check(grid.TryRebuild(gathered), "Unknown-ID index rebuild failed");
            pending.fill(sentinel);
            check(!steering.tryComputeRows(input, fields, grid, 0, pending, scratch),
                "Out-of-interval dense neighbor was accepted");
            check(std::ranges::all_of(pending, [&](const auto& sample)
            {
                return sample.id == sentinel.id && equalMotion(sample, sentinel);
            }), "Unknown dense ID wrote a row before rejection");
            check(!steering.TryCompute(input, fields, grid, pending), "Legacy unknown dense ID was accepted");
            check(std::ranges::all_of(pending, [&](const auto& sample)
            {
                return sample.id == sentinel.id && equalMotion(sample, sentinel);
            }), "Legacy unknown dense ID published staging");
        }
        check(grid.TryRebuild(gather(input)), "Valid dense index restore failed");
        auto invalid = input;
        invalid[1].id = input[0].id;
        pending.fill(sentinel);
        check(!steering.tryComputeRows(invalid, fields, grid, 0, std::span{pending}.first(1), scratch),
            "Dense interval endpoints bypassed strict complete-input validation");
        invalid = input;
        invalid.back().velocity.x = std::numeric_limits<float>::quiet_NaN();
        check(!steering.tryComputeRows(invalid, fields, grid, 0, std::span{pending}.first(1), scratch),
            "Invalid row outside dense partition escaped validation");
        check(std::ranges::all_of(pending, [&](const auto& sample)
        {
            return sample.id == sentinel.id && equalMotion(sample, sentinel);
        }), "Invalid dense input changed staging before validation completed");
    }
}
}

int main()
{
    const int originalMode = std::fegetround();
    for (const std::size_t count : {0U, 1U, 23U, 257U})
    {
        const auto nearMaximum = std::numeric_limits<std::uint64_t>::max() - (count == 0 ? 0 : count - 1);
        for (const auto firstId : std::array<std::uint64_t, 3>{0, 1, nearMaximum})
        {
            const auto input = makeInput(count, firstId);
            receiveRelabeling(input);
            if (count >= 3)
            {
                auto holes = input;
                holes.erase(holes.begin() + static_cast<std::ptrdiff_t>(count / 2));
                receiveRelabeling(holes);
            }
        }
    }
    receiveUnknownIds();
    check(std::fesetround(originalMode) == 0, "Original rounding mode restoration failed");
}
