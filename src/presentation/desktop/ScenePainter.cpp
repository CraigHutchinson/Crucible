#include <crucible/presentation/desktop/ScenePainter.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cinttypes>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace crucible::presentation::desktop {
namespace {
constexpr SDL_FColor Background{11 / 255.F, 19 / 255.F, 32 / 255.F, 1};
constexpr SDL_FColor Stocked{22 / 255.F, 38 / 255.F, 55 / 255.F, 1};
constexpr SDL_FColor Infected{159 / 255.F, 69 / 255.F, 77 / 255.F, 1};
constexpr SDL_FColor ExhaustedInfected{108 / 255.F, 86 / 255.F, 47 / 255.F, 1};
constexpr SDL_FColor ExhaustedClear{33 / 255.F, 59 / 255.F, 55 / 255.F, 1};
constexpr SDL_FColor Nanite{119 / 255.F, 221 / 255.F, 255 / 255.F, 1};
constexpr SDL_FColor Attract{115 / 255.F, 227 / 255.F, 173 / 255.F, 1};
constexpr SDL_FColor Repel{1, 199 / 255.F, 107 / 255.F, 1};
constexpr SDL_FColor Text{.86F, .91F, .98F, 1};
constexpr ScreenRect View{24, 96, 1232, 520};

bool Color(SDL_Renderer& renderer, SDL_FColor color) noexcept {
    return SDL_SetRenderDrawColorFloat(&renderer, color.r, color.g, color.b, color.a);
}

// All world vertices are clipped in double before narrowing for SDL's rasterizers.
bool Quad(SDL_Vertex* vertices, double left, double top, double right, double bottom,
          SDL_FColor color) noexcept {
    left = std::clamp(left, View.x, View.x + View.width);
    right = std::clamp(right, View.x, View.x + View.width);
    top = std::clamp(top, View.y, View.y + View.height);
    bottom = std::clamp(bottom, View.y, View.y + View.height);
    const auto x0 = static_cast<float>(left), x1 = static_cast<float>(right);
    const auto y0 = static_cast<float>(top), y1 = static_cast<float>(bottom);
    vertices[0] = {{x0, y0}, color, {}};
    vertices[1] = {{x1, y0}, color, {}};
    vertices[2] = {{x1, y1}, color, {}};
    vertices[3] = {{x0, y1}, color, {}};
    return right > left && bottom > top;
}

// Liang-Barsky segment clipping keeps even finite out-of-world fields rasterizer-safe.
bool ClipLine(ScreenPoint& a, ScreenPoint& b) noexcept {
    const auto dx = b.x - a.x, dy = b.y - a.y;
    double first = 0, last = 1;
    const std::array p{-dx, dx, -dy, dy};
    const std::array q{a.x - View.x, View.x + View.width - a.x,
                       a.y - View.y, View.y + View.height - a.y};
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (p[i] == 0) { if (q[i] < 0) return false; }
        else {
            const auto t = q[i] / p[i];
            if (p[i] < 0) first = std::max(first, t);
            else last = std::min(last, t);
            if (first > last) return false;
        }
    }
    b = {std::clamp(a.x + last * dx, View.x, View.x + View.width),
         std::clamp(a.y + last * dy, View.y, View.y + View.height)};
    a = {std::clamp(a.x + first * dx, View.x, View.x + View.width),
         std::clamp(a.y + first * dy, View.y, View.y + View.height)};
    return true;
}

bool Ring(SDL_Renderer& renderer, const FieldEdit& field, ScreenPoint origin,
          double scale, bool preview, bool selected) noexcept {
    if (field.kind != FieldEditKind::set || field.radius == 0) return true;
    constexpr std::size_t Segments = 64;
    std::array<SDL_Vertex, Segments * 6> vertices{};
    std::size_t used = 0;
    const double cx = origin.x + field.center.x * scale;
    const double cy = origin.y + field.center.y * scale;
    const double radius = field.radius * scale;
    const SDL_FColor color = preview ? Text : (field.strength >= 0 ? Attract : Repel);
    for (std::size_t i = 0; i < Segments; ++i) {
        if (preview && (i / 2) % 2 != 0) continue;
        const auto angle = 2 * std::numbers::pi * static_cast<double>(i) / Segments;
        const auto next = 2 * std::numbers::pi * static_cast<double>(i + 1) / Segments;
        ScreenPoint a{cx + radius * std::cos(angle), cy + radius * std::sin(angle)};
        ScreenPoint b{cx + radius * std::cos(next), cy + radius * std::sin(next)};
        if (!ClipLine(a, b)) continue;
        const auto dx = b.x - a.x, dy = b.y - a.y;
        const auto length = std::hypot(dx, dy);
        if (length == 0) continue;
        const auto thickness = selected || preview ? 1.5 : .75;
        const auto nx = -dy / length * thickness, ny = dx / length * thickness;
        const auto point = [](double x, double y) {
            return SDL_FPoint{static_cast<float>(std::clamp(x, View.x, View.x + View.width)),
                              static_cast<float>(std::clamp(y, View.y, View.y + View.height))};
        };
        const SDL_Vertex v0{point(a.x + nx, a.y + ny), color, {}};
        const SDL_Vertex v1{point(b.x + nx, b.y + ny), color, {}};
        const SDL_Vertex v2{point(b.x - nx, b.y - ny), color, {}};
        const SDL_Vertex v3{point(a.x - nx, a.y - ny), color, {}};
        for (const auto& v : {v0, v1, v2, v0, v2, v3}) vertices[used++] = v;
    }
    return used == 0 || SDL_RenderGeometry(&renderer, nullptr, vertices.data(), static_cast<int>(used), nullptr, 0);
}

bool Label(SDL_Renderer& renderer, float x, float y, const char* text, float scale = 1) noexcept {
    if (!SDL_SetRenderScale(&renderer, scale, scale)) return false;
    const bool drawn = SDL_RenderDebugText(&renderer, x / scale, y / scale, text);
    const bool restored = SDL_SetRenderScale(&renderer, 1, 1);
    return drawn && restored;
}
}

ScenePainter::ScenePainter(std::size_t sample_capacity, std::size_t cell_capacity)
    : sample_capacity_{sample_capacity}, cell_capacity_{cell_capacity} {
    const auto count = std::max(sample_capacity, cell_capacity);
    if (count > static_cast<std::size_t>(std::numeric_limits<int>::max() / 6) ||
        count > vertices_.max_size() / 4 || count > indices_.max_size() / 6 ||
        count > std::numeric_limits<std::size_t>::max() / (4 * sizeof(SDL_Vertex) + 6 * sizeof(int)))
        throw std::length_error("ScenePainter geometry capacity is unrepresentable");
    vertices_.resize(count * 4);
    indices_.resize(count * 6);
    for (std::size_t i = 0; i < count; ++i) {
        const auto base = static_cast<int>(i * 4);
        const std::array quad{base, base + 1, base + 2, base, base + 2, base + 3};
        std::copy(quad.begin(), quad.end(), indices_.begin() + static_cast<std::ptrdiff_t>(i * 6));
    }
}
ScenePainter::~ScenePainter() = default;

bool ScenePainter::TryDraw(SDL_Renderer& renderer, const ScenarioSnapshot& frame,
                           const Camera2D& camera, const SceneUi& ui) noexcept {
    const auto info = frame.GetInfo();
    const auto reject = [](const char* why) { return SDL_SetError("ScenePainter: %s", why); };
    if (!info) return reject("frame has not been captured");
    const auto extent = info->grid.TryValidate();
    const auto samples = frame.GetSamples();
    const auto fields = frame.GetFields();
    const auto blight = frame.GetBlight();
    const auto stocks = frame.GetStocks();
    if (!extent || info->cells != extent->cells || samples.size() != info->samples ||
        fields.size() != info->fields || blight.size() != info->cells ||
        (info->biomass ? stocks.size() != info->cells : !stocks.empty()))
        return reject("inconsistent captured frame");
    if (samples.size() > sample_capacity_ || blight.size() > cell_capacity_)
        return reject("frame exceeds startup capacity");
    const auto config = camera.GetConfig();
    const auto view = camera.GetViewport();
    const auto scale = camera.GetScale();
    const auto origin = camera.TryToScreen({0, 0});
    if (config.columns != info->grid.columns || config.rows != info->grid.rows ||
        config.cell_size != info->grid.cell_size || view.x != View.x || view.y != View.y ||
        view.width != View.width || view.height != View.height || !origin || !std::isfinite(scale) || scale <= 0 ||
        !camera.TryToScreen({extent->width, extent->height}))
        return reject("camera geometry does not match canvas/frame");
    for (const auto& sample : samples)
        if (!camera.TryToScreen(sample.position)) return reject("invalid sample projection");
    for (const auto value : blight) if (value > 1) return reject("invalid infection value");
    for (std::size_t i = 0; i < fields.size(); ++i)
        if (fields[i].slot != i || !fields[i].IsValid(fields.size())) return reject("invalid committed field");
    if (ui.preview && !ui.preview->IsValid(fields.size())) return reject("invalid preview field");

    if (!SDL_SetRenderViewport(&renderer, nullptr) || !SDL_SetRenderClipRect(&renderer, nullptr) ||
        !SDL_SetRenderScale(&renderer, 1, 1) || !Color(renderer, Background) || !SDL_RenderClear(&renderer)) return false;
    const SDL_Rect clip{24, 96, 1232, 520};
    if (!SDL_SetRenderClipRect(&renderer, &clip)) return false;
    for (std::size_t i = 0; i < blight.size(); ++i) {
        const auto col = i % config.columns, row = i / config.columns;
        const auto x = origin->x + static_cast<double>(col) * config.cell_size * scale;
        const auto y = origin->y + static_cast<double>(row) * config.cell_size * scale;
        const auto color = stocks.empty() || stocks[i] != 0 ? (blight[i] ? Infected : Stocked)
            : (blight[i] ? ExhaustedInfected : ExhaustedClear);
        (void)Quad(vertices_.data() + i * 4, x, y, x + config.cell_size * scale, y + config.cell_size * scale, color);
    }
    if (!blight.empty() && !SDL_RenderGeometry(&renderer, nullptr, vertices_.data(),
            static_cast<int>(blight.size() * 4), indices_.data(), static_cast<int>(blight.size() * 6))) return false;
    const auto half = std::clamp(scale * .08, 2., 5.);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto point = *camera.TryToScreen(samples[i].position);
        (void)Quad(vertices_.data() + i * 4, point.x - half, point.y - half,
                   point.x + half, point.y + half, Nanite);
    }
    if (!samples.empty() && !SDL_RenderGeometry(&renderer, nullptr, vertices_.data(),
            static_cast<int>(samples.size() * 4), indices_.data(), static_cast<int>(samples.size() * 6))) return false;
    for (const auto& field : fields)
        if (!Ring(renderer, field, *origin, scale, false, field.slot == ui.selected_slot)) return false;
    if (ui.preview && !Ring(renderer, *ui.preview, *origin, scale, true, true)) return false;
    if (!SDL_SetRenderClipRect(&renderer, nullptr) || !Color(renderer, Text)) return false;
    if (!Label(renderer, 24, 16, "CRUCIBLE / FINITE RECLAMATION", 2)) return false;
    char line[192]{};
    SDL_snprintf(line, sizeof line, "Tick %" PRIu64 " | Nanites %zu | %s%s | Slot %zu", info->completed_tick,
                 samples.size(), ui.paused ? "PAUSED" : "RUNNING", ui.blocked ? " / BLOCKED" : "", ui.selected_slot + 1);
    if (!Label(renderer, 24, 44, line, 1.5F)) return false;
    if (info->biomass) {
        const auto& mass = *info->biomass;
        SDL_snprintf(line, sizeof line, "Mass: initial %" PRIu64 " = stock %" PRIu64 " + mobile %" PRIu64 " + reserve %" PRIu64,
                     mass.initial_total, mass.remaining_stock, mass.mobile_mass, mass.reserve);
    } else SDL_snprintf(line, sizeof line, "Mass ledger unavailable in this scenario");
    if (!Label(renderer, 24, 70, line) ||
        !Label(renderer, 24, 84, "Cells: red infected+stock | ochre infected+empty | green reclaimed | cyan nanites | white dashed preview")) return false;
    const std::array labels{"1 ATTRACT", "2 REPEL", "3 ERASE", "TAB SLOT", ui.paused ? "RESUME" : "PAUSE", "RESTART", "FIT VIEW"};
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const auto bounds = ToolbarButton(i);
        const SDL_FRect button{static_cast<float>(bounds.x), static_cast<float>(bounds.y),
                               static_cast<float>(bounds.width), static_cast<float>(bounds.height)};
        const auto active = (i == 0 && ui.tool == FieldTool::attract) || (i == 1 && ui.tool == FieldTool::repel) ||
                            (i == 2 && ui.tool == FieldTool::remove) || (i == 4 && ui.paused);
        if (!Color(renderer, active ? Stocked : Background) || !SDL_RenderFillRect(&renderer, &button) ||
            !Color(renderer, active ? Attract : Text) || !SDL_RenderRect(&renderer, &button) ||
            !Label(renderer, button.x + 8, button.y + 12, labels[i])) return false;
    }
    // A view owns no message storage; copy the current borrow into bounded ASCII text.
    const auto length = std::min(ui.message.size(), std::size_t{153});
    for (std::size_t i = 0; i < length; ++i) {
        const auto c = static_cast<unsigned char>(ui.message[i]);
        line[i] = c >= 32 && c <= 126 ? static_cast<char>(c) : '?';
    }
    line[length] = '\0';
    if (!Color(renderer, Text) || !Label(renderer, 24, 678, line) ||
        !Label(renderer, 24, 702, "Click world: field | Middle drag: pan | Wheel: zoom | Space: pause | R: restart | F: fit | Del: erase | Esc: cancel")) return false;
    return true;
}
}
