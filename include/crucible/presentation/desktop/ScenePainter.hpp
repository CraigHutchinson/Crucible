#pragma once

#include <cstddef>
#include <vector>

struct SDL_Renderer;
struct SDL_Vertex;
namespace crucible::presentation {
class Camera2D;
class ScenarioSnapshot;
namespace desktop {
struct SceneUi;

/** Concrete logical-canvas painter with startup-owned batch scratch.
 * Coordinator-exclusive access; no renderer, frame, camera or UI borrow survives a call.
 * Host owns SDL startup, logical presentation, present and renderer destruction.
 */
class ScenePainter {
public:
    /// Exact preserves individual markers; densityOverview aggregates only at distant zoom.
    enum class ViewPolicy { exact, densityOverview };
    /// Copied counts from the last successful draw; individuals + aggregated + hidden equals mobile.
    struct DrawStatistics {
        std::size_t authoritativeMobile{}, individualSamples{}, aggregatedSamples{}, hiddenSamples{};
        std::size_t aggregateMarks{}, visibleCells{};
    };
    /** Allocates reusable cell/sample quad storage.
     * @param[in] sample_capacity Maximum samples in a captured frame.
     * @param[in] cell_capacity Maximum row-major cells in a captured frame.
     * @param policy Startup view policy; never changes simulation participation.
     * @throws std::length_error Geometry counts cannot fit SDL int counts or vector storage.
     * @throws std::bad_alloc Startup storage cannot be allocated.
     */
    ScenePainter(std::size_t sample_capacity, std::size_t cell_capacity,
                 ViewPolicy policy = ViewPolicy::exact);
    ~ScenePainter();
    ScenePainter(const ScenePainter&) = delete;
    ScenePainter& operator=(const ScenePainter&) = delete;
    ScenePainter(ScenePainter&&) = delete;
    ScenePainter& operator=(ScenePainter&&) = delete;

    /** Draws a retained world and HUD on the shared logical canvas.
     * @param[in,out] renderer Native or software SDL renderer on its owning main thread.
     * @param[in] frame Successfully captured owned scenario state.
     * @param[in] camera Same physical geometry, viewport {24,96,1232,520}.
     * @param[in] ui Display-only feedback; message storage is borrowed during this call.
     * @return False with SDL error on malformed frame, capacity, camera or drawing failure.
     * @note Frame/capacity/camera rejection precedes drawing. SDL failure may leave a partial
     * canvas; host must discard it. Leaves viewport/clip reset and render scale one on success.
     * Painter scratch is reused; SDL's own command/glyph storage may allocate.
     */
    [[nodiscard]] bool TryDraw(SDL_Renderer& renderer, const ScenarioSnapshot& frame,
                               const Camera2D& camera, const SceneUi& ui) noexcept;
    /** Returns counts from the last successful complete drawing call.
     * @return Owned counters; a failed draw preserves the previous counters.
     */
    [[nodiscard]] constexpr DrawStatistics getDrawStatistics() const noexcept { return statistics_; }
private:
    struct DensityBin { std::size_t count{}; double x{}, y{}; };
    std::size_t sample_capacity_{}, cell_capacity_{};
    ViewPolicy policy_{};
    DrawStatistics statistics_{};
    std::vector<SDL_Vertex> vertices_;
    std::vector<int> indices_;
    std::vector<DensityBin> densityBins_;
};
}
}
