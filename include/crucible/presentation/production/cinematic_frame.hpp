#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>

namespace crucible::presentation::production
{
/// Decorative instance values in right-handed scene coordinates, with +Y up.
struct NanitePose
{
    std::array<float, 3> position_{};
    std::array<float, 4> orientation_{0.0F, 0.0F, 0.0F, 1.0F}; ///< Unit quaternion XYZW.
    float scale_{1.0F};
    float coreIntensity_{1.0F};
};

/// Perspective or orthographic decorative view, independent of the simulation camera.
struct CinematicCamera
{
    /// Positive orthographic half-extents; omission retains the perspective camera.
    struct OrthographicExtent { float halfWidth_{1.0F}, halfHeight_{1.0F}; };
    std::array<float, 3> eye_{0.0F, 8.0F, 24.0F};
    std::array<float, 3> target_{};
    float verticalFovRadians_{0.75F};
    std::optional<OrthographicExtent> orthographic_{};
};

/** Borrows one complete decorative scene for synchronous renderer consumption.
 * The pose borrow expires on its producer's next evaluation or destruction.
 * An asynchronous graphics upload must copy into provider-owned storage before
 * the consuming call returns. No Runtime or ECS state enters this packet.
 * @note Coordinator-only; numerical bounds are frozen in the Phase15 contract.
 */
struct CinematicFrame
{
    /// Maximum startup-bounded decorative instances in one complete packet.
    static constexpr std::size_t maxInstances = 4096;
    std::span<const NanitePose> poses_{}; ///< Non-owning, synchronous scene borrow.
    CinematicCamera camera_{};
    float keyIntensity_{4.0F};
    float glowIntensity_{1.0F};
};
}
