#include <crucible/presentation/Camera2D.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace crucible::presentation {
Camera2D::Camera2D(GridConfig config, ScreenRect viewport) : config_(config), viewport_(viewport) {
    const auto extent = config.TryValidate();
    if (!extent || !std::isfinite(viewport.x) || !std::isfinite(viewport.y) ||
        !std::isfinite(viewport.width) || !std::isfinite(viewport.height) ||
        viewport.width <= 0 || viewport.height <= 0 ||
        !std::isfinite(viewport.x + viewport.width) || !std::isfinite(viewport.y + viewport.height) ||
        viewport.x + viewport.width <= viewport.x || viewport.y + viewport.height <= viewport.y)
        throw std::invalid_argument("Invalid camera world or logical viewport");
    extent_ = *extent;
    fit_scale_ = std::min(viewport.width / extent_.width, viewport.height / extent_.height);
    if (!std::isfinite(fit_scale_) || fit_scale_ <= 0 ||
        fit_scale_ > std::numeric_limits<double>::max() / 16)
        throw std::invalid_argument("Camera scale is unrepresentable");
    ResetFit();
}

void Camera2D::ResetFit() noexcept {
    center_ = {static_cast<double>(extent_.width) / 2, static_cast<double>(extent_.height) / 2};
    zoom_ = 1;
}

ScreenPoint Camera2D::ConstrainCenter(ScreenPoint center, double scale) const noexcept {
    const auto axis = [scale](double value, double viewport_size, double world_size) {
        const double half_view = viewport_size / scale / 2;
        return half_view >= world_size / 2 ? world_size / 2 : std::clamp(value, half_view, world_size - half_view);
    };
    return {axis(center.x, viewport_.width, extent_.width), axis(center.y, viewport_.height, extent_.height)};
}

bool Camera2D::TryPan(ScreenPoint delta) noexcept {
    if (!std::isfinite(delta.x) || !std::isfinite(delta.y)) return false;
    const auto scale = GetScale();
    const ScreenPoint proposed{center_.x - delta.x / scale, center_.y - delta.y / scale};
    if (!std::isfinite(proposed.x) || !std::isfinite(proposed.y)) return false;
    center_ = ConstrainCenter(proposed, scale);
    return true;
}

std::optional<ScreenPoint> Camera2D::TryWorldPoint(ScreenPoint point) const noexcept {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
        point.x < viewport_.x || point.y < viewport_.y ||
        point.x >= viewport_.x + viewport_.width || point.y >= viewport_.y + viewport_.height) return std::nullopt;
    const auto first = TryToScreen({0, 0});
    const auto last = TryToScreen({extent_.width, extent_.height});
    if (!first || !last || point.x < first->x || point.y < first->y ||
        point.x > last->x || point.y > last->y) return std::nullopt;
    const auto scale = GetScale();
    const ScreenPoint world{
        center_.x + (point.x - viewport_.x - viewport_.width / 2) / scale,
        center_.y + (point.y - viewport_.y - viewport_.height / 2) / scale};
    if (!std::isfinite(world.x) || !std::isfinite(world.y)) return std::nullopt;
    // Screen-space containment was proved first; only inverse cancellation is corrected here.
    return ScreenPoint{std::clamp(world.x, 0.0, static_cast<double>(extent_.width)),
        std::clamp(world.y, 0.0, static_cast<double>(extent_.height))};
}

std::optional<Position> Camera2D::TryToWorld(ScreenPoint point) const noexcept {
    const auto world = TryWorldPoint(point);
    if (!world) return std::nullopt;
    return Position{static_cast<float>(world->x), static_cast<float>(world->y)};
}

std::optional<ScreenPoint> Camera2D::TryToScreen(Position position) const noexcept {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
        position.x < 0 || position.y < 0 || position.x > extent_.width || position.y > extent_.height) return std::nullopt;
    const auto scale = GetScale();
    const auto project = [this, scale](double value, double center, double world_size,
            double origin, double viewport_size) {
        if (zoom_ == 1 || viewport_size / scale / 2 >= world_size / 2) {
            // A fitted axis covers the entire world. Construct contained bounds directly:
            // centered multiply/add cancellation (including FMA) can otherwise put an
            // exact world corner just outside the viewport. lerp preserves these endpoints.
            const double projected_size = std::min(world_size * scale, viewport_size);
            const double padding = (viewport_size - projected_size) / 2;
            return std::lerp(origin + padding, (origin + viewport_size) - padding, value / world_size);
        }
        return origin + viewport_size / 2 + (value - center) * scale;
    };
    const ScreenPoint screen{
        project(position.x, center_.x, extent_.width, viewport_.x, viewport_.width),
        project(position.y, center_.y, extent_.height, viewport_.y, viewport_.height)};
    if (!std::isfinite(screen.x) || !std::isfinite(screen.y)) return std::nullopt;
    return screen;
}

bool Camera2D::TryZoom(ScreenPoint anchor, double factor) noexcept {
    if (!std::isfinite(factor) || factor <= 0) return false;
    const auto world = TryWorldPoint(anchor);
    if (!world) return false;
    const auto proposed_zoom = factor >= 16 / zoom_ ? 16 : std::max(1.0, zoom_ * factor);
    const auto scale = fit_scale_ * proposed_zoom;
    const ScreenPoint proposed{
        world->x - (anchor.x - viewport_.x - viewport_.width / 2) / scale,
        world->y - (anchor.y - viewport_.y - viewport_.height / 2) / scale};
    if (!std::isfinite(proposed.x) || !std::isfinite(proposed.y)) return false;
    center_ = ConstrainCenter(proposed, scale);
    zoom_ = proposed_zoom;
    return true;
}
}
