#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "crucible/presentation/production/cinematic_frame.hpp"

namespace crucible::presentation::production
{
/// Immutable product-local material roles; callers never borrow native materials.
enum class NaniteMaterialRole : std::uint32_t { shield, rail, hub, core };

/** One triangle-list corner in right-handed y-up scene coordinates.
 * @note Normals are outward unit vectors. A triangle shares one role/intensity;
 * intensity multiplies that role's authored PBR base/emission hierarchy.
 */
struct NaniteMeshVertex
{
    std::array<float, 3> position_{};
    std::array<float, 3> normal_{0, 1, 0};
    NaniteMaterialRole role_{NaniteMaterialRole::shield};
    float intensity_{1.0F};
};
static_assert(sizeof(NaniteMeshVertex) == 32);
static_assert(std::is_trivially_copyable_v<NaniteMeshVertex>);

/** Borrows a complete authored nanite proxy mesh for synchronous consumption.
 * The borrow expires on the producer's next evaluation or destruction. Native
 * upload storage is copied before return and remains owned until its callback.
 * @note Coordinator-only. No prototype ECS or mission Runtime borrow is retained.
 */
struct NaniteMeshFrame
{
    /// Global balanced mesh limits; detail allocation never multiplies these caps.
    static constexpr std::size_t maxUnits = 4096, maxDetailedUnits = 128;
    /// 84 corners per basic body plus144 additional corners per detailed body.
    static constexpr std::size_t maxVertices = maxUnits * 84 + maxDetailedUnits * 144;
    std::span<const NaniteMeshVertex> vertices_{}; ///< Non-owning triangle-list borrow.
    CinematicCamera camera_{};
    float keyIntensity_{4.0F}, glowIntensity_{1.0F};
};
}
