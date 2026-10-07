#pragma once

#include <optional>

#include <crucible/contracts/GridConfig.hpp>
#include <crucible/contracts/Position.hpp>

namespace crucible::presentation {
/// Logical render coordinates; native pixels and DPI are converted by the event adapter.
struct ScreenPoint { double x{}, y{}; };
/// Logical world viewport with half-open right and bottom input edges.
struct ScreenRect { double x{}, y{}, width{}, height{}; };

/** Aspect-preserving orthographic camera for bounded simulation inspection.
 * Owns camera values only; no renderer, frame or input borrow survives a call.
 * Coordinator-exclusive access keeps view changes separate from gameplay/replay state.
 */
class Camera2D {
public:
    /** Initializes a fitted view of a finite rectangular world.
     * @param[in] config Validated physical world; camera does not change its topology.
     * @param[in] viewport Finite logical origin and positive, representable dimensions.
     * @throws std::invalid_argument Invalid geometry, viewport edges or unrepresentable fit/zoom scale.
     */
    Camera2D(GridConfig config, ScreenRect viewport);

    /// Centers and fits the entire world, restoring relative zoom to one.
    void ResetFit() noexcept;
    /** Moves the view opposite a logical drag, constrained to world coverage.
     * @param[in] delta Finite logical drag displacement; zero succeeds without motion.
     * @return False on nonfinite input/arithmetic, preserving the previous view.
     */
    [[nodiscard]] bool TryPan(ScreenPoint delta) noexcept;
    /** Zooms around a world point under the cursor, constrained to relative zoom 1..16.
     * @param[in] anchor Logical pointer position accepted by TryToWorld.
     * @param[in] factor Positive finite scale multiplier.
     * @return False on invalid factor/anchor/arithmetic, preserving the previous view.
     * @note World-edge center constraints may move the anchored point.
     */
    [[nodiscard]] bool TryZoom(ScreenPoint anchor, double factor) noexcept;
    /** Converts a logical viewport point to physical simulation coordinates.
     * @param[in] point Finite logical position, after the adapter's pixel/DPI conversion.
     * @return nullopt outside the half-open viewport or closed world, including fit letterboxing.
     */
    [[nodiscard]] std::optional<Position> TryToWorld(ScreenPoint point) const noexcept;
    /** Projects a physical world point without viewport clipping.
     * @param[in] position Finite coordinates within the closed simulation world.
     * @return Logical point, which may lie outside the viewport at zoom; nullopt for invalid input/arithmetic.
     */
    [[nodiscard]] std::optional<ScreenPoint> TryToScreen(Position position) const noexcept;

    /// Returns the immutable logical drawing/input rectangle.
    [[nodiscard]] constexpr ScreenRect GetViewport() const noexcept { return viewport_; }
    /// Returns the immutable physical world geometry.
    [[nodiscard]] constexpr GridConfig GetConfig() const noexcept { return config_; }
    /// Returns logical render units per world unit.
    [[nodiscard]] constexpr double GetScale() const noexcept { return fit_scale_ * zoom_; }

private:
    [[nodiscard]] std::optional<ScreenPoint> TryWorldPoint(ScreenPoint point) const noexcept;
    [[nodiscard]] ScreenPoint ConstrainCenter(ScreenPoint center, double scale) const noexcept;

    GridConfig config_{};
    GridExtent extent_{};
    ScreenRect viewport_{};
    ScreenPoint center_{};
    double fit_scale_{}, zoom_{1};
};
}
