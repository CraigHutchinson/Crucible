#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

#include "crucible/presentation/production/nanite_mesh_frame.hpp"

namespace crucible::spike::swarm { struct RenderRow; struct NaniteBody; struct Camera; }

namespace crucible::presentation::production::detail
{
/** Receives the exact source proxy geometry with reusable bounded storage.
 * @note Feature-private coordinator producer; no source/snapshot borrow survives
 * tryBuild. It does not simulate units or own a native graphics resource.
 */
class NaniteProxyMesh
{
public:
    /// Reserves the complete balanced vertex bound once at startup.
    NaniteProxyMesh();
    /** Builds the current complete batch from actual source rows and view values.
     * @param rows Borrowed stable-identity source snapshot, at most4096units.
     * @param body Actual prototype body dimensions.
     * @param camera Actual prototype camera and drawable values.
     * @param viewTilt Source oblique view's height/depth coefficient.
     * @param choreographySeconds Actual source time, never presentation seconds.
     * @param diameter Source unit diameter used for detail/culling selection.
     * @return False for unsupported, over-capacity or degenerate input.
     * @note The previous mesh borrow expires on every call, including refusal.
     */
    [[nodiscard]] bool tryBuild(std::span<const spike::swarm::RenderRow> rows,
        const spike::swarm::NaniteBody& body, const spike::swarm::Camera& camera,
        float viewTilt, float choreographySeconds, float diameter = 1.0F);
    /** Borrows the produced mesh until the next build/destruction.
     * @return Current complete mesh; callers check the preceding build receipt.
     */
    [[nodiscard]] NaniteMeshFrame getFrame() const noexcept;
    /** Reports the bounded selected detail count.
     * @return At most128 detailed bodies from the latest successful build.
     */
    [[nodiscard]] std::size_t getDetailedCount() const noexcept { return detailedCount_; }
    /** Reports actually retained vertex capacity for storage receiving.
     * @return The allocator's retained corner capacity, distinct from GPU residency.
     */
    [[nodiscard]] std::size_t getCapacity() const noexcept { return vertices_.capacity(); }
private:
    struct Candidate { double distance_{}; std::size_t identity_{}; };
    std::vector<NaniteMeshVertex> vertices_;
    std::array<Candidate, NaniteMeshFrame::maxUnits> candidates_{};
    std::array<bool, NaniteMeshFrame::maxUnits> detailed_{};
    CinematicCamera camera_{};
    std::size_t detailedCount_{};
};
}
