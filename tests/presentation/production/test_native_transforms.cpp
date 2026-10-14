#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "crucible/presentation/production/detail/native_transforms.hpp"

int main()
{
    using namespace crucible::presentation::production;
    try
    {
        NanitePose pose;
        for (float scale : {0.43F, 1.0F, 16.0F})
        {
            pose.scale_ = scale;
            if (!detail::tryBuildTransform(pose)) throw std::runtime_error("Ordinary scale must retain finite model and normals");
        }
        for (float scale : {1e-10F, 1e-30F, std::numeric_limits<float>::denorm_min()})
        {
            pose.scale_ = scale;
            if (detail::tryBuildTransform(pose)) throw std::runtime_error("Finite tiny model must reject the underflowed normal path");
        }
        pose = {};
        pose.position_ = {-512, 512, 512};
        pose.orientation_ = {0.70710677F, 0, 0, 0.70710677F};
        if (!detail::tryBuildTransform(pose)) throw std::runtime_error("Finite boundary translation and rotation must remain accepted");
        for (const auto normal : {std::array<float, 3>{1, 0, 0}, {0, 1, 0}, {0, 0, 1},
            {-1, 0, 0}, {0, -1, 0}, {0, 0, -1}, {0.8F, 0.6F, 0}})
        {
            const auto quaternion = detail::buildTangent(normal);
            const filament::math::mat3f basis{quaternion};
            for (std::size_t axis = 0; axis < 3; ++axis)
                if (!std::isfinite(basis[2][axis]) || std::abs(basis[2][axis] - normal[axis]) > 1e-4F)
                    throw std::runtime_error("Packed tangent quaternion changed the outward normal");
            if (std::abs(dot(quaternion, quaternion) - 1) > 1e-4F)
                throw std::runtime_error("Packed tangent quaternion is not unit length");
        }
        std::cout << "Pinned model/normal transform preflight PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
