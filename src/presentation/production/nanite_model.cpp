#include "crucible/presentation/production/nanite_model.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace crucible::presentation::production
{
namespace
{
using Vec = std::array<float, 3>;

constexpr Vec operator+(Vec a, Vec b) noexcept { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
constexpr Vec operator-(Vec a, Vec b) noexcept { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
constexpr Vec operator*(Vec a, float s) noexcept { return {a[0] * s, a[1] * s, a[2] * s}; }
constexpr float dot(Vec a, Vec b) noexcept { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
constexpr Vec cross(Vec a, Vec b) noexcept
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
Vec normalize(Vec a) noexcept
{
    const float length = std::sqrt(dot(a, a));
    return length > 0.0F ? a * (1.0F / length) : a;
}
constexpr Vec lerp(Vec a, Vec b, float t) noexcept { return a + (b - a) * t; }

/// Rotates `point` about the line through `origin` along unit `axis` (Rodrigues).
Vec rotateAbout(Vec point, Vec origin, Vec axis, float angle) noexcept
{
    const Vec p = point - origin;
    const float c = std::cos(angle), s = std::sin(angle);
    return origin + p * c + cross(axis, p) * s + axis * (dot(axis, p) * (1.0F - c));
}

// Unit proportions, in shell edges. The shell is a regular tetrahedron at rest.
constexpr float shellHeight = 0.81649658F; // sqrt(2/3)
constexpr float shellCircum = 0.57735027F; // base circumradius, 1/sqrt(3)
constexpr float shellIn = 0.28867513F;     // base inradius, 1/(2 sqrt(3))
constexpr float noseReach = 0.9F;          // nose travel at full stretch
constexpr float hubFraction = 0.17F;       // hub length along each edge
constexpr float railHalfWidth = 0.035F;
constexpr float railDepth = 0.06F;
constexpr float plateRim = 0.13F;          // plate outer edge, fraction toward the face centroid
constexpr float plateWindow = 0.48F;       // plate inner edge (window), fraction toward the centroid
constexpr float plateRecess = 0.012F;
constexpr float plateThickness = 0.04F;
constexpr float plateGap = 0.035F;         // Articulated: gap between neighbouring plates, in edge lengths
constexpr float plateLift = 0.07F;         // Shielded: shield lift at full open
constexpr float plateHinge = 0.4F;         // Articulated: hinge angle at full open, radians
constexpr float coreEdge = 0.42F;          // Frame: solid core
constexpr float housingEdge = 0.6F;        // Shielded+: containment housing
constexpr float heartRadius = 0.094F;      // inside the housing's inscribed sphere (0.122)

/// Welds equal vertices and orients every triangle outward from a hint direction.
class Builder
{
public:
    Builder(NaniteMesh& mesh, std::size_t triangles) : mesh_(mesh)
    {
        mesh_.indices_.reserve(triangles * 3);
        mesh_.vertices_.reserve(triangles);
        slots_.resize(std::bit_ceil(triangles + 64)); // about two triangles per welded vertex
    }

    void triangle(Vec a, Vec b, Vec c, Vec na, Vec nb, Vec nc, Vec outward, NaniteMaterial material)
    {
        if (dot(cross(b - a, c - a), outward) < 0.0F)
        {
            std::swap(b, c);
            std::swap(nb, nc);
        }
        mesh_.indices_.push_back(vertex(a, na, material));
        mesh_.indices_.push_back(vertex(b, nb, material));
        mesh_.indices_.push_back(vertex(c, nc, material));
    }

    void flat(Vec a, Vec b, Vec c, Vec outward, NaniteMaterial material)
    {
        Vec normal = normalize(cross(b - a, c - a));
        if (dot(normal, outward) < 0.0F) normal = normal * -1.0F;
        triangle(a, b, c, normal, normal, normal, outward, material);
    }

private:
    using Key = std::array<std::int32_t, 7>;
    /// Open-addressing weld table: one slot per key, `index_ == empty` when unused.
    struct Slot
    {
        Key key_{};
        std::uint32_t index_{empty};
    };
    static constexpr std::uint32_t empty = 0xFFFFFFFFU;

    std::uint32_t vertex(Vec position, Vec normal, NaniteMaterial material)
    {
        const auto q = [](float value, float scale)
        { return static_cast<std::int32_t>(value * scale + (value < 0.0F ? -0.5F : 0.5F)); };
        const Key key{q(position[0], 1e5F), q(position[1], 1e5F), q(position[2], 1e5F),
            q(normal[0], 1e4F), q(normal[1], 1e4F), q(normal[2], 1e4F), static_cast<std::int32_t>(material)};
        if ((mesh_.vertices_.size() + 1) * 2 > slots_.size()) grow();
        Slot& slot = find(key);
        if (slot.index_ == empty)
        {
            slot = {key, static_cast<std::uint32_t>(mesh_.vertices_.size())};
            mesh_.vertices_.push_back({position, normal, material});
        }
        return slot.index_;
    }

    Slot& find(const Key& key)
    {
        std::uint64_t hash = 1469598103934665603ULL;
        for (const auto value : key) hash = (hash ^ static_cast<std::uint32_t>(value)) * 1099511628211ULL;
        const std::size_t mask = slots_.size() - 1;
        for (std::size_t index = static_cast<std::size_t>(hash ^ (hash >> 29)) & mask;; index = (index + 1) & mask)
        {
            if (slots_[index].index_ == empty || slots_[index].key_ == key) return slots_[index];
        }
    }

    void grow()
    {
        std::vector<Slot> old(slots_.size() * 2);
        old.swap(slots_);
        for (const auto& slot : old)
        {
            if (slot.index_ != empty) find(slot.key_) = slot;
        }
    }

    NaniteMesh& mesh_;
    std::vector<Slot> slots_;
};

/// A convex polyhedron whose every vertex joins exactly three faces (tetrahedra, prisms).
struct Polyhedron
{
    std::vector<Vec> vertices_;
    std::vector<std::vector<int>> faces_;
};

Polyhedron tetrahedron(Vec a, Vec b, Vec c, Vec d)
{
    return {{a, b, c, d}, {{0, 1, 2}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}}};
}

/// A convex planar profile swept along `extrusion`.
Polyhedron prism(const std::vector<Vec>& profile, Vec extrusion)
{
    Polyhedron result;
    const int count = static_cast<int>(profile.size());
    for (const auto& point : profile) result.vertices_.push_back(point);
    for (const auto& point : profile) result.vertices_.push_back(point + extrusion);
    std::vector<int> bottom, top;
    for (int index = 0; index < count; ++index)
    {
        bottom.push_back(index);
        top.push_back(index + count);
        const int next = (index + 1) % count;
        result.faces_.push_back({index, next, next + count, index + count});
    }
    result.faces_.push_back(bottom);
    result.faces_.push_back(top);
    return result;
}

/// A square-section bar from `from` to `to`, its section aligned to `up`.
Polyhedron bar(Vec from, Vec to, Vec up, float halfWidth, float halfDepth)
{
    const Vec along = normalize(to - from);
    const Vec side = normalize(cross(along, up)) * halfWidth;
    const Vec lift = normalize(cross(side, along)) * halfDepth;
    return prism({from - side - lift, from + side - lift, from + side + lift, from - side + lift}, to - from);
}

/** Emits a polyhedron rounded by `radius` without changing its outer extent: faces are
 * offset inward, edges become cylindrical strips and corners spherical patches, each
 * `segments` steps across. Zero segments emits the sharp polyhedron. One segment is a
 * flat chamfer; more segments carry smooth normals.
 */
void emitRounded(Builder& builder, const Polyhedron& shape, float radius, int segments, NaniteMaterial material)
{
    Vec centre{};
    for (const auto& vertex : shape.vertices_) centre = centre + vertex;
    centre = centre * (1.0F / static_cast<float>(shape.vertices_.size()));

    const std::size_t faceCount = shape.faces_.size();
    std::vector<Vec> normals(faceCount);
    std::vector<float> offsets(faceCount);
    float nearest = 1e9F;
    for (std::size_t face = 0; face < faceCount; ++face)
    {
        const auto& loop = shape.faces_[face];
        Vec normal{};
        for (std::size_t index = 0; index < loop.size(); ++index)
        {
            const Vec a = shape.vertices_[loop[index]], b = shape.vertices_[loop[(index + 1) % loop.size()]];
            normal = normal + Vec{(a[1] - b[1]) * (a[2] + b[2]), (a[2] - b[2]) * (a[0] + b[0]), (a[0] - b[0]) * (a[1] + b[1])};
        }
        normal = normalize(normal);
        if (dot(normal, shape.vertices_[loop[0]] - centre) < 0.0F) normal = normal * -1.0F;
        normals[face] = normal;
        offsets[face] = dot(normal, shape.vertices_[loop[0]]);
        nearest = std::min(nearest, offsets[face] - dot(normal, centre));
    }
    if (segments <= 0 || radius <= 0.0F)
    {
        for (std::size_t face = 0; face < faceCount; ++face)
        {
            const auto& loop = shape.faces_[face];
            for (std::size_t index = 1; index + 1 < loop.size(); ++index)
            {
                builder.triangle(shape.vertices_[loop[0]], shape.vertices_[loop[index]], shape.vertices_[loop[index + 1]],
                    normals[face], normals[face], normals[face], normals[face], material);
            }
        }
        return;
    }
    radius = std::min(radius, 0.45F * nearest); // keep the inner polyhedron non-degenerate

    // Faces meeting at each vertex, and the inner vertex where their offset planes meet.
    std::vector<std::vector<int>> incident(shape.vertices_.size());
    for (std::size_t face = 0; face < faceCount; ++face)
    {
        for (const int vertex : shape.faces_[face]) incident[vertex].push_back(static_cast<int>(face));
    }
    std::vector<Vec> inner(shape.vertices_.size());
    for (std::size_t vertex = 0; vertex < shape.vertices_.size(); ++vertex)
    {
        const auto& faces = incident[vertex];
        const Vec n1 = normals[faces[0]], n2 = normals[faces[1]], n3 = normals[faces[2]];
        const float d1 = offsets[faces[0]] - radius, d2 = offsets[faces[1]] - radius, d3 = offsets[faces[2]] - radius;
        inner[vertex] = (cross(n2, n3) * d1 + cross(n3, n1) * d2 + cross(n1, n2) * d3) * (1.0F / dot(n1, cross(n2, n3)));
    }
    const bool smooth = segments > 1;
    const auto blend = [&](Vec a, Vec b, Vec c, float u, float v)
    { return normalize(a * (1.0F - u - v) + b * u + c * v); };
    const auto patch = [&](Vec pa, Vec pb, Vec pc, Vec pd, Vec na, Vec nb, Vec nc, Vec nd)
    {
        // Quad a-b-c-d; outward is the mean of its sphere normals.
        const Vec outward = na + nb + nc + nd;
        if (smooth)
        {
            builder.triangle(pa, pb, pc, na, nb, nc, outward, material);
            builder.triangle(pa, pc, pd, na, nc, nd, outward, material);
        }
        else
        {
            builder.flat(pa, pb, pc, outward, material);
            builder.flat(pa, pc, pd, outward, material);
        }
    };

    for (std::size_t face = 0; face < faceCount; ++face)
    {
        const auto& loop = shape.faces_[face];
        const Vec shift = normals[face] * radius;
        for (std::size_t index = 1; index + 1 < loop.size(); ++index)
        {
            builder.triangle(inner[loop[0]] + shift, inner[loop[index]] + shift, inner[loop[index + 1]] + shift,
                normals[face], normals[face], normals[face], normals[face], material);
        }
    }
    std::vector<std::pair<int, int>> done;
    for (std::size_t face = 0; face < faceCount; ++face)
    {
        const auto& loop = shape.faces_[face];
        for (std::size_t index = 0; index < loop.size(); ++index)
        {
            const int a = loop[index], b = loop[(index + 1) % loop.size()];
            // Each edge once, whichever way its faces list it.
            if (std::ranges::find(done, std::pair{std::min(a, b), std::max(a, b)}) != done.end()) continue;
            done.emplace_back(std::min(a, b), std::max(a, b));
            std::size_t other = faceCount;
            for (std::size_t candidate = 0; candidate < faceCount && other == faceCount; ++candidate)
            {
                if (candidate == face) continue;
                const auto& test = shape.faces_[candidate];
                if (std::ranges::find(test, a) != test.end() && std::ranges::find(test, b) != test.end()) other = candidate;
            }
            for (int step = 0; step < segments; ++step)
            {
                const float t0 = static_cast<float>(step) / segments, t1 = static_cast<float>(step + 1) / segments;
                const Vec n0 = normalize(lerp(normals[face], normals[other], t0));
                const Vec n1 = normalize(lerp(normals[face], normals[other], t1));
                patch(inner[a] + n0 * radius, inner[b] + n0 * radius, inner[b] + n1 * radius, inner[a] + n1 * radius,
                    n0, n0, n1, n1);
            }
        }
    }
    for (std::size_t vertex = 0; vertex < shape.vertices_.size(); ++vertex)
    {
        const Vec na = normals[incident[vertex][0]], nb = normals[incident[vertex][1]], nc = normals[incident[vertex][2]];
        const auto point = [&](int i, int j)
        {
            const Vec normal = blend(na, nb, nc, static_cast<float>(i) / segments, static_cast<float>(j) / segments);
            return std::pair{inner[vertex] + normal * radius, normal};
        };
        for (int i = 0; i < segments; ++i)
        {
            for (int j = 0; i + j < segments; ++j)
            {
                const auto [p0, n0] = point(i, j);
                const auto [p1, n1] = point(i + 1, j);
                const auto [p2, n2] = point(i, j + 1);
                if (smooth) builder.triangle(p0, p1, p2, n0, n1, n2, n0 + n1 + n2, material);
                else builder.flat(p0, p1, p2, n0 + n1 + n2, material);
                if (i + j + 1 < segments)
                {
                    const auto [p3, n3] = point(i + 1, j + 1);
                    if (smooth) builder.triangle(p1, p3, p2, n1, n3, n2, n1 + n3 + n2, material);
                    else builder.flat(p1, p3, p2, n1 + n3 + n2, material);
                }
            }
        }
    }
}

/// The heart: an irregular faceted crystal centred on `centre`, every vertex within `heartRadius`.
void emitHeart(Builder& builder, Vec centre, float turn, NaniteSurface surface)
{
    const int sides = surface == NaniteSurface::Hero ? 8 : surface == NaniteSurface::Bevel ? 6 : 5;
    const bool crowned = surface >= NaniteSurface::Bevel;
    // Fixed irregularity: radii and heights vary per side but never reach heartRadius.
    const auto jitter = [](int index, float salt) { return 0.5F + 0.5F * std::sin(index * 2.399963F + salt); };
    const auto ring = [&](float height, float scale, float salt)
    {
        std::vector<Vec> points;
        for (int index = 0; index < sides; ++index)
        {
            const float angle = turn + 6.2831853F * (index + 0.15F * jitter(index, salt)) / sides;
            const float radius = heartRadius * scale * (0.8F + 0.2F * jitter(index, salt + 1.0F));
            points.push_back(centre + Vec{radius * std::cos(angle), height * heartRadius, radius * std::sin(angle)});
        }
        return points;
    };
    const Vec top = centre + Vec{0.12F * heartRadius * std::cos(turn), 0.97F * heartRadius, 0.12F * heartRadius * std::sin(turn)};
    const Vec bottom = centre + Vec{0.0F, -0.97F * heartRadius, 0.0F};
    const auto waist = ring(0.0F, 0.72F, 0.3F);
    const auto fan = [&](const std::vector<Vec>& loop, Vec apex)
    {
        for (int index = 0; index < sides; ++index)
        {
            const Vec a = loop[index], b = loop[(index + 1) % sides];
            builder.flat(a, b, apex, (a + b + apex) * (1.0F / 3.0F) - centre, NaniteMaterial::Heart);
        }
    };
    const auto band = [&](const std::vector<Vec>& lower, const std::vector<Vec>& upper)
    {
        for (int index = 0; index < sides; ++index)
        {
            const int next = (index + 1) % sides;
            const Vec mid = (lower[index] + lower[next] + upper[index] + upper[next]) * 0.25F - centre;
            builder.flat(lower[index], lower[next], upper[next], mid, NaniteMaterial::Heart);
            builder.flat(lower[index], upper[next], upper[index], mid, NaniteMaterial::Heart);
        }
    };
    fan(waist, bottom);
    if (crowned)
    {
        const auto crown = ring(0.55F, 0.48F, 1.7F);
        band(waist, crown);
        fan(crown, top);
    }
    else
    {
        fan(waist, top);
    }
}

struct Tier
{
    int segments_;
    float scale_; // rounding radius multiplier
};

Tier tier(NaniteSurface surface) noexcept
{
    switch (surface)
    {
    case NaniteSurface::Flat: return {0, 0.0F};
    case NaniteSurface::Chamfer: return {1, 1.0F};
    case NaniteSurface::Bevel: return {3, 1.0F};
    case NaniteSurface::Hero: return {6, 1.15F};
    }
    return {0, 0.0F};
}
}

NaniteMesh buildNanite(NaniteDetail detail, const NanitePosture& posture)
{
    const float stretch = std::clamp(std::isfinite(posture.stretch_) ? posture.stretch_ : 0.0F, 0.0F, 1.0F);
    const float open = std::clamp(std::isfinite(posture.open_) ? posture.open_ : 0.0F, 0.0F, 1.0F);
    const float turn = std::isfinite(posture.heartTurn_) ? std::fmod(posture.heartTurn_, 6.2831853F) : 0.0F;
    const auto structure = detail.structure_;
    const auto [segments, scale] = tier(detail.surface_);

    NaniteMesh mesh;
    // Reserve for this level's size (measured; see the test's table) to build without regrowth.
    static constexpr std::array<std::size_t, 4> perSegment{80, 1300, 3200, 5500};
    Builder builder(mesh, perSegment[static_cast<std::size_t>(structure) & 3] * static_cast<std::size_t>(segments + 1));

    // Shell corners: 0 nose (+Z), 1 rear left, 2 rear right, 3 apex. Only the nose stretches.
    const std::array<Vec, 4> corners{{{0.0F, 0.0F, shellCircum + noseReach * stretch}, {-0.5F, 0.0F, -shellIn},
        {0.5F, 0.0F, -shellIn}, {0.0F, shellHeight, 0.0F}}};
    // The core sits at the compact shell's centroid whatever the stretch: it is rigid.
    const Vec centre{0.0F, shellHeight * 0.25F, 0.0F};
    Vec shellCentre{};
    for (const auto& corner : corners) shellCentre = shellCentre + corner * 0.25F;
    constexpr std::array<std::array<int, 3>, 4> faces{{{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}}};
    constexpr std::array<std::array<int, 2>, 6> edges{{{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}}};
    const auto faceNormal = [&](int face)
    {
        const auto& f = faces[face];
        Vec normal = normalize(cross(corners[f[1]] - corners[f[0]], corners[f[2]] - corners[f[0]]));
        return dot(normal, corners[f[0]] - shellCentre) < 0.0F ? normal * -1.0F : normal;
    };
    const auto edgeOutward = [&](int a, int b)
    {
        Vec sum{};
        for (int face = 0; face < 4; ++face)
        {
            const auto& f = faces[face];
            if (std::ranges::find(f, a) != f.end() && std::ranges::find(f, b) != f.end()) sum = sum + faceNormal(face);
        }
        return normalize(sum);
    };

    if (structure == NaniteStructure::Silhouette)
    {
        emitRounded(builder, tetrahedron(corners[0], corners[1], corners[2], corners[3]), 0.05F * scale, segments,
            NaniteMaterial::Silver);
        return mesh;
    }

    // Hubs: the shell's own corners, cut where the rails begin.
    std::array<Vec, 4> hubInner{};
    for (int corner = 0; corner < 4; ++corner)
    {
        std::array<Vec, 3> cut{};
        int count = 0;
        for (int other = 0; other < 4; ++other)
        {
            if (other != corner) cut[count++] = lerp(corners[corner], corners[other], hubFraction);
        }
        hubInner[corner] = (cut[0] + cut[1] + cut[2]) * (1.0F / 3.0F);
        emitRounded(builder, tetrahedron(corners[corner], cut[0], cut[1], cut[2]), 0.02F * scale, segments,
            NaniteMaterial::Graphite);
    }

    // Rails along the six edges, just inside the shell; Articulated splits each with a joint.
    for (const auto& [a, b] : edges)
    {
        const Vec up = edgeOutward(a, b);
        // Sunk until its outer corners touch the faces, so no part leaves the silhouette
        // tetrahedron however the stretch tilts them.
        const Vec side = normalize(cross(corners[b] - corners[a], up));
        float sinkDepth = railDepth * 0.5F;
        for (int face = 0; face < 4; ++face)
        {
            const auto& f = faces[face];
            if (std::ranges::find(f, a) == f.end() || std::ranges::find(f, b) == f.end()) continue;
            const Vec normal = faceNormal(face);
            sinkDepth = std::max(sinkDepth, railDepth * 0.5F + railHalfWidth * std::abs(dot(normal, side)) / dot(normal, up));
        }
        const Vec sink = up * sinkDepth;
        const Vec from = lerp(corners[a], corners[b], hubFraction * 0.8F) - sink;
        const Vec to = lerp(corners[b], corners[a], hubFraction * 0.8F) - sink;
        const float half = railDepth * 0.5F;
        if (structure == NaniteStructure::Articulated)
        {
            const Vec middle = lerp(from, to, 0.5F);
            const Vec along = normalize(to - from);
            emitRounded(builder, bar(from, middle - along * 0.03F, up, railHalfWidth, half), 0.014F * scale, segments,
                NaniteMaterial::Silver);
            emitRounded(builder, bar(middle + along * 0.03F, to, up, railHalfWidth, half), 0.014F * scale, segments,
                NaniteMaterial::Silver);
            // The joint shares the rail's outer face and runs deeper into the shell.
            const Vec deeper = up * (-half * 0.4F);
            emitRounded(builder, bar(middle - along * 0.045F + deeper, middle + along * 0.045F + deeper, up,
                railHalfWidth, half * 1.4F), 0.012F * scale, segments, NaniteMaterial::Graphite);
        }
        else
        {
            emitRounded(builder, bar(from, to, up, railHalfWidth, half), 0.014F * scale, segments, NaniteMaterial::Silver);
        }
    }

    if (structure == NaniteStructure::Frame)
    {
        // One solid luminous core, the shell's orientation, at the rigid centre.
        const float s = coreEdge;
        emitRounded(builder, tetrahedron(centre + (Vec{0.0F, -shellHeight * 0.25F, shellCircum}) * s,
            centre + (Vec{-0.5F, -shellHeight * 0.25F, -shellIn}) * s, centre + (Vec{0.5F, -shellHeight * 0.25F, -shellIn}) * s,
            centre + (Vec{0.0F, shellHeight * 0.75F, 0.0F}) * s), 0.012F * scale, std::min(segments, 1), NaniteMaterial::Heart);
        return mesh;
    }

    // Face shields: three plates per face around a central window onto the housing. Face 0 is
    // the base: it carries the unit's weight and never opens.
    for (int face = 0; face < 4; ++face)
    {
        const auto& f = faces[face];
        const Vec normal = faceNormal(face);
        const Vec centroid = (corners[f[0]] + corners[f[1]] + corners[f[2]]) * (1.0F / 3.0F);
        for (int side = 0; side < 3; ++side)
        {
            const Vec a = corners[f[side]], b = corners[f[(side + 1) % 3]];
            Vec outerA = lerp(a, centroid, plateRim), outerB = lerp(b, centroid, plateRim);
            Vec innerA = lerp(a, centroid, plateWindow), innerB = lerp(b, centroid, plateWindow);
            const Vec recess = normal * -plateRecess;
            outerA = outerA + recess; outerB = outerB + recess; innerA = innerA + recess; innerB = innerB + recess;
            if (structure == NaniteStructure::Articulated)
            {
                // Separate plates: pull each end in along the edge, then hinge about the outer edge.
                const Vec along = normalize(outerB - outerA);
                outerA = outerA + along * plateGap; outerB = outerB - along * plateGap;
                const Vec innerAlong = normalize(innerB - innerA);
                innerA = innerA + innerAlong * plateGap; innerB = innerB - innerAlong * plateGap;
                float angle = face == 0 ? 0.0F : open * plateHinge; // the base face rests on the ground
                if (dot(rotateAbout(innerA, outerA, along, angle) - innerA, normal) < 0.0F) angle = -angle;
                innerA = rotateAbout(innerA, outerA, along, angle);
                innerB = rotateAbout(innerB, outerA, along, angle);
                if (face != 0)
                {
                    const Vec lift = normal * (open * plateLift * 0.6F);
                    outerA = outerA + lift; outerB = outerB + lift; innerA = innerA + lift; innerB = innerB + lift;
                }
            }
            else if (face != 0)
            {
                const Vec lift = normal * (open * plateLift);
                outerA = outerA + lift; outerB = outerB + lift; innerA = innerA + lift; innerB = innerB + lift;
            }
            const Vec plateNormal = normalize(cross(outerB - outerA, innerA - outerA));
            const Vec depth = (dot(plateNormal, normal) > 0.0F ? plateNormal : plateNormal * -1.0F) * -plateThickness;
            emitRounded(builder, prism({outerA, outerB, innerB, innerA}, depth), 0.012F * scale, segments,
                NaniteMaterial::Silver);
        }
    }

    // Containment housing: rigid, the shell's orientation, its vertices toward the hubs.
    std::array<Vec, 4> housing{};
    {
        const std::array<Vec, 4> unit{{{0.0F, -shellHeight * 0.25F, shellCircum}, {-0.5F, -shellHeight * 0.25F, -shellIn},
            {0.5F, -shellHeight * 0.25F, -shellIn}, {0.0F, shellHeight * 0.75F, 0.0F}}};
        for (int corner = 0; corner < 4; ++corner) housing[corner] = centre + unit[corner] * housingEdge;
    }
    emitRounded(builder, tetrahedron(housing[0], housing[1], housing[2], housing[3]), 0.008F * scale,
        std::min(segments, 2), NaniteMaterial::Housing);
    if (structure == NaniteStructure::Articulated)
    {
        // Graphite housing edges and the cradle struts that hold the housing to the hubs.
        for (const auto& [a, b] : edges)
        {
            const Vec mid = lerp(housing[a], housing[b], 0.5F);
            emitRounded(builder, bar(housing[a], housing[b], normalize(mid - centre), 0.009F, 0.009F), 0.004F * scale,
                std::min(segments, 2), NaniteMaterial::Graphite);
        }
        for (int corner = 0; corner < 4; ++corner)
        {
            const Vec along = normalize(hubInner[corner] - housing[corner]);
            const Vec up = std::abs(along[1]) > 0.9F ? Vec{0.0F, 0.0F, 1.0F} : Vec{0.0F, 1.0F, 0.0F};
            emitRounded(builder, bar(housing[corner], hubInner[corner], up, 0.014F, 0.014F), 0.005F * scale,
                std::min(segments, 2), NaniteMaterial::Graphite);
        }
    }
    emitHeart(builder, centre, turn, detail.surface_);
    return mesh;
}

std::span<const NaniteDevicePreset> naniteDevicePresets() noexcept
{
    using S = NaniteStructure;
    using P = NaniteSurface;
    static constexpr std::array<NaniteDevicePreset, 5> presets{{
        {"Mobile (Android / WebGL)", {S::Shielded, P::Chamfer}, {S::Silhouette, P::Flat}},
        {"Handheld console (Switch class)", {S::Shielded, P::Bevel}, {S::Frame, P::Flat}},
        {"High-end mobile (iOS A15+ class)", {S::Articulated, P::Bevel}, {S::Frame, P::Flat}},
        {"Desktop (PC / console)", {S::Articulated, P::Bevel}, {S::Frame, P::Chamfer}},
        {"Desktop 4K hero", {S::Articulated, P::Hero}, {S::Shielded, P::Chamfer}},
    }};
    return presets;
}
}
