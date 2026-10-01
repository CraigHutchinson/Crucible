#include <crucible/simulation.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

int main() {
    using namespace crucible;
    Simulation legacy{1};
    if (legacy.GetFieldCapacity() != 0 || legacy.GetBlightInfectedCount() != 0 ||
        legacy.GetOccupiedCellCount() != 0 || legacy.TryCountNeighbors({}, 1) ||
        legacy.TryApplyFieldEdit({FieldEditKind::remove, 0})) return 1;

    Simulation scenario{1, {{3, 3, 1}, 1}};
    if (scenario.checksum() != 1 || scenario.GetBlightInfectedCount() != 1 ||
        scenario.GetOccupiedCellCount() != 1 || scenario.TryCountNeighbors({0.5F, 0.5F}, 0) != 1) return 2;
    if (scenario.TryApplyFieldEdit({FieldEditKind::set, 1, {}, 1, 1}) ||
        scenario.TryApplyFieldEdit({FieldEditKind::set, 0, {}, -1, 1})) return 3;
    if (!scenario.TryApplyFieldEdit({FieldEditKind::set, 0, {2.5F, 0.5F}, 4, 8})) return 4;
    scenario.tick();
    // Center distance is 2, so acceleration is +4 on x. Semi-implicit integration
    // uses v=(1+4*dt, .5), starting at (.5,.5).
    const double dt = 1.0 / 60.0;
    if (std::abs(scenario.checksum() - (1 + (1.5 + 4 * dt) * dt)) > 0.000001 ||
        scenario.GetBlightInfectedCount() != 5) return 5;
    if (!scenario.TryApplyFieldEdit({FieldEditKind::remove, 0})) return 6;
    scenario.tick();
    if (scenario.GetBlightInfectedCount() != 9 || scenario.GetFieldCapacity() != 1) return 7;
    if (scenario.TryCountNeighbors({}, -1)) return 8;

    Simulation empty{0, {{1, 1, 1}, 0}};
    empty.tick();
    if (empty.checksum() != 0 || empty.GetOccupiedCellCount() != 0 ||
        empty.GetBlightInfectedCount() != 1 || empty.TryCountNeighbors({}, 100) != 0) return 9;

    Simulation clamped{1, {{1, 1, 0.1F}, 1}};
    for (int tick = 0; tick < 60; ++tick) clamped.tick();
    if (std::abs(clamped.checksum() - 0.2) > 0.000001) return 10;
    const float maximum = std::numeric_limits<float>::max();
    if (!clamped.TryApplyFieldEdit({FieldEditKind::set, 0, {maximum / 2, 0}, maximum, maximum})) return 11;
    for (int tick = 0; tick < 60; ++tick) clamped.tick();
    if (!std::isfinite(clamped.checksum())) return 12;

    Simulation large_world{1, {{1, 1, maximum}, 1}};
    if (!large_world.TryApplyFieldEdit({FieldEditKind::set, 0, {maximum, maximum}, maximum, maximum}))
        return 15;
    large_world.tick();
    if (!std::isfinite(large_world.checksum()) || large_world.checksum() <= maximum) return 16;

    bool invalid = false;
    try { Simulation bad{0, {{0, 1, 1}, 0}}; } catch (const std::invalid_argument&) { invalid = true; }
    if (!invalid) return 13;
    bool too_many = false;
    try { Simulation bad{std::numeric_limits<std::size_t>::max()}; }
    catch (const std::length_error&) { too_many = true; }
    return too_many ? 0 : 14;
}
