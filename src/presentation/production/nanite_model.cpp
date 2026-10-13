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
constexpr float plateWindow = 0.6F;       // plate inner edge (window), fraction toward the centroid
constexpr float plateRecess = 0.012F;
constexpr float plateThickness = 0.04F;
constexpr float plateGap = 0.035F;         // Articulated: gap between neighbouring plates, in edge lengths
constexpr float plateLift = 0.07F;         // Shielded: shield lift at full open
constexpr float plateHinge = 0.4F;         // Articulated: hinge angle at full open, radians
constexpr float coreEdge = 0.42F;          // Frame: solid core
constexpr float housingEdge = 0.6F;        // Shielded+: containment housing
constexpr float silhouetteInset = 0.93F;  // Silhouette: outline scale toward the shell centre
constexpr float heartRadius = 0.094F;      // inside the housing's inscribed sphere (0.122)

/// Deterministic value in [0,1) for a unit variant and a per-feature salt.
float noise(std::uint32_t seed, std::uint32_t salt) noexcept
{
    std::uint32_t h = seed * 0x9E3779B9U ^ (salt + 0x7F4A7C15U) * 0x85EBCA6BU;
    h ^= h >> 16; h *= 0x7FEB352DU; h ^= h >> 15; h *= 0x846CA68BU; h ^= h >> 16;
    return static_cast<float>(h >> 8) * (1.0F / 16777216.0F);
}

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

/** An opaque window set into a triangular face: graphite caps at the corners (where higher
 * levels put their hubs), a silver rim, graphite walls sloping into the body and, at the
 * bottom, an inverted luminous facet, so the heart reads with no transparency. `window` is
 * the opening's size as a fraction of the face; `cap` the corner caps' share of each edge.
 */
void emitWindowedFace(Builder& builder, const std::array<Vec, 3>& face, Vec normal, float window, float depth,
    NaniteMaterial rim, float cap)
{
    const Vec centroid = (face[0] + face[1] + face[2]) * (1.0F / 3.0F);
    std::array<Vec, 3> top{}, bottom{}, towardNext{}, towardPrevious{};
    for (int i = 0; i < 3; ++i)
    {
        top[i] = centroid + (face[i] - centroid) * window;
        bottom[i] = centroid + (centroid - face[i]) * (window * 0.45F) - normal * depth; // opposite top[i]
        towardNext[i] = lerp(face[i], face[(i + 1) % 3], cap);
        towardPrevious[i] = lerp(face[i], face[(i + 2) % 3], cap);
    }
    for (int i = 0; i < 3; ++i)
    {
        const int j = (i + 1) % 3, k = (i + 2) % 3;
        if (cap > 0.0F)
        {
            builder.flat(face[i], towardNext[i], towardPrevious[i], normal, NaniteMaterial::Graphite);
            builder.flat(towardPrevious[i], towardNext[i], top[i], normal, rim);
            builder.flat(towardNext[i], towardPrevious[j], top[j], normal, rim);
            builder.flat(towardNext[i], top[j], top[i], normal, rim);
        }
        else
        {
            builder.flat(face[i], face[j], top[j], normal, rim);
            builder.flat(face[i], top[j], top[i], normal, rim);
        }
        builder.flat(top[i], top[j], bottom[k], normal, NaniteMaterial::Graphite);
        builder.flat(bottom[k], top[j], bottom[i], normal, NaniteMaterial::Graphite);
    }
    builder.flat(bottom[0], bottom[1], bottom[2], normal, NaniteMaterial::Heart);
}

/** Emits a polyhedron rounded by `radius` without changing its outer extent: faces are
 * offset inward, edges become cylindrical strips and corners spherical patches, each
 * `segments` steps across. Zero segments emits the sharp polyhedron. One segment is a
 * flat chamfer; more segments carry smooth normals. A nonzero `window` sets an opaque
 * window (emitWindowedFace) into every triangular face.
 */
void emitRounded(Builder& builder, const Polyhedron& shape, float radius, int segments, NaniteMaterial material,
    float window = 0.0F, float windowDepth = 0.0F, float windowCap = 0.0F)
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
            if (window > 0.0F && loop.size() == 3)
            {
                emitWindowedFace(builder, {shape.vertices_[loop[0]], shape.vertices_[loop[1]], shape.vertices_[loop[2]]},
                    normals[face], window, windowDepth, material, windowCap);
                continue;
            }
            for (std::size_t index = 1; index + 1 < loop.size(); ++index)
            {
                builder.triangle(shape.vertices_[loop[0]], shape.vertices_[loop[index]], shape.vertices_[loop[index + 1]],
                    normals[face], normals[face], normals[face], normals[face], material);
            }
        }
        return;
    }
    // Keep the inner polyhedron non-degenerate: thinner than the part, shorter than its edges.
    float shortest = 1e9F;
    for (const auto& loop : shape.faces_)
    {
        for (std::size_t index = 0; index < loop.size(); ++index)
        {
            const Vec edge = shape.vertices_[loop[(index + 1) % loop.size()]] - shape.vertices_[loop[index]];
            shortest = std::min(shortest, std::sqrt(dot(edge, edge)));
        }
    }
    radius = std::min({radius, 0.45F * nearest, 0.5F * shortest});

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
    const auto patch = [&](Vec pa, Vec pb, Vec pc, Vec pd, Vec na, Vec nb, Vec nc, Vec nd, NaniteMaterial patchMaterial)
    {
        // Quad a-b-c-d; outward is the mean of its sphere normals.
        const Vec outward = na + nb + nc + nd;
        if (smooth)
        {
            builder.triangle(pa, pb, pc, na, nb, nc, outward, patchMaterial);
            builder.triangle(pa, pc, pd, na, nc, nd, outward, patchMaterial);
        }
        else
        {
            builder.flat(pa, pb, pc, outward, patchMaterial);
            builder.flat(pa, pc, pd, outward, patchMaterial);
        }
    };

    for (std::size_t face = 0; face < faceCount; ++face)
    {
        const auto& loop = shape.faces_[face];
        const Vec shift = normals[face] * radius;
        if (window > 0.0F && loop.size() == 3)
        {
            emitWindowedFace(builder, {inner[loop[0]] + shift, inner[loop[1]] + shift, inner[loop[2]] + shift},
                normals[face], window, windowDepth, material, windowCap);
            continue;
        }
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
            // Windowed faces split their edges where the corner caps end; the strip follows,
            // graphite beside the caps, so the edge has no T-junction.
            const std::array<float, 4> spans{0.0F, windowCap, 1.0F - windowCap, 1.0F};
            for (std::size_t span = 0; span < 3; ++span)
            {
                if (windowCap <= 0.0F && span != 1) continue;
                const float u0 = windowCap <= 0.0F ? 0.0F : spans[span], u1 = windowCap <= 0.0F ? 1.0F : spans[span + 1];
                const Vec from = lerp(inner[a], inner[b], u0), to = lerp(inner[a], inner[b], u1);
                const NaniteMaterial stripMaterial = span == 1 ? material : NaniteMaterial::Graphite;
                for (int step = 0; step < segments; ++step)
                {
                    const float t0 = static_cast<float>(step) / segments, t1 = static_cast<float>(step + 1) / segments;
                    const Vec n0 = normalize(lerp(normals[face], normals[other], t0));
                    const Vec n1 = normalize(lerp(normals[face], normals[other], t1));
                    patch(from + n0 * radius, to + n0 * radius, to + n1 * radius, from + n1 * radius, n0, n0, n1, n1,
                        stripMaterial);
                }
            }
        }
    }
    // Corner patches belong to the corner caps on windowed faces.
    const NaniteMaterial cornerMaterial = windowCap > 0.0F ? NaniteMaterial::Graphite : material;
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
                if (smooth) builder.triangle(p0, p1, p2, n0, n1, n2, n0 + n1 + n2, cornerMaterial);
                else builder.flat(p0, p1, p2, n0 + n1 + n2, cornerMaterial);
                if (i + j + 1 < segments)
                {
                    const auto [p3, n3] = point(i + 1, j + 1);
                    if (smooth) builder.triangle(p1, p3, p2, n1, n3, n2, n1 + n3 + n2, cornerMaterial);
                    else builder.flat(p1, p3, p2, n1 + n3 + n2, cornerMaterial);
                }
            }
        }
    }
}

/// The heart: an irregular faceted crystal centred on `centre`, every vertex within `heartRadius`.
void emitHeart(Builder& builder, Vec centre, float turn, NaniteSurface surface, std::uint32_t variant)
{
    const int sides = surface == NaniteSurface::Hero ? 8 : surface == NaniteSurface::Bevel ? 6 : 5;
    const bool crowned = surface >= NaniteSurface::Bevel;
    // Fixed irregularity: radii and heights vary per side but never reach heartRadius.
    const auto jitter = [&](int index, float salt)
    { return noise(variant, 1000U + static_cast<std::uint32_t>(index * 7 + static_cast<int>(salt * 10.0F))); };
    const auto ring = [&](float height, float scale, float salt)
    {
        std::vector<Vec> points;
        for (int index = 0; index < sides; ++index)
        {
            const float angle = turn + 6.2831853F * (index + 0.15F * jitter(index, salt)) / sides;
            const float radius = heartRadius * scale * (0.62F + 0.38F * jitter(index, salt + 1.0F));
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

NaniteMesh buildNanite(NaniteDetail detail, const NanitePosture& posture, std::uint32_t variant)
{
    const float stretch = std::clamp(std::isfinite(posture.stretch_) ? posture.stretch_ : 0.0F, 0.0F, 1.0F);
    const float open = std::clamp(std::isfinite(posture.open_) ? posture.open_ : 0.0F, 0.0F, 1.0F);
    const float turn = std::isfinite(posture.heartTurn_) ? std::fmod(posture.heartTurn_, 6.2831853F) : 0.0F;
    const auto structure = detail.structure_;
    const bool articulated = structure == NaniteStructure::Articulated;
    const auto [segments, scale] = tier(detail.surface_);
    // Irregularity is drawn per feature in a fixed order, so a variant always builds the same unit.
    std::uint32_t salt = 0;
    const auto random = [&](float low, float high) { return low + (high - low) * noise(variant, salt++); };

    NaniteMesh mesh;
    // Reserve for this level's size (measured; see the test's table) to build without regrowth.
    static constexpr std::array<std::size_t, 4> perSegment{80, 1300, 4200, 9000};
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
    const auto sharesEdge = [&](int face, int a, int b)
    {
        const auto& f = faces[face];
        return std::ranges::find(f, a) != f.end() && std::ranges::find(f, b) != f.end();
    };
    const auto edgeOutward = [&](int a, int b)
    {
        Vec sum{};
        for (int face = 0; face < 4; ++face)
        {
            if (sharesEdge(face, a, b)) sum = sum + faceNormal(face);
        }
        return normalize(sum);
    };

    if (structure == NaniteStructure::Silhouette)
    {
        // One shell; each face carries an opaque window down to a luminous facet. It is drawn
        // slightly inside the full tetrahedron: the higher levels' sunk rails and rounded hubs
        // inset their visible outline, and a full-size silhouette covers more pixels than they do.
        std::array<Vec, 4> inset{};
        for (int corner = 0; corner < 4; ++corner) inset[corner] = lerp(shellCentre, corners[corner], silhouetteInset);
        emitRounded(builder, tetrahedron(inset[0], inset[1], inset[2], inset[3]), 0.025F * scale, segments,
            NaniteMaterial::Silver, 0.26F, 0.04F, hubFraction);
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

    // Rails along the six edges, just inside the shell, each a little different in width.
    // Articulated rails split at an uneven joint and carry graphite collars.
    for (const auto& [a, b] : edges)
    {
        const Vec up = edgeOutward(a, b);
        const Vec side = normalize(cross(corners[b] - corners[a], up));
        const float halfWidth = railHalfWidth * random(0.75F, 1.0F);
        const float half = railDepth * 0.5F;
        /// A bar on this edge between edge fractions t0 and t1, sunk until its outer corners
        /// touch the faces, so no part leaves the silhouette however the stretch tilts them.
        const auto edgeBar = [&](float t0, float t1, float barWidth, float barDepth, float radius, NaniteMaterial material,
            int maxSegments = 6)
        {
            float sinkDepth = half;
            for (int face = 0; face < 4; ++face)
            {
                if (!sharesEdge(face, a, b)) continue;
                const Vec normal = faceNormal(face);
                sinkDepth = std::max(sinkDepth, half + barWidth * std::abs(dot(normal, side)) / dot(normal, up));
            }
            // A deeper bar keeps the rail's outer face and grows into the shell.
            const Vec centreLine = up * -(sinkDepth + (barDepth - half));
            emitRounded(builder, bar(lerp(corners[a], corners[b], t0) + centreLine, lerp(corners[a], corners[b], t1) + centreLine,
                up, barWidth, barDepth), radius * scale, std::min(segments, maxSegments), material);
        };
        const float start = hubFraction * 0.8F, end = 1.0F - hubFraction * 0.8F;
        if (articulated)
        {
            const float joint = random(0.42F, 0.58F);
            edgeBar(start, joint - 0.03F, halfWidth, half, 0.014F, NaniteMaterial::Silver);
            edgeBar(joint + 0.03F, end, halfWidth, half, 0.014F, NaniteMaterial::Silver);
            edgeBar(joint - 0.045F, joint + 0.045F, halfWidth, half * 1.4F, 0.012F, NaniteMaterial::Graphite);
            for (const float at : {random(0.22F, 0.32F), random(0.68F, 0.78F)})
            {
                edgeBar(at - 0.016F, at + 0.016F, halfWidth, half * 1.6F, 0.006F, NaniteMaterial::Graphite, 1);
            }
        }
        else
        {
            edgeBar(start, end, halfWidth, half, 0.014F, NaniteMaterial::Silver);
        }
    }

    if (structure == NaniteStructure::Frame)
    {
        // One opaque core at the rigid centre: a dark housing whose faces open onto
        // luminous facets, the frame-level stand-in for housing and heart.
        const float s = coreEdge;
        emitRounded(builder, tetrahedron(centre + (Vec{0.0F, -shellHeight * 0.25F, shellCircum}) * s,
            centre + (Vec{-0.5F, -shellHeight * 0.25F, -shellIn}) * s, centre + (Vec{0.5F, -shellHeight * 0.25F, -shellIn}) * s,
            centre + (Vec{0.0F, shellHeight * 0.75F, 0.0F}) * s), 0.012F * scale, std::min(segments, 1), NaniteMaterial::Graphite,
            0.62F, 0.03F, 0.0F);
        return mesh;
    }

    // Face shields: plates around a central window onto the housing, uneven in reach,
    // thickness and tilt, with clipped corners. Articulated splits each into two hinged plates
    // and stacks a smaller armour plate on one. Face 0 is the base: it carries the unit and
    // never opens.
    const float recessDepth = articulated ? 0.02F : plateRecess;
    for (int face = 0; face < 4; ++face)
    {
        const auto& f = faces[face];
        const Vec normal = faceNormal(face);
        const Vec centroid = (corners[f[0]] + corners[f[1]] + corners[f[2]]) * (1.0F / 3.0F);
        for (int side = 0; side < 3; ++side)
        {
            const Vec a = corners[f[side]], b = corners[f[(side + 1) % 3]];
            const Vec recess = normal * -recessDepth;
            // Keep the plate's full depth behind the neighbouring face: the sharper the
            // dihedral at this edge (a stretched nose), the further in the plate starts.
            int neighbour = 0;
            while (neighbour == face || !sharesEdge(neighbour, f[side], f[(side + 1) % 3])) ++neighbour;
            const float cosine = std::clamp(-dot(normal, faceNormal(neighbour)), -0.99F, 0.99F);
            const float dihedral = std::acos(cosine);
            const Vec edgeDirection = normalize(b - a);
            const Vec toCentroid = (centroid - a) - edgeDirection * dot(centroid - a, edgeDirection);
            const float clearance = (recessDepth + 0.075F) / std::tan(dihedral) + 0.012F;
            const float rimMinimum = clearance / std::sqrt(dot(toCentroid, toCentroid));
            const Vec outerA = lerp(a, centroid, std::max(plateRim + random(-0.02F, 0.02F), rimMinimum)) + recess;
            const Vec outerB = lerp(b, centroid, std::max(plateRim + random(-0.02F, 0.02F), rimMinimum)) + recess;
            const Vec innerA = lerp(a, centroid, plateWindow + random(-0.06F, 0.05F)) + recess;
            const Vec innerB = lerp(b, centroid, plateWindow + random(-0.06F, 0.05F)) + recess;
            const float split = articulated ? random(0.38F, 0.62F) : 1.0F;
            const float gap = articulated ? 0.035F : 0.0F;
            const std::array<std::array<float, 2>, 2> pieces{{{gap, split - gap * 0.5F}, {split + gap * 0.5F, 1.0F - gap}}};
            const std::size_t pieceCount = articulated ? 2 : 1;
            const std::size_t stacked = split > 0.5F ? 0 : 1; // the longer piece
            for (std::size_t piece = 0; piece < pieceCount; ++piece)
            {
                const auto [t0, t1] = articulated ? pieces[piece] : std::array<float, 2>{0.0F, 1.0F};
                const Vec o0 = lerp(outerA, outerB, t0), o1 = lerp(outerA, outerB, t1);
                const Vec i0 = lerp(innerA, innerB, t0), i1 = lerp(innerA, innerB, t1);
                // Clip the outer corners by uneven cuts: a jagged, faceted outline that stays convex.
                const std::array<Vec, 4> quad{o0, o1, i1, i0};
                std::vector<Vec> outline;
                for (int corner = 0; corner < 4; ++corner)
                {
                    const Vec here = quad[corner], previous = quad[(corner + 3) % 4], next = quad[(corner + 1) % 4];
                    if (corner >= 2)
                    {
                        outline.push_back(here);
                        continue;
                    }
                    const float cut = random(0.03F, 0.065F);
                    const float limit = 0.3F * std::min(std::sqrt(dot(previous - here, previous - here)),
                        std::sqrt(dot(next - here, next - here)));
                    outline.push_back(here + normalize(previous - here) * std::min(cut, limit));
                    outline.push_back(here + normalize(next - here) * std::min(cut, limit));
                }
                // Pose: a slight inward tilt about the outer edge, then the opening.
                const Vec hingeAxis = normalize(o1 - o0);
                float tilt = random(0.0F, 0.09F);
                if (dot(rotateAbout(i0, o0, hingeAxis, tilt) - i0, normal) > 0.0F) tilt = -tilt;
                float hinge = articulated && face != 0 ? open * plateHinge : 0.0F;
                if (dot(rotateAbout(i0, o0, hingeAxis, hinge) - i0, normal) < 0.0F) hinge = -hinge;
                const Vec lift = face == 0 ? Vec{} : normal * (open * plateLift * (articulated ? 0.6F : 1.0F));
                const auto pose = [&](Vec point) { return rotateAbout(rotateAbout(point, o0, hingeAxis, tilt), o0, hingeAxis, hinge) + lift; };
                const float thickness = random(0.032F, 0.052F);
                std::vector<Vec> posed;
                for (const auto& point : outline) posed.push_back(pose(point));
                const Vec plateNormal = normalize(cross(posed[2] - posed[0], posed[5] - posed[0]));
                const Vec outward = dot(plateNormal, normal) > 0.0F ? plateNormal : plateNormal * -1.0F;
                emitRounded(builder, prism(posed, outward * -thickness), 0.012F * scale, segments, NaniteMaterial::Silver);
                if (articulated && face != 0 && piece == stacked)
                {
                    // A smaller, thinner armour plate stacked on top, still under the face plane.
                    Vec middle{};
                    for (const auto& point : outline) middle = middle + point * (1.0F / static_cast<float>(outline.size()));
                    const float shrink = random(0.5F, 0.68F);
                    const Vec offset = normal * 0.016F + normalize(i0 - o0) * random(0.006F, 0.02F); // toward the window
                    std::vector<Vec> layer;
                    for (const auto& point : outline) layer.push_back(pose(lerp(middle, point, shrink) + offset));
                    emitRounded(builder, prism(layer, outward * -0.016F), 0.006F * scale, std::min(segments, 2),
                        NaniteMaterial::Silver);
                }
            }
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
    if (articulated)
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
    emitHeart(builder, centre, turn, detail.surface_, variant);
    return mesh;
}

std::span<const NaniteLodStep> naniteLodLadder() noexcept
{
    using S = NaniteStructure;
    using P = NaniteSurface;
    // Switch sizes from the lab's comparison of neighbouring rungs (mean colour difference
    // over the unit, four views, 4x4 supersampled). A rung without a fade differs by at most
    // 3% where it starts; the two fading steps add parts and differ by 7-8% where they start.
    static constexpr std::array<NaniteLodStep, 6> ladder{{
        {{S::Silhouette, P::Flat}, 0.0F, false},
        {{S::Shielded, P::Flat}, 10.0F, true},
        {{S::Shielded, P::Chamfer}, 14.0F, false},
        {{S::Shielded, P::Bevel}, 40.0F, false},
        {{S::Articulated, P::Bevel}, 64.0F, true},
        {{S::Articulated, P::Hero}, 160.0F, false},
    }};
    return ladder;
}

std::size_t selectNaniteLod(float pixels, NaniteDetail cap, std::size_t current) noexcept
{
    const auto ladder = naniteLodLadder();
    if (!std::isfinite(pixels)) pixels = 0.0F;
    std::size_t highest = 0; // the cap cuts the ladder at a prefix
    while (highest + 1 < ladder.size() && ladder[highest + 1].detail_.structure_ <= cap.structure_
        && ladder[highest + 1].detail_.surface_ <= cap.surface_)
    {
        ++highest;
    }
    std::size_t target = 0;
    while (target < highest && pixels >= ladder[target + 1].minPixels_) ++target;
    if (current >= ladder.size()) return target;
    current = std::min(current, highest);
    if (target >= current) return target;
    while (current > target && pixels < ladder[current].minPixels_ * (1.0F - naniteLodHysteresis)) --current;
    return current;
}

std::span<const NaniteDevicePreset> naniteDevicePresets() noexcept
{
    using S = NaniteStructure;
    using P = NaniteSurface;
    static constexpr std::array<NaniteDevicePreset, 5> presets{{
        {"Mobile (Android / WebGL)", {S::Shielded, P::Flat}, {S::Silhouette, P::Flat}},
        {"Handheld console (Switch class)", {S::Shielded, P::Chamfer}, {S::Silhouette, P::Flat}},
        {"High-end mobile (iOS A15+ class)", {S::Shielded, P::Bevel}, {S::Shielded, P::Flat}},
        {"Desktop (PC / console)", {S::Articulated, P::Bevel}, {S::Shielded, P::Chamfer}},
        {"Desktop 4K hero", {S::Articulated, P::Hero}, {S::Shielded, P::Bevel}},
    }};
    return presets;
}
}
