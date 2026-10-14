#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "crucible/presentation/production/nanite_mesh_frame.hpp"
#include "crucible/presentation/production/nanite_model.hpp"

namespace crucible::spike::swarm { struct RenderRow; struct NaniteBody; struct Camera; }

namespace crucible::presentation::production::detail
{
/** Receives the procedural model on actual decorative source bodies.
 * Owns cold model caches and reusable frame storage; coordinator-only and not
 * thread-safe. No source borrow or native graphics resource is retained.
 */
class SwarmModelMesh
{
public:
    /// Copied storage and geometry receipts; no internal container is borrowed.
    struct Stats
    {
        std::array<std::size_t, 6> actualRungCounts_{}; ///< Complete-frame emitted units per rung; zero after refusal.
        std::size_t sliverTriangles_{}; ///< Numerical triangles removed across all cold caches.
        std::size_t cacheBytes_{}; ///< Retained cold-cache vector capacities in bytes.
        std::size_t omittedHousingTriangles_{}; ///< Complete-frame omitted housing triangles; zero after refusal.
        std::size_t visibleUnits_{}; ///< Units whose conservative envelope intersects the drawable view.
        std::size_t culledUnits_{}; ///< Source units omitted only from rendering, never from choreography.
    };

    /** Builds four variants of the consumed ladder prefix and reserves frame storage.
     * @throws std::bad_alloc Startup allocation failed.
     * @throws std::runtime_error The model or retained cache violates its contract.
     */
    SwarmModelMesh();
    /** Clears frame receipts and LOD history for replay, retaining cold storage.
     * @note Expires the previous frame borrow; coordinator-only and allocation-free.
     */
    void reset() noexcept;
    /** Produces one complete frame without allocating or rebuilding the model.
     * @param rows Stable-identity source snapshot, at most maxUnits entries.
     * @param body Actual source body dimensions; attachment uses this height.
     * @param camera Actual source camera and drawable values.
     * @param viewTilt Source oblique height/depth coefficient.
     * @param choreographySeconds Source choreography time, not presentation time.
     * @param diameter Source visibility padding in world units.
     * @return False for invalid, degenerate or unsupported input; no partial frame remains.
     * @note Every call expires the previous frame borrow, including refusal.
     */
    [[nodiscard]] bool tryBuild(std::span<const spike::swarm::RenderRow> rows,
        const spike::swarm::NaniteBody& body, const spike::swarm::Camera& camera,
        float viewTilt, float choreographySeconds, float diameter = 1.0F);
    /** Borrows the complete frame until the next build, reset or destruction.
     * @return Current geometry; callers check the preceding build receipt.
     */
    [[nodiscard]] NaniteMeshFrame getFrame() const noexcept;
    /// Reports retained frame corner capacity, separately from cold cache storage.
    [[nodiscard]] std::size_t getCapacity() const noexcept { return vertices_.capacity(); }
    /// Reports units actually emitted above the silhouette rung, at most maxDetailedUnits.
    [[nodiscard]] std::size_t getDetailedCount() const noexcept { return detailedCount_; }
    /// Copies cold-cache receipts and the latest build's complete-frame counts, zero on refusal.
    [[nodiscard]] Stats getStats() const noexcept { return stats_; }

private:
    struct CachedVertex
    {
        std::array<float, 4> weights_{};
        std::array<float, 3> normal_{};
        std::array<float, 3> interior_{};
        NaniteMaterial material_{};
    };
    struct CachedMesh
    {
        std::vector<CachedVertex> vertices_;
        std::vector<std::uint32_t> indices_;
        std::size_t emittedCorners_{}, housingTriangles_{};
    };
    struct Candidate { double distance_{}; std::size_t identity_{}; };
    static constexpr std::size_t rungCount = 6, variantCount = 4;
    std::array<std::array<CachedMesh, rungCount>, variantCount> caches_{};
    std::vector<NaniteMeshVertex> vertices_;
    std::array<Candidate, NaniteMeshFrame::maxUnits> candidates_{};
    std::array<std::size_t, NaniteMeshFrame::maxUnits> currentRungs_{}, chosenRungs_{};
    std::array<bool, NaniteMeshFrame::maxUnits> visible_{};
    CinematicCamera camera_{};
    Stats stats_{};
    std::size_t detailedCount_{};
};
}
