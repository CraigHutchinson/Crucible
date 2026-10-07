#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <algorithm>
namespace crucible::presentation {
bool ScenarioSnapshot::HasEqualState(const ScenarioSnapshot& other) const noexcept {
    const auto a = GetInfo(), b = other.GetInfo();
    if (!a || !b || a->completed_tick != b->completed_tick || a->biomass != b->biomass ||
        a->structural.has_value() != b->structural.has_value() || a->samples != b->samples || a->fields != b->fields ||
        a->cells != b->cells || a->grid.columns != b->grid.columns || a->grid.rows != b->grid.rows ||
        a->grid.cell_size != b->grid.cell_size) return false;
    if (a->structural) {
        const auto& x = *a->structural;
        const auto& y = *b->structural;
        if (x.occupied != y.occupied || x.generation != y.generation || x.hold_ticks != y.hold_ticks ||
            x.eligible_mobile != y.eligible_mobile || x.members != y.members ||
            x.settings.relay_center.x != y.settings.relay_center.x || x.settings.relay_center.y != y.settings.relay_center.y ||
            x.settings.eligibility_radius != y.settings.eligibility_radius ||
            x.settings.protection_radius != y.settings.protection_radius || x.settings.hold_ticks != y.settings.hold_ticks)
            return false;
    }
    return std::ranges::equal(GetSamples(), other.GetSamples(), [](const auto& x, const auto& y) {
        return x.id == y.id && x.position.x == y.position.x && x.position.y == y.position.y &&
            x.velocity.x == y.velocity.x && x.velocity.y == y.velocity.y && x.activity == y.activity;
    }) && std::ranges::equal(GetFields(), other.GetFields(), [](const auto& x, const auto& y) {
        return x.kind == y.kind && x.slot == y.slot && x.center.x == y.center.x && x.center.y == y.center.y &&
            x.radius == y.radius && x.strength == y.strength && x.end.x == y.end.x && x.end.y == y.end.y;
    }) && std::ranges::equal(GetBlight(), other.GetBlight()) &&
        std::ranges::equal(GetStocks(), other.GetStocks());
}
ScenarioSnapshot::ScenarioSnapshot(std::size_t samples, std::size_t fields, std::size_t cells)
    : m_Samples(samples), m_Fields(fields), m_Blight(cells), m_Stocks(cells) {}
bool ScenarioSnapshot::TryCapture(Simulation& simulation) noexcept {
    const auto info = simulation.TryCopyState({m_Samples, m_Fields, m_Blight, m_Stocks});
    if (!info) return false;
    m_Info = info;
    return true;
}
std::span<const SampleState> ScenarioSnapshot::GetSamples() const noexcept {
    return std::span<const SampleState>{m_Samples}.first(m_Info ? m_Info->samples : 0);
}
std::span<const FieldEdit> ScenarioSnapshot::GetFields() const noexcept {
    return std::span<const FieldEdit>{m_Fields}.first(m_Info ? m_Info->fields : 0);
}
std::span<const std::uint8_t> ScenarioSnapshot::GetBlight() const noexcept {
    return std::span<const std::uint8_t>{m_Blight}.first(m_Info ? m_Info->cells : 0);
}
std::span<const std::uint64_t> ScenarioSnapshot::GetStocks() const noexcept {
    return std::span<const std::uint64_t>{m_Stocks}.first(m_Info && m_Info->biomass ? m_Info->cells : 0);
}
}
