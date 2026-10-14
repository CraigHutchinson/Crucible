#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <SwarmIntro.hpp>

#include "crucible/presentation/production/detail/swarm_model_mesh.hpp"

namespace crucible::presentation::production::detail
{
namespace
{
using Vec = std::array<double, 3>;
using Tetrahedron = std::array<Vec, 4>;
using Matrix = std::array<Vec, 3>; // columns
constexpr std::array<std::array<std::size_t, 3>, 4> faces{{{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}}};
constexpr std::size_t cacheLimit = 24 * 1024 * 1024;
constexpr double clearance = 0.04, sliverAreaSquared = 1e-16;

Vec add(Vec a, Vec b) noexcept
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}
Vec subtract(Vec a, Vec b) noexcept
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
Vec multiply(Vec a, double s) noexcept
{
    return {a[0] * s, a[1] * s, a[2] * s};
}
double dot(Vec a, Vec b) noexcept
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Vec cross(Vec a, Vec b) noexcept
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
std::optional<Vec> normalize(Vec value) noexcept
{
    const double lengthSquared = dot(value, value);
    if (!std::isfinite(lengthSquared) || lengthSquared <= 1e-24) return std::nullopt;
    return multiply(value, 1.0 / std::sqrt(lengthSquared));
}
Vec unpack(const std::array<float, 3>& value) noexcept
{
    return {value[0], value[1], value[2]};
}
Vec transformVector(const Matrix& matrix, Vec value) noexcept
{
    return add(add(multiply(matrix[0], value[0]), multiply(matrix[1], value[1])), multiply(matrix[2], value[2]));
}
Matrix cofactor(const Matrix& matrix) noexcept
{
    return {cross(matrix[1], matrix[2]), cross(matrix[2], matrix[0]), cross(matrix[0], matrix[1])};
}
std::optional<Matrix> inverse(const Matrix& matrix) noexcept
{
    const auto cofactors = cofactor(matrix);
    const double determinant = dot(matrix[0], cofactors[0]);
    if (!std::isfinite(determinant) || std::abs(determinant) <= 1e-12) return std::nullopt;
    Matrix result{};
    for (std::size_t column = 0; column < 3; ++column)
        for (std::size_t row = 0; row < 3; ++row) result[column][row] = cofactors[row][column] / determinant;
    return result;
}
Vec centroid(const Tetrahedron& corners) noexcept
{
    Vec value{};
    for (const auto& corner : corners) value = add(value, corner);
    return multiply(value, 0.25);
}
Tetrahedron unitEnvelope() noexcept
{
    const auto envelope = naniteCompactEnvelope();
    Tetrahedron result{};
    for (std::size_t corner = 0; corner < result.size(); ++corner) result[corner] = unpack(envelope[corner]);
    return result;
}
Matrix tetrahedronBasis(const Tetrahedron& corners) noexcept
{
    return {subtract(corners[1], corners[0]), subtract(corners[2], corners[0]), subtract(corners[3], corners[0])};
}
Vec modelToSource(Vec value, double cosine, double sine) noexcept
{
    // A cyclic axis permutation preserves handedness: model +Z is source +X.
    return {cosine * value[2] - sine * value[0], sine * value[2] + cosine * value[0], value[1]};
}
Matrix worldBasis(const spike::swarm::RenderRow& row) noexcept
{
    const double heading = row.shaped ? 0.0 : row.heading;
    const double c = std::cos(heading), s = std::sin(heading);
    const double cp = std::cos(static_cast<double>(row.plane)), sp = std::sin(static_cast<double>(row.plane));
    // Heading, then source plane tilt, then source z-up to native y-up.
    return {{{c, s * sp, -s * cp}, {-s, c * sp, -c * cp}, {0, cp, sp}}};
}
struct UnitGeometry
{
    Tetrahedron compact_{}, shell_{};
    Matrix world_{}, shellCofactor_{};
    Vec origin_{}, centre_{};
    double orientationCosine_{1}, orientationSine_{}, interiorScale_{};
};
std::optional<UnitGeometry> makeGeometry(const spike::swarm::RenderRow& row,
    const spike::swarm::NaniteBody& body, const Matrix& inverseUnit) noexcept
{
    UnitGeometry result;
    for (std::size_t corner = 0; corner < 3; ++corner)
    {
        if (row.shaped)
        {
            const auto sourceCorner = (corner + 2) % 3;
            result.compact_[corner] = {row.shape[sourceCorner * 2], row.shape[sourceCorner * 2 + 1], 0};
        }
        else
        {
            const double angle = corner == 0 ? 0 : (corner == 1 ? 1 : -1) * 2.0 * std::numbers::pi / 3.0;
            const double radius = body.span / std::sqrt(3.0);
            result.compact_[corner] = {radius * std::cos(angle), radius * std::sin(angle), 0};
        }
    }
    const Vec baseCentre = multiply(add(add(result.compact_[0], result.compact_[1]), result.compact_[2]), 1.0 / 3.0);
    result.compact_[3] = add(baseCentre, multiply(subtract(result.compact_[0], baseCentre), body.lean));
    result.compact_[3][2] = body.height;
    const auto direction = normalize(subtract(result.compact_[0], baseCentre));
    if (!direction) return std::nullopt;
    result.orientationCosine_ = (*direction)[0];
    result.orientationSine_ = (*direction)[1];
    result.centre_ = centroid(result.compact_);
    result.interiorScale_ = body.span;
    result.shell_ = result.compact_;
    if (!row.shaped)
    {
        double fraction = std::clamp(static_cast<double>(row.speed) / body.stretchSpeed, 0.0, 1.0);
        fraction = fraction * fraction * (3.0 - 2.0 * fraction);
        result.shell_[0] = add(result.shell_[0], multiply(*direction, fraction * body.stretch * body.span));
    }
    // Source triangles are CCW in their plane; the model's +Y-up base is CW
    // after the proper (z,x,y) permutation. Corresponding corners keep that order.
    const Tetrahedron corresponding{result.shell_[0], result.shell_[2], result.shell_[1], result.shell_[3]};
    const auto sourceBasis = tetrahedronBasis(corresponding);
    Matrix linear{};
    for (std::size_t column = 0; column < 3; ++column) linear[column] = transformVector(sourceBasis, inverseUnit[column]);
    const auto cofactors = cofactor(linear);
    const double determinant = dot(linear[0], cofactors[0]);
    if (!std::isfinite(determinant) || determinant <= 1e-12) return std::nullopt;
    result.shellCofactor_ = cofactors;
    result.world_ = worldBasis(row);
    result.origin_ = {row.x, row.z, -row.y};
    return result;
}
Vec shellPosition(const UnitGeometry& geometry, const std::array<float, 4>& weights) noexcept
{
    const Tetrahedron corresponding{geometry.shell_[0], geometry.shell_[2], geometry.shell_[1], geometry.shell_[3]};
    Vec value{};
    for (std::size_t corner = 0; corner < corresponding.size(); ++corner)
        value = add(value, multiply(corresponding[corner], weights[corner]));
    return value;
}
double luminance(Vec rgb) noexcept
{
    return rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722;
}
float shellIntensity(const spike::swarm::RenderRow& row, std::uint8_t deepest) noexcept
{
    using spike::swarm::Act;
    constexpr Vec silver{196, 202, 210}, dim{120, 126, 134};
    Vec value = row.act == Act::Dormant || row.act == Act::Reserve ? dim : silver;
    const bool resting = row.act == Act::Dormant || row.act == Act::Locked || row.act == Act::Moored || row.act == Act::Reserve;
    if (row.act == Act::Climb || (row.plane < 0.1F && !resting && row.z > 0.1F))
        for (auto& channel : value) channel = std::min(255.0, channel + 30.0);
    double intensity = luminance(value) / luminance(silver);
    if (deepest && (row.act == Act::Locked || row.act == Act::Dock))
        intensity *= 1.0 - 0.22 * static_cast<double>(deepest - row.layer) / deepest;
    return static_cast<float>(intensity);
}
float heartIntensity(const spike::swarm::RenderRow& row, float seconds) noexcept
{
    using spike::swarm::Act;
    constexpr Vec cyan{60, 230, 255}, dim{30, 90, 110}, settled{40, 160, 185};
    if (row.act == Act::Dormant || (row.act == Act::Reserve && static_cast<double>(seconds) - row.actSince > 1.0))
        return static_cast<float>(luminance(dim) / luminance(cyan));
    if (row.act == Act::Locked || row.act == Act::Moored) return static_cast<float>(luminance(settled) / luminance(cyan));
    return 1.0F;
}
NaniteMaterialRole materialRole(NaniteMaterial material) noexcept
{
    switch (material)
    {
    case NaniteMaterial::Silver: return NaniteMaterialRole::shield;
    case NaniteMaterial::Graphite: return NaniteMaterialRole::hub;
    case NaniteMaterial::Heart: return NaniteMaterialRole::core;
    case NaniteMaterial::Housing: return NaniteMaterialRole::shield;
    }
    return NaniteMaterialRole::shield;
}
}

SwarmModelMesh::SwarmModelMesh()
{
    const auto envelope = unitEnvelope();
    const auto inverseUnit = inverse(tetrahedronBasis(envelope));
    if (!inverseUnit) throw std::runtime_error("Nanite compact envelope is degenerate");
    const Vec centre = centroid(envelope);
    const auto ladder = naniteLodLadder();
    if (ladder.size() < rungCount) throw std::runtime_error("Nanite ladder has no consumed prefix");
    for (std::size_t variant = 0; variant < variantCount; ++variant)
    {
        for (std::size_t rung = 0; rung < rungCount; ++rung)
        {
            const auto source = buildNanite(ladder[rung].detail_, {}, static_cast<std::uint32_t>(variant));
            if (source.indices_.size() % 3 != 0) throw std::runtime_error("Nanite cache has an incomplete triangle");
            auto& cache = caches_[variant][rung];
            cache.vertices_.reserve(source.vertices_.size());
            cache.indices_.reserve(source.indices_.size());
            for (const auto& vertex : source.vertices_)
            {
                if (!std::ranges::all_of(vertex.position_, [](float value) { return std::isfinite(value); }) ||
                    !std::ranges::all_of(vertex.normal_, [](float value) { return std::isfinite(value); }) ||
                    !normalize(unpack(vertex.normal_))) throw std::runtime_error("Nanite cache has an invalid vertex");
                const Vec coordinate = transformVector(*inverseUnit, subtract(unpack(vertex.position_), envelope[0]));
                const Vec interior = subtract(unpack(vertex.position_), centre);
                cache.vertices_.push_back({{static_cast<float>(1 - coordinate[0] - coordinate[1] - coordinate[2]),
                    static_cast<float>(coordinate[0]), static_cast<float>(coordinate[1]), static_cast<float>(coordinate[2])},
                    vertex.normal_, {static_cast<float>(interior[0]), static_cast<float>(interior[1]), static_cast<float>(interior[2])},
                    vertex.material_});
            }
            for (std::size_t triangle = 0; triangle < source.indices_.size(); triangle += 3)
            {
                const auto a = source.indices_[triangle], b = source.indices_[triangle + 1], c = source.indices_[triangle + 2];
                if (a >= source.vertices_.size() || b >= source.vertices_.size() || c >= source.vertices_.size())
                    throw std::runtime_error("Nanite cache index is invalid");
                const Vec normal = cross(subtract(unpack(source.vertices_[b].position_), unpack(source.vertices_[a].position_)),
                    subtract(unpack(source.vertices_[c].position_), unpack(source.vertices_[a].position_)));
                if (dot(normal, normal) <= sliverAreaSquared)
                {
                    ++stats_.sliverTriangles_;
                    continue;
                }
                const auto material = source.vertices_[a].material_;
                if (material != source.vertices_[b].material_ || material != source.vertices_[c].material_)
                    throw std::runtime_error("Nanite triangle mixes material roles");
                cache.indices_.insert(cache.indices_.end(), {a, b, c});
                if (material == NaniteMaterial::Housing) ++cache.housingTriangles_;
                else cache.emittedCorners_ += 3;
            }
            stats_.cacheBytes_ += cache.vertices_.capacity() * sizeof(CachedVertex) + cache.indices_.capacity() * sizeof(std::uint32_t);
            if (stats_.cacheBytes_ > cacheLimit) throw std::runtime_error("Nanite cold cache exceeds its storage bound");
        }
    }
    vertices_.reserve(NaniteMeshFrame::maxVertices);
    reset();
}

void SwarmModelMesh::reset() noexcept
{
    vertices_.clear();
    currentRungs_.fill(naniteLodLadder().size());
    chosenRungs_.fill(0);
    visible_.fill(false);
    camera_ = {};
    detailedCount_ = 0;
    stats_.actualRungCounts_.fill(0);
    stats_.omittedHousingTriangles_ = 0;
    stats_.visibleUnits_ = 0;
    stats_.culledUnits_ = 0;
}

bool SwarmModelMesh::tryBuild(std::span<const spike::swarm::RenderRow> rows,
    const spike::swarm::NaniteBody& body, const spike::swarm::Camera& camera,
    float viewTilt, float choreographySeconds, float diameter)
{
    vertices_.clear();
    detailedCount_ = 0;
    stats_.actualRungCounts_.fill(0);
    stats_.omittedHousingTriangles_ = 0;
    stats_.visibleUnits_ = 0;
    stats_.culledUnits_ = 0;
    const auto refuse = [&]()
    {
        vertices_.clear();
        detailedCount_ = 0;
        stats_.actualRungCounts_.fill(0);
        stats_.omittedHousingTriangles_ = 0;
        stats_.visibleUnits_ = 0;
        stats_.culledUnits_ = 0;
        return false;
    };
    const std::array<double, 14> parameters{body.height, body.span, body.lean, body.core, body.stretch, body.stretchSpeed,
        camera.cx, camera.cy, camera.halfWidth, camera.aspect, camera.pixelsWide, camera.yaw, viewTilt, choreographySeconds};
    if (rows.size() > NaniteMeshFrame::maxUnits || !std::ranges::all_of(parameters, [](double value) { return std::isfinite(value); }) ||
        body.height <= 0 || body.span <= 0 || body.core <= 0 || body.stretch < 0 || body.stretchSpeed <= 0 ||
        camera.halfWidth <= 0.01F || camera.halfWidth > 1024 || camera.aspect < 1.0F / 32 || camera.aspect > 32 ||
        camera.pixelsWide < 1 || camera.pixelsWide > 4096 || !std::isfinite(diameter) || diameter <= 0 ||
        viewTilt < 0 || viewTilt > 4 || choreographySeconds < 0) return false;
    const auto inverseUnit = inverse(tetrahedronBasis(unitEnvelope()));
    if (!inverseUnit) return false;
    const double yaw = camera.yaw * std::numbers::pi / 180.0, c = std::cos(yaw), s = std::sin(yaw);
    const double halfHeight = camera.halfWidth / camera.aspect;
    const double normalizedHalfHeight = halfHeight / std::sqrt(1.0 + viewTilt * viewTilt);
    if (normalizedHalfHeight <= 0.01 || normalizedHalfHeight > 32768) return false;
    camera_.target_ = {camera.cx, camera.cy, 0};
    camera_.eye_ = {static_cast<float>(camera.cx - s * 64), camera.cy + viewTilt * 64, static_cast<float>(c * 64)};
    camera_.orthographic_ = CinematicCamera::OrthographicExtent{camera.halfWidth, static_cast<float>(normalizedHalfHeight)};
    if (!std::ranges::all_of(camera_.eye_, [](float value) { return std::isfinite(value) && std::abs(value) <= 512; }) ||
        !std::ranges::all_of(camera_.target_, [](float value) { return std::isfinite(value) && std::abs(value) <= 512; })) return false;
    const auto cap = naniteLodLadder()[rungCount - 1].detail_;
    const double pixelsPerUnit = camera.pixelsWide / (2.0 * camera.halfWidth);
    std::size_t candidateCount{}, corners{}, visibleCount{};
    std::uint8_t deepest{};
    chosenRungs_.fill(0);
    visible_.fill(false);
    for (std::size_t identity = 0; identity < rows.size(); ++identity)
    {
        const auto& row = rows[identity];
        const std::array<float, 8> values{row.x, row.y, row.z, row.heading, row.plane, row.actSince, row.speed, choreographySeconds};
        if (!std::ranges::all_of(values, [](float value) { return std::isfinite(value); }) || row.speed < 0 ||
            static_cast<unsigned>(row.act) > static_cast<unsigned>(spike::swarm::Act::Queue) || static_cast<unsigned>(row.tier) > 3 ||
            !std::ranges::all_of(row.shape, [](float value) { return std::isfinite(value); })) return false;
        deepest = std::max(deepest, row.layer);
        const auto geometry = makeGeometry(row, body, *inverseUnit);
        if (!geometry) return false;
        double minX = 1e100, maxX = -1e100, minY = 1e100, maxY = -1e100;
        for (const auto& corner : geometry->shell_)
        {
            const auto world = add(geometry->origin_, transformVector(geometry->world_, corner));
            if (!std::ranges::all_of(world, [](double value) { return std::isfinite(value) && std::abs(value) <= 512; })) return false;
            const double sourceX = world[0], sourceY = -world[2], sourceZ = world[1];
            const double x = (sourceX - camera.cx) * c - sourceY * s;
            const double y = sourceZ + viewTilt * ((sourceX - camera.cx) * s + sourceY * c) - camera.cy;
            minX = std::min(minX, x); maxX = std::max(maxX, x);
            minY = std::min(minY, y); maxY = std::max(maxY, y);
        }
        // This bounds the complete mapped shell, including the moving nose.
        // The margin covers the generator's tolerated tiny rounding excursions.
        constexpr double visibilityMargin = 0.01;
        if (maxX < -camera.halfWidth - visibilityMargin || minX > camera.halfWidth + visibilityMargin ||
            maxY < -halfHeight - visibilityMargin || minY > halfHeight + visibilityMargin) continue;
        visible_[identity] = true;
        ++visibleCount;
        const double projectedDiameter = std::hypot(maxX - minX, maxY - minY) * pixelsPerUnit;
        const auto target = selectNaniteLod(static_cast<float>(projectedDiameter), cap, currentRungs_[identity]);
        const double dx = (row.x - camera.cx) * c - row.y * s;
        const double dy = row.z + viewTilt * ((row.x - camera.cx) * s + row.y * c) - camera.cy;
        if (target > 0 && maxX >= -camera.halfWidth - 2 * diameter && minX <= camera.halfWidth + 2 * diameter &&
            maxY >= -halfHeight - 2 * diameter && minY <= halfHeight + 2 * diameter)
        {
            candidates_[candidateCount++] = {(dx / camera.halfWidth) * (dx / camera.halfWidth) + (dy / halfHeight) * (dy / halfHeight), identity};
            chosenRungs_[identity] = target;
        }
        corners += caches_[identity % variantCount][0].emittedCorners_;
    }
    if (corners > NaniteMeshFrame::maxVertices) return false;
    std::sort(candidates_.begin(), candidates_.begin() + static_cast<std::ptrdiff_t>(candidateCount), [](const auto& a, const auto& b)
    {
        return a.distance_ < b.distance_ || (a.distance_ == b.distance_ && a.identity_ < b.identity_);
    });
    for (std::size_t candidate = 0; candidate < candidateCount; ++candidate)
    {
        const auto identity = candidates_[candidate].identity_;
        auto rung = chosenRungs_[identity];
        const auto& cache = caches_[identity % variantCount];
        if (candidate >= NaniteMeshFrame::maxDetailedUnits) rung = 0;
        while (rung > 0 && corners + cache[rung].emittedCorners_ - cache[0].emittedCorners_ > NaniteMeshFrame::maxVertices) --rung;
        corners += cache[rung].emittedCorners_ - cache[0].emittedCorners_;
        chosenRungs_[identity] = rung;
    }
    for (std::size_t identity = 0; identity < rows.size(); ++identity)
    {
        if (!visible_[identity]) continue;
        const auto& row = rows[identity];
        const auto rung = chosenRungs_[identity];
        const auto& cache = caches_[identity % variantCount][rung];
        auto geometry = makeGeometry(row, body, *inverseUnit);
        if (!geometry) return refuse();
        if (rung > 0)
        {
            for (const auto& face : faces)
            {
                auto outward = normalize(cross(subtract(geometry->compact_[face[1]], geometry->compact_[face[0]]),
                    subtract(geometry->compact_[face[2]], geometry->compact_[face[0]])));
                if (!outward) return refuse();
                if (dot(*outward, subtract(geometry->compact_[face[0]], geometry->centre_)) < 0) *outward = multiply(*outward, -1);
                const double available = dot(*outward, subtract(geometry->compact_[face[0]], geometry->centre_)) - clearance;
                if (available <= 0) return refuse();
                for (const auto& vertex : cache.vertices_)
                {
                    if (vertex.material_ != NaniteMaterial::Housing && vertex.material_ != NaniteMaterial::Heart) continue;
                    const Vec offset = modelToSource(unpack(vertex.interior_), geometry->orientationCosine_, geometry->orientationSine_);
                    const double reach = dot(offset, *outward);
                    if (reach > 1e-12) geometry->interiorScale_ = std::min(geometry->interiorScale_, available / reach);
                }
            }
            if (!std::isfinite(geometry->interiorScale_) || geometry->interiorScale_ <= 0) return refuse();
        }
        const float shellGlow = shellIntensity(row, deepest), coreGlow = heartIntensity(row, choreographySeconds);
        for (std::size_t triangle = 0; triangle < cache.indices_.size(); triangle += 3)
        {
            const auto material = cache.vertices_[cache.indices_[triangle]].material_;
            if (material == NaniteMaterial::Housing) continue;
            std::array<NaniteMeshVertex, 3> output{};
            std::array<Vec, 3> mappedPositions{};
            for (std::size_t corner = 0; corner < output.size(); ++corner)
            {
                const auto& vertex = cache.vertices_[cache.indices_[triangle + corner]];
                const bool physicalHeart = rung > 0 && material == NaniteMaterial::Heart;
                const Vec local = physicalHeart ? add(geometry->centre_, multiply(modelToSource(unpack(vertex.interior_),
                    geometry->orientationCosine_, geometry->orientationSine_), geometry->interiorScale_)) : shellPosition(*geometry, vertex.weights_);
                const Vec position = add(geometry->origin_, transformVector(geometry->world_, local));
                mappedPositions[corner] = position;
                const Vec sourceNormal = physicalHeart ? modelToSource(unpack(vertex.normal_), geometry->orientationCosine_, geometry->orientationSine_)
                    : transformVector(geometry->shellCofactor_, unpack(vertex.normal_));
                const auto normal = normalize(transformVector(geometry->world_, sourceNormal));
                if (!normal) return refuse();
                for (std::size_t axis = 0; axis < 3; ++axis)
                {
                    if (!std::isfinite(position[axis]) || std::abs(position[axis]) > 512) return refuse();
                    output[corner].position_[axis] = static_cast<float>(position[axis]);
                    output[corner].normal_[axis] = static_cast<float>((*normal)[axis]);
                }
                output[corner].role_ = materialRole(material);
                output[corner].intensity_ = material == NaniteMaterial::Heart ? coreGlow : shellGlow;
            }
            const Vec geometric = cross(subtract(unpack(output[1].position_), unpack(output[0].position_)),
                subtract(unpack(output[2].position_), unpack(output[0].position_)));
            const auto faceNormal = normalize(geometric);
            const Vec mappedFace = cross(subtract(mappedPositions[1], mappedPositions[0]),
                subtract(mappedPositions[2], mappedPositions[0]));
            if (!faceNormal || dot(geometric, geometric) <= sliverAreaSquared || dot(geometric, mappedFace) <= 0 ||
                vertices_.size() + 3 > NaniteMeshFrame::maxVertices) return refuse();
            // Anisotropic docking/stretch can move a smoothed cofactor normal
            // outside this face's shading hemisphere. Preserve supported normals
            // and use the emitted face for only the unsupported corner. Winding
            // is proved above against the double mapped face, before this repair.
            for (auto& vertex : output)
            {
                if (dot(*faceNormal, unpack(vertex.normal_)) < 0.01)
                    for (std::size_t axis = 0; axis < 3; ++axis)
                        vertex.normal_[axis] = static_cast<float>((*faceNormal)[axis]);
            }
            if (!std::ranges::all_of(output, [&](const auto& vertex) { return dot(*faceNormal, unpack(vertex.normal_)) >= 0.01; }))
                return refuse();
            vertices_.insert(vertices_.end(), output.begin(), output.end());
        }
        ++stats_.actualRungCounts_[rung];
        stats_.omittedHousingTriangles_ += cache.housingTriangles_;
        detailedCount_ += rung > 0 ? 1 : 0;
    }
    for (std::size_t identity = 0; identity < rows.size(); ++identity) currentRungs_[identity] = chosenRungs_[identity];
    stats_.visibleUnits_ = visibleCount;
    stats_.culledUnits_ = rows.size() - visibleCount;
    return true;
}

NaniteMeshFrame SwarmModelMesh::getFrame() const noexcept
{
    NaniteMeshFrame frame;
    frame.vertices_ = vertices_;
    frame.camera_ = camera_;
    frame.normalMode_ = NaniteNormalMode::smooth;
    return frame;
}
}
