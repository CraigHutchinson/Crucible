#pragma once
#include <crucible/contracts/StateCopy.hpp>
#include <optional>
#include <vector>

namespace crucible { class Simulation; }
namespace crucible::presentation {
/// Owned scenario frame, allocated once at startup. Capture requires coordinator exclusivity.
/// Observers borrow this object's storage until its next successful capture or destruction.
class ScenarioSnapshot {
public:
    ScenarioSnapshot(std::size_t samples, std::size_t fields, std::size_t cells);
    ScenarioSnapshot(const ScenarioSnapshot&) = delete;
    ScenarioSnapshot& operator=(const ScenarioSnapshot&) = delete;
    ScenarioSnapshot(ScenarioSnapshot&&) = delete;
    ScenarioSnapshot& operator=(ScenarioSnapshot&&) = delete;
    /// Rejection preserves the previous frame and metadata; no allocation during capture.
    [[nodiscard]] bool TryCapture(Simulation& simulation) noexcept;
    /// Exact owned-state replay comparison, including activity and structural state.
    [[nodiscard]] bool HasEqualState(const ScenarioSnapshot& other) const noexcept;
    [[nodiscard]] std::optional<ScenarioStateInfo> GetInfo() const noexcept { return m_Info; }
    [[nodiscard]] std::span<const SampleState> GetSamples() const noexcept;
    [[nodiscard]] std::span<const FieldEdit> GetFields() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> GetBlight() const noexcept;
    /** Borrows row-major substrate quantities from the retained frame.
     * @return Every cell's stock for resource-enabled frames, otherwise an empty span.
     * @note Requires coordinator exclusivity; the borrow expires on successful capture or destruction.
     */
    [[nodiscard]] std::span<const std::uint64_t> GetStocks() const noexcept;
private:
    std::vector<SampleState> m_Samples;
    std::vector<FieldEdit> m_Fields;
    std::vector<std::uint8_t> m_Blight;
    std::vector<std::uint64_t> m_Stocks;
    std::optional<ScenarioStateInfo> m_Info;
};
}
