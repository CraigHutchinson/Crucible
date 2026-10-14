#pragma once

#include <cmath>
#include <cstdint>
#include <initializer_list>

#include "crucible/presentation/production/detail/frame_validation.hpp"
#include "crucible/presentation/production/nanite_mesh_frame.hpp"

namespace crucible::presentation::production::detail
{
/** Validates all mesh corners and triangle geometry before native staging changes.
 * @param frame Synchronously borrowed complete triangle-list frame.
 * @return True for bounded, finite, outward-facing nondegenerate geometry.
 */
[[nodiscard]] inline bool isValidFrame(const NaniteMeshFrame& frame) noexcept
{
    if (frame.vertices_.size() > NaniteMeshFrame::maxVertices || frame.vertices_.size() % 3 != 0
        || !isValidFrame(CinematicFrame{{}, frame.camera_, frame.keyIntensity_, frame.glowIntensity_})) return false;
    for (const auto& vertex : frame.vertices_)
    {
        if (static_cast<std::uint32_t>(vertex.role_) > static_cast<std::uint32_t>(NaniteMaterialRole::core)
            || !std::isfinite(vertex.intensity_) || vertex.intensity_ < 0 || vertex.intensity_ > 4) return false;
        double norm{};
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            if (!std::isfinite(vertex.position_[axis]) || std::abs(vertex.position_[axis]) > 512
                || !std::isfinite(vertex.normal_[axis])) return false;
            norm += static_cast<double>(vertex.normal_[axis]) * vertex.normal_[axis];
        }
        if (std::abs(norm - 1) > 0.002) return false;
    }
    for (std::size_t index = 0; index < frame.vertices_.size(); index += 3)
    {
        const auto& a = frame.vertices_[index];
        const auto& b = frame.vertices_[index + 1];
        const auto& c = frame.vertices_[index + 2];
        if (a.role_ != b.role_ || a.role_ != c.role_ || a.intensity_ != b.intensity_ || a.intensity_ != c.intensity_) return false;
        double ab[3]{}, ac[3]{};
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            ab[axis] = static_cast<double>(b.position_[axis]) - a.position_[axis];
            ac[axis] = static_cast<double>(c.position_[axis]) - a.position_[axis];
        }
        const double cross[3]{ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0]};
        const double squared = cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2];
        if (squared < 1e-16) return false;
        const double length = std::sqrt(squared);
        for (const auto* vertex : {&a, &b, &c})
        {
            const double dot = cross[0] * vertex->normal_[0] + cross[1] * vertex->normal_[1] + cross[2] * vertex->normal_[2];
            if (dot / length < 0.995) return false;
        }
    }
    return true;
}
}
