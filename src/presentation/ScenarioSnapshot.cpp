#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
namespace crucible::presentation {
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
