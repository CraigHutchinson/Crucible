#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <math/mat3.h>
#include <math/mat4.h>
#include <math/quat.h>
#include <optional>

#include "crucible/presentation/production/cinematic_frame.hpp"

namespace crucible::presentation::production::detail
{
/** Builds only transforms whose pinned instance-normal path stays finite.
 * @param pose Scene pose already accepted by packet validation.
 * @return Model matrix or refusal, without clamping/renormalizing the pose.
 * @note Mirrors FInstanceBuffer::prepare's cofactor/prescale derivation from the
 * received release. Finite model entries alone do not prove valid normal math.
 */
[[nodiscard]] inline std::optional<filament::math::mat4f> tryBuildTransform(const NanitePose& pose) noexcept
{
    using namespace filament::math;
    const auto& q = pose.orientation_;
    const auto model = mat4f::translation(float3{pose.position_[0], pose.position_[1], pose.position_[2]})
        * mat4f{quatf{q[3], q[0], q[1], q[2]}} * mat4f::scaling(float3{pose.scale_});
    const auto isFinite = [](const auto& matrix, std::size_t width)
    {
        for (std::size_t column = 0; column < width; ++column)
        {
            for (std::size_t row = 0; row < width; ++row)
            {
                if (!std::isfinite(matrix[column][row])) return false;
            }
        }
        return true;
    };
    if (!isFinite(model, 4)) return std::nullopt;
    const auto cofactor = mat3f::getTransformForNormals(model.upperLeft());
    const float largest = std::max({length2(cofactor[0]), length2(cofactor[1]), length2(cofactor[2])});
    // Avoid dividing by zero before evaluating the actual pinned prescale path.
    // The received backend uses /fp:fast. Do not depend on subnormal squared
    // lengths surviving the backend's arithmetic mode.
    if (!std::isfinite(largest) || largest < std::numeric_limits<float>::min()
        || !isFinite(cofactor, 3)) return std::nullopt;
    if (!isFinite(prescaleForNormals(cofactor), 3)) return std::nullopt;
    return model;
}
}
