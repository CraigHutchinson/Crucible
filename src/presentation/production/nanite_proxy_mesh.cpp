#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <SwarmIntro.hpp>

#include "crucible/presentation/production/detail/nanite_proxy_mesh.hpp"
#include "crucible/presentation/production/nanite_mesh_frame.hpp"

namespace crucible::presentation::production::detail
{
namespace
{
using Vec = std::array<double, 3>;
using Tetrahedron = std::array<Vec, 4>;
constexpr std::array<std::array<std::size_t, 3>, 4> faces{{{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}}};
Vec add(Vec a, Vec b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
Vec subtract(Vec a, Vec b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vec multiply(Vec a, double s) { return {a[0] * s, a[1] * s, a[2] * s}; }
double dot(Vec a, Vec b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vec cross(Vec a, Vec b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
std::optional<Vec> normal(Vec a, Vec b, Vec c)
{
    const auto n = cross(subtract(b, a), subtract(c, b));
    const double lengthSquared = dot(n, n);
    if (!std::isfinite(lengthSquared) || lengthSquared < 1e-16) return std::nullopt;
    return multiply(n, 1.0 / std::sqrt(lengthSquared));
}
Vec mapCoordinates(Vec p) { return {p[0], p[2], -p[1]}; }
Vec toWorld(Vec local, const spike::swarm::RenderRow& row)
{
    const double c = std::cos(static_cast<double>(row.heading)), s = std::sin(static_cast<double>(row.heading));
    const double a = row.shaped ? local[0] : c * local[0] - s * local[1];
    const double b = row.shaped ? local[1] : s * local[0] + c * local[1];
    const double cp = std::cos(static_cast<double>(row.plane)), sp = std::sin(static_cast<double>(row.plane));
    return mapCoordinates({row.x + a, row.y + b * cp - local[2] * sp, row.z + b * sp + local[2] * cp});
}
double luminance(Vec rgb) { return rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722; }
float shellIntensity(const spike::swarm::RenderRow& row, std::uint8_t deepest)
{
    using spike::swarm::Act;
    constexpr Vec silver{196, 202, 210}, dim{120, 126, 134};
    Vec value = row.act == Act::Dormant || row.act == Act::Reserve ? dim : silver;
    const bool rest = row.act == Act::Dormant || row.act == Act::Locked || row.act == Act::Moored || row.act == Act::Reserve;
    if (row.act == Act::Climb || (row.plane < 0.1F && !rest && row.z > 0.1F))
        for (auto& channel : value) channel = std::min(255.0, channel + 30.0);
    double intensity = luminance(value) / luminance(silver);
    if (deepest && (row.act == Act::Locked || row.act == Act::Dock))
        intensity *= 1.0 - 0.22 * static_cast<double>(deepest - row.layer) / deepest;
    return static_cast<float>(intensity);
}
float coreIntensity(const spike::swarm::RenderRow& row, float choreographySeconds)
{
    using spike::swarm::Act;
    constexpr Vec cyan{60, 230, 255}, dim{30, 90, 110}, settled{40, 160, 185};
    if (row.act == Act::Dormant || (row.act == Act::Reserve && static_cast<double>(choreographySeconds) - row.actSince > 1.0))
        return static_cast<float>(luminance(dim) / luminance(cyan));
    if (row.act == Act::Locked || row.act == Act::Moored)
        return static_cast<float>(luminance(settled) / luminance(cyan));
    return 1.0F;
}

bool emitTriangle(std::vector<NaniteMeshVertex>& output, Vec a, Vec b, Vec c,
    Vec outward, NaniteMaterialRole role, float intensity)
{
    const std::array<Vec, 3> positions{a, b, c};
    std::array<NaniteMeshVertex, 3> triangle;
    for (std::size_t corner = 0; corner < 3; ++corner)
    {
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            if (!std::isfinite(positions[corner][axis]) || std::abs(positions[corner][axis]) > 512 || !std::isfinite(outward[axis]))
                return false;
            triangle[corner].position_[axis] = static_cast<float>(positions[corner][axis]);
            triangle[corner].normal_[axis] = static_cast<float>(outward[axis]);
        }
        triangle[corner].role_ = role;
        triangle[corner].intensity_ = intensity;
    }
    const auto packed = [](const NaniteMeshVertex& v) -> Vec { return {v.position_[0], v.position_[1], v.position_[2]}; };
    const auto n = normal(packed(triangle[0]), packed(triangle[1]), packed(triangle[2]));
    if (!n || dot(*n, outward) < 0.995 || !std::isfinite(intensity) || intensity < 0 || intensity > 4 || output.size() + 3 > NaniteMeshFrame::maxVertices)
        return false;
    output.insert(output.end(), triangle.begin(), triangle.end());
    return true;
}
bool emitQuad(std::vector<NaniteMeshVertex>& output, std::array<Vec, 4> p,
    Vec outward, NaniteMaterialRole role, float intensity)
{
    return emitTriangle(output, p[0], p[1], p[2], outward, role, intensity) &&
        emitTriangle(output, p[0], p[2], p[3], outward, role, intensity);
}

bool emitBody(std::vector<NaniteMeshVertex>& output, const spike::swarm::RenderRow& row,
    const spike::swarm::NaniteBody& body, bool detailed, float shellGlow, float coreGlow)
{
    Tetrahedron compact{};
    if (row.shaped)
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            const auto corner = (i + 2) % 3; // Source docked nose is tile corner2.
            compact[i] = {row.shape[corner * 2], row.shape[corner * 2 + 1], 0};
        }
    }
    else
    {
        const double radius = body.span / std::sqrt(3.0);
        for (std::size_t i = 0; i < 3; ++i)
        {
            const double angle = i == 0 ? 0 : (i == 1 ? 1 : -1) * 2.0 * std::numbers::pi / 3.0;
            compact[i] = {radius * std::cos(angle), radius * std::sin(angle), 0};
        }
    }
    const auto centroid = multiply(add(add(compact[0], compact[1]), compact[2]), 1.0 / 3.0);
    compact[3] = add(centroid, multiply(subtract(compact[0], centroid), body.lean));
    compact[3][2] = body.height;
    const Vec anchor{compact[3][0], compact[3][1], 0.12 * body.height};
    auto toward = subtract(compact[0], compact[3]);
    toward[2] = 0;
    const double towardLength = std::sqrt(dot(toward, toward));
    if (!(towardLength > 1e-12)) return false;
    toward = multiply(toward, 1 / towardLength);
    Tetrahedron unit{}, heart{};
    double edge{};
    for (std::size_t i = 0; i < 3; ++i)
    {
        const double angle = i == 0 ? 0 : (i == 1 ? 1 : -1) * 2.0 * std::numbers::pi / 3.0;
        unit[i] = {(std::cos(angle) * toward[0] - std::sin(angle) * toward[1]) / std::sqrt(3.0),
            (std::sin(angle) * toward[0] + std::cos(angle) * toward[1]) / std::sqrt(3.0), 0};
        const auto difference = subtract(compact[i], compact[(i + 1) % 3]);
        edge += std::sqrt(dot(difference, difference)) / 3.0;
    }
    unit[3] = {0, 0, std::sqrt(2.0 / 3.0)};
    edge *= body.core;
    for (const auto& face : faces)
    {
        const auto outward = normal(compact[face[0]], compact[face[1]], compact[face[2]]);
        if (!outward) return false;
        const double gap = dot(subtract(anchor, compact[face[0]]), *outward);
        double reach = -1e100;
        for (const auto& vertex : unit) reach = std::max(reach, dot(vertex, *outward));
        if (reach > 1e-9) edge = std::min(edge, (-0.04 - gap) / reach);
    }
    if (!std::isfinite(edge) || edge <= 0) return false;
    // Construct the rigid core BEFORE elongating the moving shell's nose.
    for (std::size_t i = 0; i < 4; ++i) heart[i] = toWorld(add(anchor, multiply(unit[i], edge)), row);
    Tetrahedron shell = compact;
    if (!row.shaped)
    {
        double fraction = std::clamp(static_cast<double>(row.speed) / std::max(static_cast<double>(body.stretchSpeed), 1e-6), 0.0, 1.0);
        fraction = fraction * fraction * (3.0 - 2.0 * fraction);
        auto forward = subtract(compact[0], centroid);
        forward = multiply(forward, 1 / std::max(std::sqrt(dot(forward, forward)), 1e-12));
        shell[0] = add(shell[0], multiply(forward, fraction * body.stretch * body.span));
    }
    for (auto& vertex : shell) vertex = toWorld(vertex, row);
    for (const auto& face : faces)
    {
        const std::array<Vec, 3> p{shell[face[0]], shell[face[1]], shell[face[2]]};
        const auto outward = normal(p[0], p[1], p[2]);
        if (!outward) return false;
        const auto center = multiply(add(add(p[0], p[1]), p[2]), 1.0 / 3.0);
        std::array<Vec, 3> window{}, inner{};
        for (std::size_t i = 0; i < 3; ++i)
        {
            window[i] = add(p[i], multiply(subtract(center, p[i]), 0.66));
            inner[i] = add(p[i], multiply(subtract(center, p[i]), 0.20));
        }
        for (std::size_t a = 0; a < 3; ++a)
        {
            const auto b = (a + 1) % 3;
            if (!detailed)
            {
                if (!emitQuad(output, {p[a], p[b], window[b], window[a]}, *outward, NaniteMaterialRole::shield, shellGlow)) return false;
            }
            else
            {
                const auto ha = add(p[a], multiply(subtract(p[b], p[a]), 0.24));
                const auto hb = add(p[b], multiply(subtract(p[a], p[b]), 0.24));
                if (!emitQuad(output, {ha, hb, inner[b], inner[a]}, *outward, NaniteMaterialRole::rail, shellGlow)) return false;
                const auto sunk = multiply(*outward, -0.035);
                if (!emitQuad(output, {add(inner[a], sunk), add(inner[b], sunk), add(window[b], sunk), add(window[a], sunk)},
                    *outward, NaniteMaterialRole::shield, shellGlow)) return false;
            }
        }
        if (detailed)
            for (std::size_t a = 0; a < 3; ++a)
            {
                const auto b = (a + 1) % 3, c = (a + 2) % 3;
                if (!emitQuad(output, {p[a], add(p[a], multiply(subtract(p[b], p[a]), 0.24)), inner[a],
                    add(p[a], multiply(subtract(p[c], p[a]), 0.24))}, *outward, NaniteMaterialRole::hub, shellGlow)) return false;
            }
    }
    for (const auto& face : faces)
    {
        const auto outward = normal(heart[face[0]], heart[face[1]], heart[face[2]]);
        if (!outward || !emitTriangle(output, heart[face[0]], heart[face[1]], heart[face[2]], *outward, NaniteMaterialRole::core, coreGlow)) return false;
    }
    return true;
}
}

NaniteProxyMesh::NaniteProxyMesh() { vertices_.reserve(NaniteMeshFrame::maxVertices); }

bool NaniteProxyMesh::tryBuild(std::span<const spike::swarm::RenderRow> rows,
    const spike::swarm::NaniteBody& body, const spike::swarm::Camera& camera,
    float viewTilt, float choreographySeconds, float diameter)
{
    vertices_.clear();
    detailed_.fill(false);
    detailedCount_ = 0;
    const std::array<double, 14> parameters{body.height, body.span, body.lean, body.core, body.stretch, body.stretchSpeed,
        camera.cx, camera.cy, camera.halfWidth, camera.aspect, camera.pixelsWide, camera.yaw, viewTilt, choreographySeconds};
    if (rows.size() > NaniteMeshFrame::maxUnits || !std::ranges::all_of(parameters, [](double v) { return std::isfinite(v); }) ||
        body.height <= 0 || body.span <= 0 || body.core <= 0 || body.stretch < 0 || body.stretchSpeed <= 0 ||
        camera.halfWidth <= 0.01F || camera.halfWidth > 1024 || camera.aspect < 1.0F / 32 || camera.aspect > 32 ||
        camera.pixelsWide < 1 || camera.pixelsWide > 4096 || !std::isfinite(diameter) || diameter <= 0 ||
        viewTilt < 0 || viewTilt > 4 || choreographySeconds < 0) return false;
    const double yaw = camera.yaw * std::numbers::pi / 180.0, c = std::cos(yaw), s = std::sin(yaw);
    const double halfHeight = camera.halfWidth / camera.aspect;
    const double normalizedHalfHeight = halfHeight / std::sqrt(1.0 + viewTilt * viewTilt);
    if (normalizedHalfHeight <= 0.01 || normalizedHalfHeight > 32768) return false;
    camera_.target_ = {camera.cx, camera.cy, 0};
    camera_.eye_ = {static_cast<float>(camera.cx - s * 64), camera.cy + viewTilt * 64, static_cast<float>(c * 64)};
    camera_.orthographic_ = CinematicCamera::OrthographicExtent{camera.halfWidth, static_cast<float>(normalizedHalfHeight)};
    if (!std::ranges::all_of(camera_.eye_, [](float v) { return std::isfinite(v) && std::abs(v) <= 512; }) ||
        !std::ranges::all_of(camera_.target_, [](float v) { return std::isfinite(v) && std::abs(v) <= 512; })) return false;
    std::uint8_t deepest{};
    std::size_t candidateCount{};
    const bool eligibleScale = camera.pixelsPerUnit() * diameter >= 14;
    for (std::size_t identity = 0; identity < rows.size(); ++identity)
    {
        const auto& row = rows[identity];
        const std::array<float, 8> values{row.x, row.y, row.z, row.heading, row.plane, row.actSince, row.speed, choreographySeconds};
        if (!std::ranges::all_of(values, [](float v) { return std::isfinite(v); }) || row.speed < 0 ||
            static_cast<unsigned>(row.act) > static_cast<unsigned>(spike::swarm::Act::Queue) || static_cast<unsigned>(row.tier) > 3 ||
            !std::ranges::all_of(row.shape, [](float v) { return std::isfinite(v); })) return false;
        deepest = std::max(deepest, row.layer);
        const double dx = (row.x - camera.cx) * c - row.y * s;
        const double dy = row.z + viewTilt * ((row.x - camera.cx) * s + row.y * c) - camera.cy;
        if (eligibleScale && std::abs(dx) <= camera.halfWidth + 2 * diameter && std::abs(dy) <= halfHeight + 2 * diameter)
            candidates_[candidateCount++] = {(dx / camera.halfWidth) * (dx / camera.halfWidth) + (dy / halfHeight) * (dy / halfHeight), identity};
    }
    std::sort(candidates_.begin(), candidates_.begin() + static_cast<std::ptrdiff_t>(candidateCount), [](const auto& a, const auto& b) {
        return a.distance_ < b.distance_ || (a.distance_ == b.distance_ && a.identity_ < b.identity_);
    });
    detailedCount_ = std::min(candidateCount, NaniteMeshFrame::maxDetailedUnits);
    for (std::size_t index = 0; index < detailedCount_; ++index) detailed_[candidates_[index].identity_] = true;
    for (std::size_t identity = 0; identity < rows.size(); ++identity)
        if (!emitBody(vertices_, rows[identity], body, detailed_[identity], shellIntensity(rows[identity], deepest), coreIntensity(rows[identity], choreographySeconds)))
        { vertices_.clear(); return false; }
    return true;
}

NaniteMeshFrame NaniteProxyMesh::getFrame() const noexcept
{
    NaniteMeshFrame frame;
    frame.vertices_ = vertices_;
    frame.camera_ = camera_;
    return frame;
}
}
