#include "PacketPainter.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include "WorldPacket.hpp"
#include <crucible/presentation/Camera2D.hpp>
#include <SDL3/SDL.h>
namespace crucible::spike {
PacketPainter::PacketPainter(std::size_t capacity) {
    if (capacity > static_cast<std::size_t>(std::numeric_limits<int>::max() / 6) ||
        capacity > vertices_.max_size() / 4 || capacity > indices_.max_size() / 6)
        throw std::length_error("packet painter capacity");
    vertices_.resize(capacity * 4); indices_.resize(capacity * 6);
    for (std::size_t i = 0; i < capacity; ++i) {
        const auto base = static_cast<int>(i * 4);
        const std::array indices{base, base + 1, base + 2, base, base + 2, base + 3};
        std::copy(indices.begin(), indices.end(), indices_.begin() + static_cast<std::ptrdiff_t>(i * 6));
    }
}
PacketPainter::~PacketPainter() = default;
bool PacketPainter::TryDraw(SDL_Renderer& renderer, const WorldPacket& packet,
                             const presentation::Camera2D& camera) noexcept {
    const auto grid = packet.GetGrid();
    const auto config = camera.GetConfig();
    const auto view = camera.GetViewport();
    if (!grid || grid->columns != config.columns || grid->rows != config.rows || grid->cell_size != config.cell_size ||
        packet.GetCells().size() > vertices_.size() / 4 || packet.GetMarkers().size() > vertices_.size() / 4 ||
        view.x != 24 || view.y != 96 || view.width != 1232 || view.height != 520) return SDL_SetError("packet preflight");
    if (!SDL_SetRenderViewport(&renderer, nullptr) || !SDL_SetRenderClipRect(&renderer, nullptr) ||
        !SDL_SetRenderScale(&renderer, 1, 1) || !SDL_SetRenderDrawColor(&renderer, 11, 19, 32, 255) ||
        !SDL_RenderClear(&renderer)) return false;
    const SDL_Rect clip{24, 96, 1232, 520};
    if (!SDL_SetRenderClipRect(&renderer, &clip)) return false;
    const auto draw = [&](std::span<const WorldInstance> instances, auto half_size) {
        for (std::size_t i = 0; i < instances.size(); ++i) {
            const auto& item = instances[i];
            const auto center = camera.TryToScreen({item.geometry[0], item.geometry[1]});
            if (!center) return SDL_SetError("instance projection");
            const auto half = half_size(item);
            const float left = static_cast<float>(std::clamp(center->x - half[0], 24., 1256.));
            const float right = static_cast<float>(std::clamp(center->x + half[0], 24., 1256.));
            const float top = static_cast<float>(std::clamp(center->y - half[1], 96., 616.));
            const float bottom = static_cast<float>(std::clamp(center->y + half[1], 96., 616.));
            const SDL_FColor color{item.color[0], item.color[1], item.color[2], item.color[3]};
            vertices_[i * 4] = {{left, top}, color, {}};
            vertices_[i * 4 + 1] = {{right, top}, color, {}};
            vertices_[i * 4 + 2] = {{right, bottom}, color, {}};
            vertices_[i * 4 + 3] = {{left, bottom}, color, {}};
        }
        return instances.empty() || SDL_RenderGeometry(&renderer, nullptr, vertices_.data(),
            static_cast<int>(instances.size() * 4), indices_.data(), static_cast<int>(instances.size() * 6));
    };
    const auto scale = camera.GetScale();
    return draw(packet.GetCells(), [scale](const WorldInstance& i) { return std::array{i.geometry[2] * scale, i.geometry[3] * scale}; }) &&
        draw(packet.GetMarkers(), [scale](const WorldInstance& i) { return std::array{std::clamp(i.geometry[2] * scale, 2., 5.), std::clamp(i.geometry[3] * scale, 2., 5.)}; }) &&
        SDL_SetRenderClipRect(&renderer, nullptr);
}
}
