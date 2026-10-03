#include <crucible/presentation/gpu/InstancePacket.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/simulation.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
using namespace crucible;
using namespace crucible::presentation;
using namespace crucible::presentation::gpu;
constexpr ScreenRect View{24, 96, 1232, 520};
void Require(bool condition, const char* why) { if (!condition) throw std::runtime_error(why); }
void VertexDomain(GridConfig grid) {
    const auto extent = *grid.TryValidate();
    Simulation simulation{3, {grid, 0}};
    ScenarioSnapshot frame{3, 0, extent.cells};
    InstancePacket packet{3, extent.cells};
    Require(frame.TryCapture(simulation) && packet.TryCapture(frame), "normalized fixture capture");
    Camera2D camera{grid, View};
    const auto verify = [&] {
        const auto projection = TryMakeProjection(packet, camera);
        Require(projection.has_value(), "bounded camera projection");
        const auto origin = *camera.TryToScreen({0, 0});
        const auto& p = projection->origin_extent;
        for (std::size_t i = 0; i < extent.cells; ++i) {
            const auto& g = packet.GetCells()[i].geometry;
            const std::array center{static_cast<double>(i % grid.columns) + .5,
                                    static_cast<double>(i / grid.columns) + .5};
            for (const float sign : {-1.F, 1.F}) {
                const double expected_x = std::clamp(origin.x + (center[0] + sign * .5) * grid.cell_size * camera.GetScale(), 24., 1256.);
                const double expected_y = std::clamp(origin.y + (center[1] + sign * .5) * grid.cell_size * camera.GetScale(), 96., 616.);
                // Exercise contracted and uncontracted float shader arithmetic independently.
                for (const bool fma : {false, true}) {
                    const float cx = fma ? std::fma(g[0], p[2], p[0]) : p[0] + g[0] * p[2];
                    const float cy = fma ? std::fma(g[1], p[3], p[1]) : p[1] + g[1] * p[3];
                    const float x = std::clamp(cx + sign * (g[2] * p[2]), 24.F, 1256.F);
                    const float y = std::clamp(cy + sign * (g[3] * p[3]), 96.F, 616.F);
                    Require(std::abs(x - expected_x) <= .01 && std::abs(y - expected_y) <= .01, "frozen .01 pixel cell vertex bound");
                }
            }
        }
        for (std::size_t i = 0; i < frame.GetSamples().size(); ++i) {
            const auto expected = *camera.TryToScreen(frame.GetSamples()[i].position);
            const auto& g = packet.GetMarkers()[i].geometry;
            const auto x = std::fma(g[0], p[2], p[0]), y = std::fma(g[1], p[3], p[1]);
            Require(std::abs(x - expected.x) <= .01 && std::abs(y - expected.y) <= .01, "frozen marker center bound");
            Require(projection->canvas_marker[2] >= 2 && projection->canvas_marker[2] <= 5, "marker logical clamp");
        }
    };
    verify();
    Require(camera.TryZoom({640, 356}, 16) && camera.TryPan({37, -19}), "zoom/pan fixture");
    verify();
    camera.ResetFit(); verify();
    Require(!camera.TryToWorld({23, 356}) && !camera.TryToWorld({640, 616}), "strict exterior unchanged");
}
void OwnershipAndFailure() {
    const GridConfig grid{3, 1, 1};
    Simulation simulation{3, {grid, 0}};
    ScenarioSnapshot frame{3, 0, 3};
    InstancePacket packet{3, 3};
    std::array<OpaqueColor, 3> cells{{{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}}};
    std::array<OpaqueColor, 3> markers{{{1, 1, 0, 1}, {0, 1, 1, 1}, {1, 0, 1, 1}}};
    Require(frame.TryCapture(simulation) && packet.TryCapture(frame, cells, markers), "arbitrary palette capture");
    const auto cell = packet.GetCells()[0], marker = packet.GetMarkers()[0];
    cells[0] = {0, 0, 0, 1}; markers[0] = {0, 0, 0, 1};
    simulation.tick(); Require(frame.TryCapture(simulation), "later mutable snapshot");
    Require(packet.GetTick() == 0 && std::memcmp(&cell, &packet.GetCells()[0], sizeof cell) == 0 &&
        std::memcmp(&marker, &packet.GetMarkers()[0], sizeof marker) == 0, "owned packet survives color/frame changes");
    cells[0][3] = .5F;
    Require(!packet.TryCapture(frame, cells, markers) && packet.GetTick() == 0 &&
        std::memcmp(&cell, &packet.GetCells()[0], sizeof cell) == 0, "alpha rejection preserves previous records");
    cells[0] = {std::numeric_limits<float>::quiet_NaN(), 0, 0, 1};
    Require(!packet.TryCapture(frame, cells, markers), "nonfinite color rejected");
    Require(!packet.TryCapture(frame, std::span<const OpaqueColor>{cells}.first(1)), "partial palette rejected");
    InstancePacket too_small{2, 3}; Require(!too_small.TryCapture(frame) && !too_small.GetGrid(), "capacity rejection atomic");
    Camera2D wrong{{1, 1, 1}, View}; Require(!TryMakeProjection(packet, wrong), "wrong camera rejected");
    bool rejected{};
    try { InstancePacket huge{std::numeric_limits<std::size_t>::max(), 1}; }
    catch (const std::length_error&) { rejected = true; }
    Require(rejected, "checked buffer byte domain");
}
}
int main() {
    try {
        VertexDomain({7, 5, .1F}); VertexDomain({1, 1, std::numeric_limits<float>::denorm_min()});
        VertexDomain({4, 1, std::numeric_limits<float>::max() / 4});
        VertexDomain({64, 32, 1}); OwnershipAndFailure();
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
