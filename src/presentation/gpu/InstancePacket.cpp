#include <crucible/presentation/gpu/InstancePacket.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace crucible::presentation::gpu {
namespace {
bool ValidColor(const OpaqueColor& color) noexcept {
    return color[3] == 1 && std::all_of(color.begin(), color.end(), [](float c) {
        return std::isfinite(c) && c >= 0 && c <= 1;
    });
}
OpaqueColor CellColor(bool infected, bool stocked) noexcept {
    if (stocked) return infected ? OpaqueColor{159 / 255.F, 69 / 255.F, 77 / 255.F, 1}
        : OpaqueColor{22 / 255.F, 38 / 255.F, 55 / 255.F, 1};
    return infected ? OpaqueColor{108 / 255.F, 86 / 255.F, 47 / 255.F, 1}
        : OpaqueColor{33 / 255.F, 59 / 255.F, 55 / 255.F, 1};
}
}
InstancePacket::InstancePacket(std::size_t samples, std::size_t cells) {
    constexpr auto limit = std::numeric_limits<std::uint32_t>::max() / sizeof(Instance);
    if (cells > limit || samples > limit - cells || cells > cells_.max_size() || samples > markers_.max_size())
        throw std::length_error("GPU instance capacity exceeds buffer domain");
    cells_.resize(cells); markers_.resize(samples);
}
bool InstancePacket::TryCapture(const ScenarioSnapshot& frame, std::span<const OpaqueColor> colors,
                                std::span<const OpaqueColor> marker_colors) noexcept {
    const auto info = frame.GetInfo();
    if (!info) return false;
    const auto extent = info->grid.TryValidate();
    const auto samples = frame.GetSamples();
    const auto blight = frame.GetBlight();
    const auto stocks = frame.GetStocks();
    if (!extent || info->cells != extent->cells || blight.size() != extent->cells || samples.size() != info->samples ||
        samples.size() > markers_.size() || blight.size() > cells_.size() ||
        (info->biomass ? stocks.size() != extent->cells : !stocks.empty()) ||
        (!colors.empty() && colors.size() != blight.size()) ||
        (!marker_colors.empty() && marker_colors.size() != samples.size())) return false;
    for (const auto value : blight) if (value > 1) return false;
    for (const auto& color : colors) if (!ValidColor(color)) return false;
    for (const auto& color : marker_colors) if (!ValidColor(color)) return false;
    for (const auto& sample : samples) {
        if (sample.activity != SampleActivity::mobile && sample.activity != SampleActivity::anchored &&
            sample.activity != SampleActivity::lost) return false;
        const auto p = sample.position;
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || p.x < 0 || p.y < 0 ||
            p.x > extent->width || p.y > extent->height) return false;
    }
    const double half_x = (static_cast<double>(info->grid.cell_size) / extent->width) / 2;
    const double half_y = (static_cast<double>(info->grid.cell_size) / extent->height) / 2;
    if (!std::isfinite(half_x) || !std::isfinite(half_y) || static_cast<float>(half_x) <= 0 ||
        static_cast<float>(half_y) <= 0) return false;
    // Every rejection precedes writes; subsequent conversions have bounded ratios.
    for (std::size_t i = 0; i < blight.size(); ++i) {
        const auto x = ((static_cast<double>(i % info->grid.columns) + .5) * info->grid.cell_size) / extent->width;
        const auto y = ((static_cast<double>(i / info->grid.columns) + .5) * info->grid.cell_size) / extent->height;
        cells_[i] = {{static_cast<float>(x), static_cast<float>(y), static_cast<float>(half_x), static_cast<float>(half_y)},
            colors.empty() ? CellColor(blight[i] != 0, stocks.empty() || stocks[i] != 0) : colors[i]};
    }
    std::size_t mobile_count = 0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (samples[i].activity != SampleActivity::mobile) continue;
        markers_[mobile_count++] = {{static_cast<float>(static_cast<double>(samples[i].position.x) / extent->width),
            static_cast<float>(static_cast<double>(samples[i].position.y) / extent->height), 0, 0},
            marker_colors.empty() ? OpaqueColor{119 / 255.F, 221 / 255.F, 1, 1} : marker_colors[i]};
    }
    cell_count_ = blight.size(); sample_count_ = mobile_count; grid_ = info->grid; tick_ = info->completed_tick;
    return true;
}
std::optional<Projection> TryMakeProjection(const InstancePacket& packet, const Camera2D& camera) noexcept {
    const auto grid = packet.GetGrid();
    const auto config = camera.GetConfig();
    const auto view = camera.GetViewport();
    if (!grid || grid->columns != config.columns || grid->rows != config.rows || grid->cell_size != config.cell_size ||
        view.x != 24 || view.y != 96 || view.width != 1232 || view.height != 520) return std::nullopt;
    const auto extent = grid->TryValidate();
    const auto origin = camera.TryToScreen({0, 0});
    if (!extent || !origin) return std::nullopt;
    const auto scale = camera.GetScale();
    const double width = extent->width * scale, height = extent->height * scale;
    if (!std::isfinite(width) || !std::isfinite(height) || width > 20000 || height > 20000 ||
        !std::isfinite(origin->x) || !std::isfinite(origin->y) ||
        std::abs(origin->x) > 20000 || std::abs(origin->y) > 20000) return std::nullopt;
    Projection projection;
    projection.origin_extent = {static_cast<float>(origin->x), static_cast<float>(origin->y),
                               static_cast<float>(width), static_cast<float>(height)};
    projection.canvas_marker[2] = static_cast<float>(std::clamp(scale * .08, 2., 5.));
    return projection;
}
}
