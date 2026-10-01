#include <crucible/swarm/Steering.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <crucible/contracts/Tick.hpp>
#include <crucible/fields/FieldSet.hpp>
#include <crucible/spatial/Grid.hpp>

namespace crucible::swarm {
namespace {
void CapMagnitude(double& x, double& y, double limit) noexcept {
    const double magnitude = std::hypot(x, y);
    if (magnitude > limit) {
        const double scale = limit / magnitude;
        x *= scale;
        y *= scale;
    }
}
}

Steering::Steering(GridConfig config, SteeringSettings settings, std::size_t sample_capacity)
    : m_Settings(settings) {
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
                          spatial::Grid& grid, std::span<SampleState> output) noexcept {
    if (input.size() > m_Output.size() || input.size() > output.size()) return false;
    for (std::size_t row = 0; row < input.size(); ++row) {
        const auto& sample = input[row];
        if ((row != 0 && input[row - 1].id >= sample.id) ||
            !std::isfinite(sample.position.x) || !std::isfinite(sample.position.y) ||
            !std::isfinite(sample.velocity.x) || !std::isfinite(sample.velocity.y) ||
            sample.position.x < 0.0F || sample.position.x > m_Extent.width ||
            sample.position.y < 0.0F || sample.position.y > m_Extent.height) return false;
    }
    std::size_t row = 0;
    for (const auto& sample : input) {
        double separation_x = 0.0, separation_y = 0.0;
        std::size_t neighbor_count = 0;
        bool found_self = false;
        // The query result is consumed completely before any subsequent grid query.
        const auto neighbors = grid.TryQuery(sample.position, m_Settings.neighbor_radius);
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
                const double weight = std::max(0.0, 1.0 - distance / m_Settings.neighbor_radius);
                separation_x += dx / distance * weight;
                separation_y += dy / distance * weight;
            }
            ++neighbor_count;
        }
        if (!found_self) return false;
        if (neighbor_count != 0) {
            const double scale = m_Settings.separation_strength / static_cast<double>(neighbor_count);
            separation_x *= scale;
            separation_y *= scale;
        }
        const auto radial = fields.Sample(sample.position);
        double acceleration_x = separation_x + radial.x;
        double acceleration_y = separation_y + radial.y;
        CapMagnitude(acceleration_x, acceleration_y, m_Settings.maximum_acceleration);
        double velocity_x = sample.velocity.x + acceleration_x * tick_seconds;
        double velocity_y = sample.velocity.y + acceleration_y * tick_seconds;
        CapMagnitude(velocity_x, velocity_y, m_Settings.maximum_speed);
        m_Output[row++] = {sample.id,
            {static_cast<float>(std::clamp(sample.position.x + velocity_x * tick_seconds,
                                          0.0, static_cast<double>(m_Extent.width))),
             static_cast<float>(std::clamp(sample.position.y + velocity_y * tick_seconds,
                                          0.0, static_cast<double>(m_Extent.height)))},
            {static_cast<float>(velocity_x), static_cast<float>(velocity_y)}};
    }
    std::copy_n(m_Output.begin(), input.size(), output.begin());
    return true;
}
}
