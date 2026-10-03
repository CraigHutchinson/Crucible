#include <crucible/presentation/Camera2D.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using crucible::GridConfig;
using crucible::Position;
using crucible::presentation::Camera2D;
using crucible::presentation::ScreenPoint;
using crucible::presentation::ScreenRect;

#define CHECK(expression) do { if (!(expression)) { std::cerr << #expression << '\n'; return false; } } while (false)

bool Near(double left, double right) {
    return std::abs(left - right) <= 1e-6 * std::max({1.0, std::abs(left), std::abs(right)});
}
bool ScreenIs(const Camera2D& camera, Position world, ScreenPoint expected) {
    const auto actual = camera.TryToScreen(world);
    return actual && Near(actual->x, expected.x) && Near(actual->y, expected.y);
}
bool WorldIs(const Camera2D& camera, ScreenPoint screen, Position expected) {
    const auto actual = camera.TryToWorld(screen);
    return actual && Near(actual->x, expected.x) && Near(actual->y, expected.y);
}

bool AffineFixtures() {
    Camera2D camera{{8, 4, 1}, {0, 0, 400, 400}};
    // Hand-derived affine fixture: world 8x4 at 50 logical units/world unit.
    CHECK(Near(camera.GetScale(), 50));
    CHECK(ScreenIs(camera, {0, 0}, {0, 100}));
    CHECK(ScreenIs(camera, {4, 2}, {200, 200}));
    CHECK(ScreenIs(camera, {8, 4}, {400, 300}));
    CHECK(WorldIs(camera, {0, 100}, {0, 0}));
    CHECK(WorldIs(camera, {200, 200}, {4, 2}));
    CHECK(!camera.TryToWorld({200, 50}));
    CHECK(!camera.TryToWorld({200, 350}));
    CHECK(!camera.TryToWorld({400, 200}));
    CHECK(!camera.TryToWorld({200, 400}));
    CHECK(camera.TryZoom({100, 200}, 2));
    CHECK(Near(camera.GetScale(), 100));
    CHECK(WorldIs(camera, {100, 200}, {2, 2}));
    CHECK(ScreenIs(camera, {3, 2}, {200, 200}));
    CHECK(camera.TryPan({100, 0}));
    CHECK(ScreenIs(camera, {2, 2}, {200, 200}));
    CHECK(WorldIs(camera, {100, 200}, {1, 2}));
    // Further drag reaches the left constraint, rather than panning off-world.
    CHECK(camera.TryPan({10000, 10000}));
    CHECK(ScreenIs(camera, {2, 2}, {200, 200}));
    CHECK(camera.TryPan({-10000, -10000}));
    CHECK(ScreenIs(camera, {6, 2}, {200, 200}));
    camera.ResetFit();
    CHECK(Near(camera.GetScale(), 50));
    CHECK(ScreenIs(camera, {4, 2}, {200, 200}));
    return true;
}

bool EdgeZoomFixtures() {
    Camera2D camera{{8, 4, 1}, {0, 0, 400, 400}};
    CHECK(camera.TryZoom({0, 100}, 2));
    // Top-left zoom's anchor yields to center constraints: the world corner stays on the edge.
    CHECK(ScreenIs(camera, {0, 0}, {0, 0}));
    CHECK(ScreenIs(camera, {2, 2}, {200, 200}));
    CHECK(camera.TryZoom({200, 200}, std::numeric_limits<double>::max()));
    CHECK(Near(camera.GetScale(), 800));
    CHECK(camera.TryZoom({200, 200}, 2));
    CHECK(Near(camera.GetScale(), 800));
    CHECK(camera.TryZoom({200, 200}, std::numeric_limits<double>::denorm_min()));
    CHECK(Near(camera.GetScale(), 50));
    CHECK(ScreenIs(camera, {4, 2}, {200, 200}));
    // Projection intentionally remains unclipped for painter-side viewport clipping.
    CHECK(camera.TryZoom({200, 200}, 16));
    const auto outside = camera.TryToScreen({0, 0});
    CHECK(outside && outside->x < 0 && outside->y < 0);
    return true;
}

bool OffsetAndRoundTripFixtures() {
    Camera2D camera{{64, 32, 1}, {24, 96, 1232, 520}};
    CHECK(Near(camera.GetScale(), 16.25));
    CHECK(ScreenIs(camera, {32, 16}, {640, 356}));
    CHECK(ScreenIs(camera, {0, 0}, {120, 96}));
    CHECK(ScreenIs(camera, {64, 32}, {1160, 616}));
    CHECK(!camera.TryToWorld({24, 356}));
    CHECK(!camera.TryToWorld({640, 95}));
    CHECK(!camera.TryToWorld({640, 616}));
    for (const Position point : std::array{Position{0, 0}, Position{12.5F, 3.25F}, Position{32, 16}, Position{64, 31}}) {
        const auto projected = camera.TryToScreen(point);
        CHECK(projected.has_value());
        CHECK(WorldIs(camera, *projected, point));
    }
    CHECK(camera.TryZoom({640, 356}, 4));
    CHECK(camera.TryPan({24, -12}));
    const auto projected = camera.TryToScreen({32, 16});
    CHECK(projected && WorldIs(camera, *projected, {32, 16}));
    CHECK(camera.GetConfig().columns == 64 && camera.GetConfig().rows == 32 && camera.GetConfig().cell_size == 1);
    CHECK(camera.GetViewport().x == 24 && camera.GetViewport().y == 96 &&
        camera.GetViewport().width == 1232 && camera.GetViewport().height == 520);
    Camera2D nonbinary{{7, 5, 1}, {0, 0, 400, 400}};
    const auto edge = nonbinary.TryToScreen({0, 0});
    CHECK(edge && WorldIs(nonbinary, *edge, {0, 0}));
    CHECK(!nonbinary.TryToWorld({std::nextafter(edge->x, -std::numeric_limits<double>::infinity()), edge->y}));
    return true;
}

bool RejectionFixtures() {
    Camera2D camera{{8, 4, 1}, {0, 0, 400, 400}};
    CHECK(camera.TryZoom({100, 200}, 2));
    const auto unchanged = [&camera] { return Near(camera.GetScale(), 100) && ScreenIs(camera, {3, 2}, {200, 200}); };
    const auto infinity = std::numeric_limits<double>::infinity();
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    for (const ScreenPoint point : std::array{ScreenPoint{nan, 0}, ScreenPoint{0, infinity}}) {
        CHECK(!camera.TryPan(point));
        CHECK(unchanged());
        CHECK(!camera.TryZoom(point, 2));
        CHECK(unchanged());
        CHECK(!camera.TryToWorld(point));
    }
    for (const double factor : {0.0, -1.0, infinity, nan}) {
        CHECK(!camera.TryZoom({200, 200}, factor));
        CHECK(unchanged());
    }
    for (const ScreenPoint point : std::array{ScreenPoint{-1, 200}, ScreenPoint{400, 200}, ScreenPoint{200, 400}}) {
        CHECK(!camera.TryZoom(point, 2));
        CHECK(unchanged());
    }
    for (const Position point : std::array{Position{-1, 0}, Position{9, 0}, Position{0, 5},
            Position{std::numeric_limits<float>::infinity(), 0}, Position{0, std::numeric_limits<float>::quiet_NaN()}})
        CHECK(!camera.TryToScreen(point));
    Camera2D arithmetic{{1, 1, std::numeric_limits<float>::max()}, {0, 0, 400, 400}};
    const auto scale = arithmetic.GetScale();
    CHECK(!arithmetic.TryPan({std::numeric_limits<double>::max(), 0}));
    CHECK(arithmetic.GetScale() == scale);
    CHECK(ScreenIs(arithmetic, {0, 0}, {0, 0}));
    return true;
}

bool StartupFixtures() {
    const auto rejects = [](GridConfig config, ScreenRect viewport) {
        try { Camera2D camera{config, viewport}; }
        catch (const std::invalid_argument&) { return true; }
        return false;
    };
    CHECK(rejects({0, 1, 1}, {0, 0, 400, 400}));
    CHECK(rejects({1, 1, 0}, {0, 0, 400, 400}));
    for (const ScreenRect rect : std::array{ScreenRect{0, 0, 0, 400}, ScreenRect{0, 0, 400, -1},
            ScreenRect{std::numeric_limits<double>::infinity(), 0, 400, 400},
            ScreenRect{0, 0, std::numeric_limits<double>::quiet_NaN(), 400},
            ScreenRect{std::numeric_limits<double>::max(), 0, 1, 400},
            ScreenRect{std::numeric_limits<double>::max(), 0, std::numeric_limits<double>::max(), 400}})
        CHECK(rejects({8, 4, 1}, rect));
    CHECK(rejects({1, 1, std::numeric_limits<float>::denorm_min()},
        {0, 0, std::numeric_limits<double>::max(), std::numeric_limits<double>::max()}));
    Camera2D tiny{{1, 1, std::numeric_limits<float>::denorm_min()}, {0, 0, 400, 400}};
    CHECK(std::isfinite(tiny.GetScale()) && tiny.GetScale() > 0);
    CHECK(tiny.TryZoom({200, 200}, 16));
    return true;
}
}

int main() {
    if (!AffineFixtures() || !EdgeZoomFixtures() || !OffsetAndRoundTripFixtures() ||
        !RejectionFixtures() || !StartupFixtures()) return 1;
    std::cout << "Camera independent affine/boundary fixtures passed\n";
}
