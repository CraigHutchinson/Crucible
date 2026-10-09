#pragma once

#include <cmath>

#include "crucible/presentation/production/cinematic_frame.hpp"

namespace crucible::presentation::production::detail
{
/// Checks the complete shared packet before any native frame/resource mutation.
[[nodiscard]] inline bool isValidFrame(const CinematicFrame& frame) noexcept
{
    const auto isPosition = [](const auto& values)
    {
        for (float value : values)
        {
            if (!std::isfinite(value) || std::abs(value) > 512.0F)
            {
                return false;
            }
        }
        return true;
    };
    if (frame.poses_.size() > CinematicFrame::maxInstances || !isPosition(frame.camera_.eye_)
        || !isPosition(frame.camera_.target_) || !std::isfinite(frame.camera_.verticalFovRadians_)
        || frame.camera_.verticalFovRadians_ <= 0.1F || frame.camera_.verticalFovRadians_ >= 2.8F
        || !std::isfinite(frame.keyIntensity_) || frame.keyIntensity_ < 0 || frame.keyIntensity_ > 8
        || !std::isfinite(frame.glowIntensity_) || frame.glowIntensity_ < 0 || frame.glowIntensity_ > 4)
    {
        return false;
    }
    double lengthSquared{}, horizontalSquared{};
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        const double delta = static_cast<double>(frame.camera_.eye_[axis]) - frame.camera_.target_[axis];
        lengthSquared += delta * delta;
        if (axis != 1)
        {
            horizontalSquared += delta * delta;
        }
    }
    if (lengthSquared < 0.01 || horizontalSquared / lengthSquared < 0.0001)
    {
        return false;
    }
    for (const auto& pose : frame.poses_)
    {
        if (!isPosition(pose.position_) || !std::isfinite(pose.scale_) || pose.scale_ <= 0 || pose.scale_ > 16
            || !std::isfinite(pose.coreIntensity_) || pose.coreIntensity_ < 0 || pose.coreIntensity_ > 4)
        {
            return false;
        }
        double squared{};
        for (float value : pose.orientation_)
        {
            if (!std::isfinite(value))
            {
                return false;
            }
            squared += static_cast<double>(value) * value;
        }
        if (std::abs(squared - 1.0) > 0.002)
        {
            return false;
        }
    }
    return true;
}
}
