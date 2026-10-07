#include <crucible/interactions/Reclamation.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

#include <crucible/blight/Grid.hpp>

namespace {
using crucible::BiomassLedger;
using crucible::GridConfig;
using crucible::ResourceSettings;
using crucible::SampleState;
using crucible::blight::Grid;
using crucible::interactions::Reclamation;

#define CHECK(expression) do { if (!(expression)) { std::cerr << #expression << '\n'; return false; } } while (false)

SampleState Sample(std::uint64_t id, float x = 0.5F, float y = 0.5F) {
    return {{id}, {x, y}, {}};
}

bool AccountingFixtures() {
    // Expected numbers are hand-calculated biomass transfers, not production-derived totals.
    for (const auto stock : {std::uint64_t{3}, std::uint64_t{1}, std::uint64_t{0}}) {
        const GridConfig config{1, 1, 1.0F};
        Grid blight{config};
        Reclamation reclaim{config, 2, {stock, 1, 0, 2}};
        std::array samples{Sample(2), Sample(1)};
        CHECK(blight.TrySeed(0, 0));
        CHECK(reclaim.TryStep(blight, samples));
        const auto ledger = reclaim.GetLedger();
        if (stock == 3) {
            CHECK((ledger == BiomassLedger{5, 1, 2, 2, 2, 2}));
            CHECK(blight.IsInfected(0, 0));
        } else if (stock == 1) {
            CHECK((ledger == BiomassLedger{3, 0, 2, 1, 1, 1}));
            CHECK(!blight.IsInfected(0, 0));
        } else {
            CHECK((ledger == BiomassLedger{2, 0, 2, 0, 0, 1}));
            CHECK(!blight.IsInfected(0, 0));
        }
        CHECK(ledger.initial_total == ledger.remaining_stock + ledger.mobile_mass + ledger.reserve);
        CHECK(reclaim.GetStocks()[0] == ledger.remaining_stock);
        const auto previous = ledger;
        if (stock != 3) {
            CHECK(reclaim.TryStep(blight, samples));
            CHECK(reclaim.GetLedger() == previous);
            CHECK(blight.TrySeed(0, 0));
            CHECK(reclaim.TryStep(blight, samples));
            CHECK(reclaim.GetLedger().reserve == previous.reserve);
            CHECK(reclaim.GetLedger().work_actions == previous.work_actions + 1);
        }
    }

    Grid clear{{1, 1, 1.0F}};
    Reclamation untouched{{1, 1, 1.0F}, 2, {3, 1, 0, 2}};
    std::array pair{Sample(1), Sample(2)};
    CHECK(untouched.TryStep(clear, pair));
    CHECK((untouched.GetLedger() == BiomassLedger{5, 3, 2, 0, 0, 0}));
    CHECK(clear.TrySeed(0, 0));
    Reclamation disabled{{1, 1, 1.0F}, 2, {3, 1, 0, 0}};
    CHECK(disabled.TryStep(clear, pair));
    CHECK((disabled.GetLedger() == BiomassLedger{5, 3, 2, 0, 0, 0}));
    CHECK(clear.IsInfected(0, 0));
    return true;
}

bool SpreadAndBudgetFixtures() {
    const GridConfig config{3, 1, 1.0F};
    Grid no_swarm{config};
    Reclamation empty{config, 0, {2, 1, 0, 64}};
    CHECK(no_swarm.TrySeed(0, 0));
    CHECK(empty.TryStep(no_swarm, {}));
    CHECK(no_swarm.IsInfected(0, 0) && no_swarm.IsInfected(1, 0) && !no_swarm.IsInfected(2, 0));
    CHECK((empty.GetLedger() == BiomassLedger{6, 6, 0, 0, 0, 0}));

    Grid arrival{config};
    Reclamation arrived{config, 1, {2, 1, 0, 64}};
    const std::array one{Sample(1, 1.5F)};
    CHECK(arrival.TrySeed(0, 0));
    CHECK(arrived.TryStep(arrival, one));
    CHECK((arrived.GetLedger() == BiomassLedger{7, 5, 1, 1, 1, 1}));
    CHECK((std::ranges::equal(arrived.GetStocks(), std::array<std::uint64_t, 3>{2, 1, 2})));
    CHECK(arrival.IsInfected(1, 0) && !arrival.IsInfected(2, 0));
    CHECK(arrived.TryStep(arrival, one));
    CHECK(!arrival.IsInfected(1, 0));
    CHECK(arrival.IsInfected(2, 0));
    CHECK(arrived.TryStep(arrival, one));
    CHECK((arrived.GetLedger() == BiomassLedger{7, 4, 1, 2, 2, 3}));
    CHECK(!arrival.IsInfected(1, 0));

    Grid priority{{2, 1, 1.0F}};
    Reclamation bounded{{2, 1, 1.0F}, 2, {2, 1, 0, 1}};
    const std::array contenders{Sample(1, 1.5F), Sample(2, 0.5F)};
    CHECK(priority.TrySeed(0, 0));
    CHECK(bounded.TryStep(priority, contenders));
    CHECK((std::ranges::equal(bounded.GetStocks(), std::array<std::uint64_t, 2>{1, 2})));
    CHECK((bounded.GetLedger() == BiomassLedger{6, 3, 2, 1, 1, 1}));

    Grid reversed{{2, 1, 1.0F}};
    Reclamation shuffled{{2, 1, 1.0F}, 2, {2, 1, 0, 1}};
    CHECK(reversed.TrySeed(0, 0));
    const std::array reverse{contenders[1], contenders[0]};
    CHECK(shuffled.TryStep(reversed, reverse));
    CHECK(std::ranges::equal(shuffled.GetStocks(), bounded.GetStocks()));
    CHECK(shuffled.GetLedger() == bounded.GetLedger());
    return true;
}

bool MappingFixtures() {
    struct Contact { float x, y; std::size_t expected; };
    // 2x2 expected row-major cells are selected independently from the mapping code.
    const std::array contacts{
        Contact{0.0F, 0.0F, 0}, Contact{-100.0F, -100.0F, 0},
        Contact{1.0F, 0.5F, 1}, Contact{0.5F, 1.0F, 2}, Contact{1.0F, 1.0F, 3},
        Contact{2.0F, 2.0F, 3}, Contact{100.0F, 100.0F, 3},
        Contact{std::nextafter(1.0F, 0.0F), 0.5F, 0},
        Contact{std::nextafter(1.0F, 2.0F), 0.5F, 1}};
    for (const auto& contact : contacts) {
        Grid grid{{2, 2, 1.0F}};
        for (std::size_t row = 0; row < 2; ++row)
            for (std::size_t column = 0; column < 2; ++column) CHECK(grid.TrySeed(column, row));
        Reclamation reclaim{{2, 2, 1.0F}, 1, {1, 1, 0, 1}};
        const std::array sample{Sample(1, contact.x, contact.y)};
        CHECK(reclaim.TryStep(grid, sample));
        for (std::size_t index = 0; index < 4; ++index) {
            CHECK(reclaim.GetStocks()[index] == (index == contact.expected ? 0 : 1));
            CHECK(grid.IsInfected(index % 2, index / 2) == (index != contact.expected));
        }
    }
    return true;
}

bool RejectionFixtures() {
    Grid grid{{2, 1, 1.0F}};
    Reclamation reclaim{{2, 1, 1.0F}, 2, {1, 1, 5, 64}};
    CHECK(grid.TrySeed(0, 0));
    const auto initial = reclaim.GetLedger();
    const std::array invalid_pairs{
        std::array{Sample(1), Sample(1)}, std::array{Sample(0), Sample(2)},
        std::array{Sample(1), Sample(3)},
        std::array{Sample(1, std::numeric_limits<float>::infinity()), Sample(2)},
        std::array{Sample(1), Sample(2, 0.5F, std::numeric_limits<float>::quiet_NaN())}};
    for (const auto& invalid : invalid_pairs) {
        CHECK(!reclaim.TryStep(grid, invalid));
        CHECK(reclaim.GetLedger() == initial);
        CHECK(grid.IsInfected(0, 0) && !grid.IsInfected(1, 0));
        CHECK((std::ranges::equal(reclaim.GetStocks(), std::array<std::uint64_t, 2>{1, 1})));
    }
    const std::array valid{Sample(1), Sample(2, 1.5F)};
    CHECK(!reclaim.TryStep(grid, std::span{valid}.first(1)));
    const std::array excess{Sample(1), Sample(2), Sample(3)};
    CHECK(!reclaim.TryStep(grid, excess));
    Grid wrong_shape{{1, 2, 1.0F}};
    Grid wrong_size{{2, 1, 2.0F}};
    CHECK(!reclaim.TryStep(wrong_shape, valid));
    CHECK(!reclaim.TryStep(wrong_size, valid));
    {
        auto lease = grid.TryPrepareStep();
        CHECK(lease.has_value());
        CHECK(!reclaim.TryStep(grid, valid));
        CHECK(reclaim.GetLedger() == initial);
    }
    CHECK(reclaim.TryStep(grid, valid));
    CHECK((reclaim.GetLedger() == BiomassLedger{9, 0, 2, 7, 2, 2}));
    return true;
}

bool StartupFixtures() {
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    const auto rejects_overflow = [](GridConfig config, std::size_t count, ResourceSettings settings) {
        try { Reclamation reclaim{config, count, settings}; }
        catch (const std::overflow_error&) { return true; }
        return false;
    };
    CHECK(rejects_overflow({2, 1, 1.0F}, 0, {maximum, 1, 0, 1}));
    CHECK(rejects_overflow({1, 1, 1.0F}, 2, {0, maximum, 0, 1}));
    CHECK(rejects_overflow({1, 1, 1.0F}, 1, {maximum, 1, 0, 1}));
    CHECK(rejects_overflow({1, 1, 1.0F}, 1, {0, 1, maximum, 1}));
    for (const auto config : {GridConfig{0, 1, 1.0F}, GridConfig{1, 1, 0.0F}}) {
        bool rejected = false;
        try { Reclamation reclaim{config, 1, {}}; }
        catch (const std::invalid_argument&) { rejected = true; }
        CHECK(rejected);
    }
    bool rejected = false;
    try { Reclamation reclaim{{1, 1, 1.0F}, 0, {0, 0, 0, 0}}; }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    Reclamation maximal{{1, 1, 1.0F}, 0, {0, 1, maximum, maximum}};
    Grid grid{{1, 1, 1.0F}};
    CHECK(grid.TrySeed(0, 0));
    CHECK(maximal.TryStep(grid, {}));
    CHECK((maximal.GetLedger() == BiomassLedger{maximum, 0, 0, maximum, 0, 0}));
    Grid boundary_grid{{1, 1, 1.0F}};
    Reclamation boundary{{1, 1, 1.0F}, 1, {1, 1, maximum - 2, 1}};
    const std::array sample{Sample(1)};
    CHECK(boundary_grid.TrySeed(0, 0));
    CHECK(boundary.TryStep(boundary_grid, sample));
    CHECK((boundary.GetLedger() == BiomassLedger{maximum, 0, 1, maximum - 1, 1, 1}));
    return true;
}
}

int main() {
    if (!AccountingFixtures() || !SpreadAndBudgetFixtures() || !MappingFixtures() ||
        !RejectionFixtures() || !StartupFixtures()) return 1;
    std::cout << "Reclamation independent accounting fixtures passed\n";
}
