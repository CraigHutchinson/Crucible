#include <algorithm>
#include <array>
#include <cinttypes>
#include <cmath>
#include <limits>
#include <numbers>
#include <SDL3/SDL.h>
#include <stdexcept>

#include "crucible/presentation/Camera2D.hpp"
#include "crucible/presentation/desktop/ScenePainter.hpp"
#include "crucible/presentation/desktop/SceneUi.hpp"
#include "crucible/presentation/ScenarioSnapshot.hpp"

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
constexpr SDL_FColor Lattice{1, .76F, .32F, 1};
constexpr ScreenRect View{24, 96, 1232, 520};
constexpr double DensityCellSize = 4;
constexpr auto DensityColumns = static_cast<std::size_t>(View.width / DensityCellSize);
constexpr auto DensityRows = static_cast<std::size_t>(View.height / DensityCellSize);
static_assert(DensityColumns * DensityCellSize == View.width && DensityRows * DensityCellSize == View.height);

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

// Clipped strokes share the radial/corridor/arrow rasterization boundary.
bool Stroke(SDL_Renderer& renderer, ScreenPoint a, ScreenPoint b, SDL_FColor color,
            double thickness) noexcept {
    if (!ClipLine(a, b)) return true;
    const auto dx = b.x - a.x, dy = b.y - a.y;
    const auto length = std::hypot(dx, dy);
    if (length == 0) return true;
    const auto nx = -dy / length * thickness, ny = dx / length * thickness;
    const auto vertex = [color](double x, double y) {
        return SDL_Vertex{{static_cast<float>(std::clamp(x, View.x, View.x + View.width)),
                           static_cast<float>(std::clamp(y, View.y, View.y + View.height))}, color, {}};
    };
    const auto v0 = vertex(a.x + nx, a.y + ny), v1 = vertex(b.x + nx, b.y + ny);
    const auto v2 = vertex(b.x - nx, b.y - ny), v3 = vertex(a.x - nx, a.y - ny);
    const std::array vertices{v0, v1, v2, v0, v2, v3};
    return SDL_RenderGeometry(&renderer, nullptr, vertices.data(), static_cast<int>(vertices.size()), nullptr, 0);
}

bool Relay(SDL_Renderer& renderer, const StructuralState& state, ScreenPoint origin, double scale) noexcept {
    const ScreenPoint center{origin.x + state.settings.relay_center.x * scale,
                             origin.y + state.settings.relay_center.y * scale};
    const auto circle = [&](double radius, SDL_FColor color, bool dashed) {
        for (std::size_t i = 0; i < 64; ++i) {
            if (dashed && (i / 2) % 2 != 0) continue;
            const auto a = 2 * std::numbers::pi * static_cast<double>(i) / 64;
            const auto b = 2 * std::numbers::pi * static_cast<double>(i + 1) / 64;
            if (!Stroke(renderer, {center.x + radius * std::cos(a), center.y + radius * std::sin(a)},
                {center.x + radius * std::cos(b), center.y + radius * std::sin(b)}, color, .75)) return false;
        }
        return true;
    };
    if (!circle(state.settings.eligibility_radius * scale,
        state.eligible_mobile >= StructuralSettings::cost ? Attract : Text, true)) return false;
    if (state.occupied && !circle(state.settings.protection_radius * scale, Lattice, false)) return false;
    const auto half = std::clamp(scale * .6, 6., 24.);
    const std::array points{ScreenPoint{center.x, center.y - half}, ScreenPoint{center.x + half, center.y},
                           ScreenPoint{center.x, center.y + half}, ScreenPoint{center.x - half, center.y}};
    for (std::size_t i = 0; i < points.size(); ++i)
        if (!Stroke(renderer, points[i], points[(i + 1) % points.size()], state.occupied ? Lattice : Text, 1.5)) return false;
    return !state.occupied ||
        (Stroke(renderer, points[0], points[2], Lattice, 1) && Stroke(renderer, points[1], points[3], Lattice, 1));
}

bool Arrow(SDL_Renderer& renderer, ScreenPoint center, ScreenPoint direction,
           SDL_FColor color, double thickness) noexcept {
    const ScreenPoint tip{center.x + direction.x * 8, center.y + direction.y * 8};
    const ScreenPoint back{center.x - direction.x * 4, center.y - direction.y * 4};
    return Stroke(renderer, {back.x - direction.y * 5, back.y + direction.x * 5}, tip, color, thickness) &&
           Stroke(renderer, {back.x + direction.y * 5, back.y - direction.x * 5}, tip, color, thickness);
}

bool Field(SDL_Renderer& renderer, const FieldEdit& field, ScreenPoint origin,
           double scale, bool preview, bool selected) noexcept {
    if (field.kind == FieldEditKind::remove || field.radius == 0) return true;
    const double cx = origin.x + static_cast<double>(field.center.x) * scale;
    const double cy = origin.y + static_cast<double>(field.center.y) * scale;
    const double radius = static_cast<double>(field.radius) * scale;
    const auto color = preview ? Text : (field.strength >= 0 ? Attract : Repel);
    const auto thickness = preview || selected ? 1.5 : .75;
    if (field.kind == FieldEditKind::set) {
        if (!Ring(renderer, field, origin, scale, preview, selected)) return false;
        if (field.strength == 0) return true;
        for (std::size_t i = 0; i < 8; ++i) {
            const auto angle = 2 * std::numbers::pi * static_cast<double>(i) / 8;
            const auto x = std::cos(angle), y = std::sin(angle);
            const auto sign = field.strength > 0 ? -1. : 1.;
            if (!Arrow(renderer, {cx + .6 * radius * x, cy + .6 * radius * y},
                       {sign * x, sign * y}, color, thickness)) return false;
        }
        return true;
    }
    const ScreenPoint a{cx, cy};
    const ScreenPoint b{origin.x + static_cast<double>(field.end.x) * scale,
                        origin.y + static_cast<double>(field.end.y) * scale};
    const auto dx = b.x - a.x, dy = b.y - a.y;
    const auto length = std::hypot(dx, dy);
    if (length == 0) return true;
    const ScreenPoint direction{dx / length, dy / length};
    const ScreenPoint normal{-direction.y * radius, direction.x * radius};
    // Subdivision keeps preview boundaries dashed with a fixed, bounded amount of work.
    for (std::size_t i = 0; i < 32; ++i) {
        if (preview && (i / 2) % 2 != 0) continue;
        const auto t0 = static_cast<double>(i) / 32, t1 = static_cast<double>(i + 1) / 32;
        for (const auto sign : {-1., 1.}) {
            if (!Stroke(renderer, {a.x + dx * t0 + sign * normal.x, a.y + dy * t0 + sign * normal.y},
                        {a.x + dx * t1 + sign * normal.x, a.y + dy * t1 + sign * normal.y}, color, thickness)) return false;
        }
        const auto angle0 = std::numbers::pi * t0, angle1 = std::numbers::pi * t1;
        for (const auto sign : {-1., 1.}) {
            const auto center = sign < 0 ? a : b;
            const auto cap = [&](double angle) {
                return ScreenPoint{center.x + sign * direction.x * radius * std::sin(angle) + normal.x * std::cos(angle),
                                   center.y + sign * direction.y * radius * std::sin(angle) + normal.y * std::cos(angle)};
            };
            if (!Stroke(renderer, cap(angle0), cap(angle1), color, thickness)) return false;
        }
    }
    if (field.strength == 0) return true;
    auto visible_a = a, visible_b = b;
    if (!ClipLine(visible_a, visible_b)) return true;
    for (const auto t : {.2, .5, .8}) {
        const ScreenPoint center{visible_a.x + (visible_b.x - visible_a.x) * t,
                                 visible_a.y + (visible_b.y - visible_a.y) * t};
        if (!Arrow(renderer, center, direction, color, thickness)) return false;
    }
    return true;
}

bool Label(SDL_Renderer& renderer, float x, float y, const char* text, float scale = 1) noexcept {
    if (!SDL_SetRenderScale(&renderer, scale, scale)) return false;
    const bool drawn = SDL_RenderDebugText(&renderer, x / scale, y / scale, text);
    const bool restored = SDL_SetRenderScale(&renderer, 1, 1);
    return drawn && restored;
}
bool drawFeedback(SDL_Renderer& renderer, std::string_view message) noexcept {
    constexpr std::size_t columns = 61;
    for (int row = 0; row < 2 && !message.empty(); ++row) {
        auto count = std::min(message.size(), columns);
        if (message.size() > columns && row == 0) {
            const auto space = message.substr(0, columns).find_last_of(' ');
            if (space != std::string_view::npos && space != 0) count = space;
        }
        char line[columns + 1]{};
        for (std::size_t i = 0; i < count; ++i) {
            const auto c = static_cast<unsigned char>(message[i]);
            line[i] = c >= 32 && c <= 126 ? static_cast<char>(c) : '?';
        }
        if (row == 1 && message.size() > count) {
            line[count - 3] = '.'; line[count - 2] = '.'; line[count - 1] = '.';
        }
        if (!Label(renderer, 24, 744.F + 24.F * static_cast<float>(row), line, BodyTextScale)) return false;
        message.remove_prefix(count);
        while (!message.empty() && message.front() == ' ') message.remove_prefix(1);
    }
    return true;
}
}

ScenePainter::ScenePainter(std::size_t sample_capacity, std::size_t cell_capacity, ViewPolicy policy)
    : sample_capacity_{sample_capacity}, cell_capacity_{cell_capacity}, policy_{policy} {
    if (policy != ViewPolicy::exact && policy != ViewPolicy::densityOverview)
        throw std::invalid_argument("Invalid painter view policy");
    const auto count = std::max(sample_capacity, cell_capacity);
    if (count > static_cast<std::size_t>(std::numeric_limits<int>::max() / 6) ||
        count > vertices_.max_size() / 4 || count > indices_.max_size() / 6 ||
        count > std::numeric_limits<std::size_t>::max() / (4 * sizeof(SDL_Vertex) + 6 * sizeof(int)))
        throw std::length_error("ScenePainter geometry capacity is unrepresentable");
    vertices_.resize(count * 4);
    indices_.resize(count * 6);
    if (policy == ViewPolicy::densityOverview) densityBins_.resize(DensityColumns * DensityRows);
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
    for (const auto& sample : samples) {
        if (sample.activity != SampleActivity::mobile && sample.activity != SampleActivity::anchored &&
            sample.activity != SampleActivity::lost) return reject("invalid sample activity");
        if (!camera.TryToScreen(sample.position)) return reject("invalid sample projection");
    }
    if (info->structural) {
        const auto& structure = *info->structural;
        if (!info->biomass || !camera.TryToScreen(structure.settings.relay_center) ||
            !std::isfinite(structure.settings.eligibility_radius) || structure.settings.eligibility_radius < 0 ||
            !std::isfinite(structure.settings.protection_radius) || structure.settings.protection_radius < 0 ||
            structure.settings.hold_ticks == 0) return reject("invalid structural observation");
    }
    for (const auto value : blight) if (value > 1) return reject("invalid infection value");
    for (std::size_t i = 0; i < fields.size(); ++i)
        if (fields[i].slot != i || !fields[i].IsValid(fields.size())) return reject("invalid committed field");
    if (ui.preview && !ui.preview->IsValid(fields.size())) return reject("invalid preview field");

    if (ui.mission) {
        const auto& mission = *ui.mission;
        if (!info->biomass || mission.completed_tick != info->completed_tick ||
            mission.settings.target_reclaimed == 0 || mission.settings.deadline_ticks == 0 ||
            mission.reclaimed > info->biomass->reserve) return reject("mission does not match captured boundary");
        const bool quota = mission.reclaimed >= mission.settings.target_reclaimed;
        const bool held = !info->structural || (info->structural->occupied &&
            info->structural->hold_ticks >= info->structural->settings.hold_ticks);
        const bool won = quota && held;
        const bool starved = info->structural && info->biomass->mobile_mass +
            (info->structural->occupied ? StructuralSettings::refund : 0) < StructuralSettings::cost;
        const bool deadline = mission.completed_tick >= mission.settings.deadline_ticks;
        switch (mission.outcome) {
        case ReclamationMissionOutcome::active:
            if (won || deadline || starved) return reject("active mission has reached an outcome");
            break;
        case ReclamationMissionOutcome::won:
            if (!won || mission.completed_tick > mission.settings.deadline_ticks) return reject("invalid winning mission");
            break;
        case ReclamationMissionOutcome::lost:
            if (won || (!starved && mission.completed_tick != mission.settings.deadline_ticks)) return reject("invalid losing mission");
            break;
        default: return reject("invalid mission outcome");
        }
    }

    if (!SDL_SetRenderViewport(&renderer, nullptr) || !SDL_SetRenderClipRect(&renderer, nullptr) ||
        !SDL_SetRenderScale(&renderer, 1, 1) || !Color(renderer, Background) || !SDL_RenderClear(&renderer)) return false;
    const SDL_Rect clip{24, 96, 1232, 520};
    if (!SDL_SetRenderClipRect(&renderer, &clip)) return false;
    DrawStatistics statistics{};
    std::size_t visibleCells = 0;
    for (std::size_t i = 0; i < blight.size(); ++i) {
        const auto col = i % config.columns, row = i / config.columns;
        const auto x = origin->x + static_cast<double>(col) * config.cell_size * scale;
        const auto y = origin->y + static_cast<double>(row) * config.cell_size * scale;
        const auto color = stocks.empty() || stocks[i] != 0 ? (blight[i] ? Infected : Stocked)
            : (blight[i] ? ExhaustedInfected : ExhaustedClear);
        if (Quad(vertices_.data() + visibleCells * 4, x, y,
                 x + config.cell_size * scale, y + config.cell_size * scale, color)) ++visibleCells;
    }
    statistics.visibleCells = visibleCells;
    if (visibleCells && !SDL_RenderGeometry(&renderer, nullptr, vertices_.data(),
            static_cast<int>(visibleCells * 4), indices_.data(), static_cast<int>(visibleCells * 6))) return false;
    const auto half = std::clamp(scale * .08, 2., 5.);
    const bool density = policy_ == ViewPolicy::densityOverview && scale < 4;
    if (density) std::ranges::fill(densityBins_, DensityBin{});
    std::size_t mobile_count = 0;
    for (const auto& sample : samples) {
        if (sample.activity != SampleActivity::mobile) continue;
        ++statistics.authoritativeMobile;
        const auto point = *camera.TryToScreen(sample.position);
        if (point.x + half <= View.x || point.y + half <= View.y ||
            point.x - half >= View.x + View.width || point.y - half >= View.y + View.height) {
            ++statistics.hiddenSamples;
            continue;
        }
        if (density) {
            const auto x = static_cast<std::size_t>(std::clamp((point.x - View.x) / DensityCellSize,
                0., static_cast<double>(DensityColumns - 1)));
            const auto y = static_cast<std::size_t>(std::clamp((point.y - View.y) / DensityCellSize,
                0., static_cast<double>(DensityRows - 1)));
            auto& bin = densityBins_[y * DensityColumns + x];
            ++bin.count;
            bin.x += std::clamp(point.x, View.x, View.x + View.width);
            bin.y += std::clamp(point.y, View.y, View.y + View.height);
            ++statistics.aggregatedSamples;
        } else {
            if (Quad(vertices_.data() + mobile_count * 4, point.x - half, point.y - half,
                     point.x + half, point.y + half, Nanite)) {
                ++mobile_count;
                ++statistics.individualSamples;
            }
        }
    }
    if (density) {
        for (const auto& bin : densityBins_) {
            if (bin.count == 0) continue;
            const auto count = static_cast<double>(bin.count);
            const auto intensity = static_cast<float>(std::clamp(std::log2(1. + count) / 6., .35, 1.));
            const SDL_FColor color{Nanite.r * intensity, Nanite.g * intensity, Nanite.b * intensity, 1};
            const auto x = bin.x / count, y = bin.y / count;
            if (Quad(vertices_.data() + mobile_count * 4, x - 1, y - 1, x + 1, y + 1, color)) {
                ++mobile_count;
                ++statistics.aggregateMarks;
            }
        }
    }
    if (mobile_count && !SDL_RenderGeometry(&renderer, nullptr, vertices_.data(),
            static_cast<int>(mobile_count * 4), indices_.data(), static_cast<int>(mobile_count * 6))) return false;
    if (info->structural && !Relay(renderer, *info->structural, *origin, scale)) return false;
    for (const auto& field : fields)
        if (!Field(renderer, field, *origin, scale, false, field.slot == ui.selected_slot)) return false;
    if (ui.preview && !Field(renderer, *ui.preview, *origin, scale, true, true)) return false;
    if (!SDL_SetRenderClipRect(&renderer, nullptr) || !Color(renderer, Text)) return false;
    if (!Label(renderer, 24, 16, info->structural ? "CRUCIBLE / SECURE THE RELAY" : "CRUCIBLE / FINITE RECLAMATION", 2)) return false;
    char line[192]{};
    if (ui.mission) {
        const auto& mission = *ui.mission;
        const auto outcome = mission.outcome;
        const auto color = outcome == ReclamationMissionOutcome::won ? Attract :
            (outcome == ReclamationMissionOutcome::lost ? Repel : Nanite);
        const char* label = outcome == ReclamationMissionOutcome::won ? "WON" :
            (outcome == ReclamationMissionOutcome::lost ? "LOST" : "ACTIVE");
        if (!Color(renderer, color) || !Label(renderer, 536, 16, label, 2)) return false;
        const SDL_FRect track{840, 16, 416, 16};
        const auto ratio = static_cast<double>(std::min(mission.reclaimed, mission.settings.target_reclaimed)) /
            static_cast<double>(mission.settings.target_reclaimed);
        const SDL_FRect progress{track.x + 1, track.y + 1, static_cast<float>(414 * std::clamp(ratio, 0., 1.)), 14};
        if (!Color(renderer, Stocked) || !SDL_RenderFillRect(&renderer, &track) || !Color(renderer, color) ||
            (progress.w > 0 && !SDL_RenderFillRect(&renderer, &progress)) || !SDL_RenderRect(&renderer, &track)) return false;
        const auto ticks_left = mission.completed_tick < mission.settings.deadline_ticks
            ? mission.settings.deadline_ticks - mission.completed_tick : 0;
        SDL_snprintf(line, sizeof line, "Recovered %" PRIu64 " / %" PRIu64 " | Ticks left %" PRIu64,
                     mission.reclaimed, mission.settings.target_reclaimed, ticks_left);
        if (!Color(renderer, Text) || !Label(renderer, 24, 34, line, BodyTextScale)) return false;
    }
    const char* run_state = ui.mission && ui.mission->outcome != ReclamationMissionOutcome::active
        ? "STOPPED" : (ui.paused ? "PAUSED" : "RUNNING");
    SDL_snprintf(line, sizeof line, "Tick %" PRIu64 " | Nanites %zu | %s%s | Slot %zu", info->completed_tick,
                 statistics.authoritativeMobile, run_state, ui.blocked ? " / BLOCKED" : "", ui.selected_slot + 1);
    if (!Label(renderer, 24, 54, line, BodyTextScale)) return false;
    if (info->biomass) {
        const auto& mass = *info->biomass;
        SDL_snprintf(line, sizeof line, "Stock %" PRIu64 "|Mobile %" PRIu64 "|Reserve %" PRIu64 "|Lattice %" PRIu64 "|Lost %" PRIu64,
                     mass.remaining_stock, mass.mobile_mass, mass.reserve, mass.structure_mass, mass.lost_mass);
    } else SDL_snprintf(line, sizeof line, "Mass ledger unavailable in this scenario");
    if (!Label(renderer, 24, 74, line, BodyTextScale)) return false;
    const char* instruction = "Red: infected | Green: reclaimed | Cyan: swarm";
    if (ui.mission) {
        switch (ui.mission->outcome) {
        case ReclamationMissionOutcome::active:
            instruction = "Recover quota before the deadline. FLOW drag / radial click.";
            break;
        case ReclamationMissionOutcome::won:
            instruction = "WON: Quota recovered. Run stopped; R or RESTART to play again.";
            break;
        case ReclamationMissionOutcome::lost:
            instruction = "LOST: Deadline reached. Run stopped; R or RESTART to try again.";
            break;
        }
    }
    if (info->structural && ui.mission) {
        switch (ui.mission->outcome) {
        case ReclamationMissionOutcome::active:
            instruction = "Quota + hold relay120 ticks. Shatter returns48, loses16.";
            break;
        case ReclamationMissionOutcome::won:
            instruction = "WON: Quota and relay secured. R or RESTART to play again.";
            break;
        case ReclamationMissionOutcome::lost:
            instruction = "LOST: Deadline or too little mass. R or RESTART to try again.";
            break;
        }
    }
    if (!Label(renderer, 24, 792, instruction, BodyTextScale)) return false;
    if (info->structural) {
        const auto& state = *info->structural;
        SDL_snprintf(line, sizeof line, "Relay: eligible %zu/64 | %s | Held %" PRIu64 "/%" PRIu64 " ticks",
            state.eligible_mobile, state.occupied ? "ANCHORED" : "EMPTY", state.hold_ticks, state.settings.hold_ticks);
        if (!Label(renderer, 24, 622, line, BodyTextScale)) return false;
    }
    const std::array labels{"1 ATTRACT", "2 REPEL", "3 ERASE", "TAB SLOT", ui.paused ? "RESUME" : "PAUSE", "RESTART", "FIT VIEW", "4 FLOW",
        "F FUSE", "X SHATTER", ui.fullscreen_ ? "F11 WIN" : "F11 FULL"};
    static_assert(labels.size() == ToolbarButtonCount);
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const auto bounds = toolbarButton(i);
        const SDL_FRect button{static_cast<float>(bounds.x), static_cast<float>(bounds.y),
                               static_cast<float>(bounds.width), static_cast<float>(bounds.height)};
        const auto active = (i == 0 && ui.tool == FieldTool::attract) || (i == 1 && ui.tool == FieldTool::repel) ||
                            (i == 2 && ui.tool == FieldTool::remove) || (i == 4 && ui.paused) ||
                            (i == 7 && ui.tool == FieldTool::flow);
        const bool enabled = isToolbarActionEnabled(static_cast<ToolbarAction>(i), info->structural.has_value());
        if (!Color(renderer, active ? Stocked : Background) || !SDL_RenderFillRect(&renderer, &button) ||
            !Color(renderer, !enabled ? ExhaustedInfected : (active ? Attract : Text)) || !SDL_RenderRect(&renderer, &button) ||
            !Label(renderer, button.x + 8, button.y + 11, labels[i], BodyTextScale)) return false;
    }
    if (!Color(renderer, Text) || !drawFeedback(renderer, ui.message) ||
        !Label(renderer, 24, 816, "Middle pan | Wheel zoom | Tab slot | Del erase", BodyTextScale) ||
        !Label(renderer, 24, 840, "Space pause | R restart | F11 full | Esc cancel / window", BodyTextScale)) return false;
    statistics_ = statistics;
    return true;
}
}
