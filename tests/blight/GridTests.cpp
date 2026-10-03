#include <crucible/blight/Grid.hpp>

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <type_traits>

namespace {
using crucible::blight::Grid;

bool CheckGrid(const Grid &grid, std::size_t columns, std::string_view cells) {
    std::size_t count = 0;
    for (std::size_t index = 0; index < cells.size(); ++index) {
        const bool expected = cells[index] == '#';
        if (grid.IsInfected(index % columns, index / columns) != expected) {
            return false;
        }
        count += static_cast<std::size_t>(expected);
    }
    return grid.GetInfectedCount() == count;
}

bool Rejects(crucible::GridConfig config) {
    try {
        Grid grid{config};
        return false;
    } catch (const std::invalid_argument &) {
        return true;
    }
}
bool PreparedFixtures() {
    static_assert(!std::is_copy_constructible_v<Grid::PreparedStep>);
    static_assert(std::is_nothrow_move_constructible_v<Grid::PreparedStep>);
    Grid grid{{3, 1, 1.0F}};
    if (!grid.TrySeed(0, 0)) return false;
    {
        auto pending = grid.TryPrepareStep();
        if (!pending || !pending->IsInfected(0) || !pending->IsInfected(1) || pending->IsInfected(2) ||
            grid.TryPrepareStep() || grid.TrySeed(2, 0) || !CheckGrid(grid, 3, "#..")) return false;
        bool rejected = false;
        try { grid.Step(); } catch (const std::logic_error&) { rejected = true; }
        if (!rejected || !pending->TryClear(1) || pending->TryClear(3) || !CheckGrid(grid, 3, "#..")) return false;
        auto moved = std::move(*pending);
        if (pending->TryClear(0) || pending->IsInfected(0) || std::move(*pending).Commit()) return false;
        if (!moved.IsInfected(0) || moved.IsInfected(1)) return false;
        // Abandon the moved lease; current values and cached count stay unchanged.
    }
    if (!CheckGrid(grid, 3, "#..")) return false;
    auto pending = grid.TryPrepareStep();
    if (!pending || !pending->TryClear(0) || !std::move(*pending).Commit() ||
        std::move(*pending).Commit() || !CheckGrid(grid, 3, ".#.")) return false;
    // Destruction of the consumed guard must not release a newer active lease.
    auto newer = grid.TryPrepareStep();
    pending.reset();
    if (!newer || grid.TryPrepareStep()) return false;
    if (!std::move(*newer).Commit() || !CheckGrid(grid, 3, "###")) return false;
    return true;
}
} // namespace

int main() {
    // Fixtures are manually derived, independent of the step implementation.
    Grid center{{3, 3, 1.0F}};
    if (!center.TrySeed(1, 1) || !center.TrySeed(1, 1) || !CheckGrid(center, 3, "....#...."))
        return 1;
    center.Step();
    if (!CheckGrid(center, 3, ".#.###.#."))
        return 2;
    center.Step();
    if (!CheckGrid(center, 3, "#########"))
        return 3;

    Grid border{{4, 2, 2.0F}};
    if (!border.TrySeed(0, 0))
        return 4;
    constexpr std::array border_ticks{"#.......", "##..#...", "###.##..", "#######.", "########"};
    for (const auto *expected : border_ticks) {
        if (!CheckGrid(border, 4, expected))
            return 5;
        border.Step();
    }

    Grid horizontal{{5, 1, 1.0F}};
    if (!horizontal.TrySeed(0, 0))
        return 6;
    constexpr std::array line_ticks{"#....", "##...", "###..", "####.", "#####"};
    for (const auto *expected : line_ticks) {
        if (!CheckGrid(horizontal, 5, expected))
            return 7;
        horizontal.Step();
    }
    Grid vertical{{1, 5, 1.0F}};
    if (!vertical.TrySeed(0, 0))
        return 8;
    for (const auto *expected : line_ticks) {
        if (!CheckGrid(vertical, 1, expected))
            return 9;
        vertical.Step();
    }

    Grid empty{{4, 3, 1.0F}};
    for (int tick = 0; tick < 4; ++tick)
        empty.Step();
    if (!CheckGrid(empty, 4, "............") || empty.TrySeed(4, 0) || empty.TrySeed(0, 3) ||
        empty.TrySeed(std::numeric_limits<std::size_t>::max(), 0) || empty.IsInfected(4, 0) ||
        empty.IsInfected(0, 3) || empty.GetInfectedCount() != 0)
        return 10;

    Grid dense{{3, 2, 1.0F}};
    if (!dense.TrySeed(0, 0) || !dense.TrySeed(2, 0) || !dense.TrySeed(1, 1) ||
        !CheckGrid(dense, 3, "#.#.#."))
        return 11;
    dense.Step();
    if (!CheckGrid(dense, 3, "######"))
        return 12;
    dense.Step();
    if (!CheckGrid(dense, 3, "######"))
        return 13;
    Grid singleton{{1, 1, 1.0F}};
    singleton.Step();
    if (!CheckGrid(singleton, 1, ".") || !singleton.TrySeed(0, 0))
        return 14;
    singleton.Step();
    if (!CheckGrid(singleton, 1, "#"))
        return 15;

    Grid seeded_later{{5, 1, 1.0F}};
    seeded_later.Step();
    if (!seeded_later.TrySeed(4, 0))
        return 16;
    seeded_later.Step();
    if (!CheckGrid(seeded_later, 5, "...##"))
        return 17;

    Grid full{{2, 2, 1.0F}};
    for (std::size_t row = 0; row < 2; ++row) {
        for (std::size_t column = 0; column < 2; ++column) {
            if (!full.TrySeed(column, row))
                return 18;
        }
    }
    full.Step();
    if (!CheckGrid(full, 2, "####"))
        return 19;

    if (!Rejects({0, 1, 1.0F}) || !Rejects({1, 0, 1.0F}) || !Rejects({1, 1, 0.0F}) ||
        !Rejects({1, 1, -1.0F}) || !Rejects({1, 1, std::numeric_limits<float>::infinity()}) ||
        !Rejects({1, 1, std::numeric_limits<float>::quiet_NaN()}) ||
        !Rejects({std::numeric_limits<std::size_t>::max(), 2, 1.0F}) ||
        !Rejects({2, 1, std::numeric_limits<float>::max()}))
        return 20;

    try {
        Grid too_large{{std::numeric_limits<std::size_t>::max(), 1, 1.0F}};
        return 21;
    } catch (const std::length_error &) {
        // Geometry fits, but storage must be rejected before attempting allocation.
    }

    if (!PreparedFixtures()) return 22;

    std::cout << "Blight hand-calculated fixtures passed\n";
}
