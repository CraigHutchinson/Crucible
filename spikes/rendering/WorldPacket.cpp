#include "WorldPacket.hpp"
#include <cmath>
#include <crucible/presentation/ScenarioSnapshot.hpp>
namespace crucible::spike {
WorldPacket::WorldPacket(std::size_t samples, std::size_t cells) : cells_(cells), markers_(samples) {}
bool WorldPacket::TryPack(const presentation::ScenarioSnapshot& frame) noexcept {
    const auto info = frame.GetInfo();
    if (!info) return false;
    const auto extent = info->grid.TryValidate();
    const auto samples = frame.GetSamples();
    const auto blight = frame.GetBlight();
    const auto stocks = frame.GetStocks();
    const float half = info->grid.cell_size / 2;
    if (!extent || half <= 0 || samples.size() != info->samples || blight.size() != extent->cells ||
        samples.size() > markers_.size() || blight.size() > cells_.size() ||
        (info->biomass ? stocks.size() != extent->cells : !stocks.empty())) return false;
    for (const auto value : blight) if (value > 1) return false;
    for (const auto& sample : samples) {
        const auto p = sample.position;
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || p.x < 0 || p.y < 0 ||
            p.x > extent->width || p.y > extent->height) return false;
    }
    // Colors mirror the live world pass; the independent consumer tests compare actual raster output.
    for (std::size_t i = 0; i < blight.size(); ++i) {
        const bool stocked = stocks.empty() || stocks[i] != 0;
        const std::array<float, 4> color = stocked
            ? (blight[i] ? std::array{159 / 255.F, 69 / 255.F, 77 / 255.F, 1.F}
                         : std::array{22 / 255.F, 38 / 255.F, 55 / 255.F, 1.F})
            : (blight[i] ? std::array{108 / 255.F, 86 / 255.F, 47 / 255.F, 1.F}
                         : std::array{33 / 255.F, 59 / 255.F, 55 / 255.F, 1.F});
        const auto x = static_cast<float>((static_cast<double>(i % info->grid.columns) + .5) * info->grid.cell_size);
        const auto y = static_cast<float>((static_cast<double>(i / info->grid.columns) + .5) * info->grid.cell_size);
        cells_[i] = {{x, y, half, half}, color};
    }
    for (std::size_t i = 0; i < samples.size(); ++i)
        markers_[i] = {{samples[i].position.x, samples[i].position.y, .08F, .08F},
                       {119 / 255.F, 221 / 255.F, 1.F, 1.F}};
    cell_count_ = blight.size(); sample_count_ = samples.size();
    grid_ = info->grid; tick_ = info->completed_tick;
    return true;
}
}
