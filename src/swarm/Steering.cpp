#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

#include "crucible/contracts/timing.hpp"
#include "crucible/fields/FieldSet.hpp"
#include "crucible/spatial/Grid.hpp"
#include "crucible/swarm/Steering.hpp"

namespace crucible::swarm {
namespace {
void capMagnitude(double& x, double& y, double limit) noexcept
{
    const double magnitude = std::hypot(x, y);
    if (magnitude > limit) {
        const double scale = limit / magnitude;
        x *= scale;
        y *= scale;
    }
}

bool isInputValid(std::span<const SampleState> input, GridExtent extent) noexcept
{
    for (std::size_t row = 0; row < input.size(); ++row) {
        const auto& sample = input[row];
        if ((row != 0 && input[row - 1].id >= sample.id) ||
            !std::isfinite(sample.position.x) || !std::isfinite(sample.position.y) ||
            !std::isfinite(sample.velocity.x) || !std::isfinite(sample.velocity.y) ||
            sample.position.x < 0.0F || sample.position.x > extent.width ||
            sample.position.y < 0.0F || sample.position.y > extent.height) return false;
    }
    return true;
}

bool overlapsInput(std::span<const SampleState> input, std::span<SampleState> output) noexcept
{
    if (input.empty() || output.empty()) return false;
    const std::less<const SampleState*> before;
    return before(input.data(), output.data() + output.size()) &&
        before(output.data(), input.data() + input.size());
}

template<typename Query>
bool computeRows(std::span<const SampleState> input, const fields::FieldSet& fields,
    GridExtent extent, SteeringSettings settings, std::size_t firstRow,
    std::span<SampleState> pendingRows, const Query& query) noexcept
{
    std::size_t row = 0;
    for (const auto& sample : input.subspan(firstRow, pendingRows.size())) {
        double separation_x = 0.0, separation_y = 0.0;
        std::size_t neighbor_count = 0;
        bool found_self = false;
        // The query result is consumed completely before any subsequent grid query.
        const auto neighbors = query(sample.position, settings.neighbor_radius);
        if (!neighbors) return false;
        for (const auto id : *neighbors) {
            if (id == sample.id) { found_self = true; continue; }
            const auto neighbor = std::ranges::lower_bound(input, id, {}, &SampleState::id);
            if (neighbor == input.end() || neighbor->id != id) return false;
            const double dx = static_cast<double>(sample.position.x) - neighbor->position.x;
            const double dy = static_cast<double>(sample.position.y) - neighbor->position.y;
            const double distance = std::hypot(dx, dy);
            if (distance == 0.0) {
                separation_x += sample.id < id ? -1.0 : 1.0;
            } else {
                const double weight = std::max(0.0, 1.0 - distance / settings.neighbor_radius);
                separation_x += dx / distance * weight;
                separation_y += dy / distance * weight;
            }
            ++neighbor_count;
        }
        if (!found_self) return false;
        if (neighbor_count != 0) {
            const double scale = settings.separation_strength / static_cast<double>(neighbor_count);
            separation_x *= scale;
            separation_y *= scale;
        }
        const auto radial = fields.Sample(sample.position);
        double acceleration_x = separation_x + radial.x;
        double acceleration_y = separation_y + radial.y;
        capMagnitude(acceleration_x, acceleration_y, settings.maximum_acceleration);
        double velocity_x = sample.velocity.x + acceleration_x * tick_seconds;
        double velocity_y = sample.velocity.y + acceleration_y * tick_seconds;
        capMagnitude(velocity_x, velocity_y, settings.maximum_speed);
        pendingRows[row++] = {sample.id,
            {static_cast<float>(std::clamp(sample.position.x + velocity_x * tick_seconds,
                                          0.0, static_cast<double>(extent.width))),
             static_cast<float>(std::clamp(sample.position.y + velocity_y * tick_seconds,
                                          0.0, static_cast<double>(extent.height)))},
            {static_cast<float>(velocity_x), static_cast<float>(velocity_y)}};
    }
    return true;
}
}

Steering::Steering(GridConfig config, SteeringSettings settings, std::size_t sample_capacity)
    : m_Settings(settings)
{
    const auto extent = config.TryValidate();
    if (!extent || !std::isfinite(settings.neighbor_radius) || settings.neighbor_radius <= 0.0F ||
        !std::isfinite(settings.separation_strength) || settings.separation_strength < 0.0F ||
        !std::isfinite(settings.maximum_acceleration) || settings.maximum_acceleration <= 0.0F ||
        !std::isfinite(settings.maximum_speed) || settings.maximum_speed <= 0.0F)
        throw std::invalid_argument("Invalid swarm geometry or steering settings");
    if (sample_capacity > m_Output.max_size())
        throw std::length_error("Swarm sample capacity exceeds storage limits");
    m_Extent = *extent;
    m_Output.resize(sample_capacity);
}

bool Steering::TryCompute(std::span<const SampleState> input, const fields::FieldSet& fields,
                          spatial::Grid& grid, std::span<SampleState> output) noexcept
{
    if (input.size() > m_Output.size() || input.size() > output.size() ||
        !isInputValid(input, m_Extent)) return false;
    const auto query = [&grid](Position center, float radius) noexcept {
        return grid.TryQuery(center, radius);
    };
    if (!computeRows(input, fields, m_Extent, m_Settings, 0,
        std::span{m_Output}.first(input.size()), query)) return false;
    std::copy_n(m_Output.begin(), input.size(), output.begin());
    return true;
}

bool Steering::tryComputeRows(std::span<const SampleState> fullInput,
    const fields::FieldSet& fields, const spatial::Grid& constGrid, std::size_t firstRow,
    std::span<SampleState> pendingRows, std::span<SampleId> queryScratch) const noexcept
{
    if (fullInput.size() > m_Output.size() || firstRow > fullInput.size() ||
        pendingRows.size() > fullInput.size() - firstRow || queryScratch.size() < fullInput.size() ||
        overlapsInput(fullInput, pendingRows) || !isInputValid(fullInput, m_Extent)) return false;
    const auto query = [&constGrid, queryScratch](Position center, float radius) noexcept {
        return constGrid.tryQuery(center, radius, queryScratch);
    };
    return computeRows(fullInput, fields, m_Extent, m_Settings, firstRow, pendingRows, query);
}
}
