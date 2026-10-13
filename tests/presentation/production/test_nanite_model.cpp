#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>

#include "crucible/presentation/production/nanite_model.hpp"

namespace
{
using namespace crucible::presentation::production;
using Vec = std::array<float, 3>;

constexpr std::array structures{NaniteStructure::Silhouette, NaniteStructure::Frame, NaniteStructure::Shielded,
    NaniteStructure::Articulated};
constexpr std::array surfaces{NaniteSurface::Flat, NaniteSurface::Chamfer, NaniteSurface::Bevel, NaniteSurface::Hero};
constexpr std::array postures{NanitePosture{}, NanitePosture{1.0F, 0.0F, 0.0F}, NanitePosture{0.0F, 1.0F, 0.0F},
    NanitePosture{0.6F, 0.7F, 2.1F}};

void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

std::string name(NaniteDetail detail)
{
    return "S" + std::to_string(static_cast<int>(detail.structure_)) + "P" + std::to_string(static_cast<int>(detail.surface_));
}

Vec sub(Vec a, Vec b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
float dot(Vec a, Vec b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vec cross(Vec a, Vec b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }

/// Every triangle valid, every part closed (position-welded directed edges pair up) and wound outward.
void checkWellFormed(const NaniteMesh& mesh, const std::string& label)
{
    require(!mesh.indices_.empty() && mesh.indices_.size() % 3 == 0, label + ": triangle list");
    require(mesh.vertices_.size() < 65536, label + ": exceeds 16-bit index range");
    for (const auto& vertex : mesh.vertices_)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            require(std::isfinite(vertex.position_[axis]) && std::isfinite(vertex.normal_[axis]), label + ": nonfinite");
        }
        require(std::abs(dot(vertex.normal_, vertex.normal_) - 1.0F) < 1e-3F, label + ": normal not unit length");
        require(vertex.position_[1] > -1e-4F, label + ": below the ground plane");
    }
    std::map<std::tuple<long, long, long>, int> ids;
    const auto id = [&](std::uint32_t index)
    {
        const auto& p = mesh.vertices_.at(index).position_;
        const auto key = std::tuple{std::lround(p[0] * 1e4), std::lround(p[1] * 1e4), std::lround(p[2] * 1e4)};
        return ids.try_emplace(key, static_cast<int>(ids.size())).first->second;
    };
    std::map<std::pair<int, int>, int> balance;
    double volume = 0.0;
    for (std::size_t index = 0; index < mesh.indices_.size(); index += 3)
    {
        const auto a = mesh.indices_[index], b = mesh.indices_[index + 1], c = mesh.indices_[index + 2];
        require(a < mesh.vertices_.size() && b < mesh.vertices_.size() && c < mesh.vertices_.size(), label + ": index range");
        const auto& pa = mesh.vertices_[a].position_;
        const auto& pb = mesh.vertices_[b].position_;
        const auto& pc = mesh.vertices_[c].position_;
        const Vec face = cross(sub(pb, pa), sub(pc, pa));
        // A smooth vertex normal never faces away from its own triangle.
        for (const auto vertex : {a, b, c})
        {
            require(dot(face, mesh.vertices_[vertex].normal_) > -1e-7F, label + ": normal opposes its winding");
        }
        volume += dot(pa, cross(pb, pc)) / 6.0;
        const std::array corner{id(a), id(b), id(c)};
        for (int edge = 0; edge < 3; ++edge)
        {
            const int from = corner[edge], to = corner[(edge + 1) % 3];
            if (from == to) continue; // degenerate sliver at a rounding pole
            if (from < to) ++balance[{from, to}];
            else --balance[{to, from}];
        }
    }
    for (const auto& [edge, count] : balance) require(count == 0, label + ": open or inconsistently wound part");
    require(volume > 0.0, label + ": parts wound inward");
}

std::size_t triangles(NaniteDetail detail) { return buildNanite(detail).indices_.size() / 3; }

std::vector<Vec> positionsOf(const NaniteMesh& mesh, NaniteMaterial material)
{
    std::vector<Vec> result;
    for (const auto& vertex : mesh.vertices_)
    {
        if (vertex.material_ == material) result.push_back(vertex.position_);
    }
    return result;
}
}

int main()
{
    try
    {
        for (const auto structure : structures)
        {
            for (const auto surface : surfaces)
            {
                for (std::size_t pose = 0; pose < postures.size(); ++pose)
                {
                    const NaniteDetail detail{structure, surface};
                    checkWellFormed(buildNanite(detail, postures[pose]), name(detail) + " pose " + std::to_string(pose));
                }
            }
        }
        require(triangles({NaniteStructure::Silhouette, NaniteSurface::Flat}) == 4,
            "The lowest detail on both axes is exactly one tetrahedron");

        // Each axis adds triangles on its own, whatever the other axis holds.
        for (std::size_t s = 0; s < structures.size(); ++s)
        {
            for (std::size_t p = 0; p < surfaces.size(); ++p)
            {
                const auto count = triangles({structures[s], surfaces[p]});
                if (s > 0) require(count > triangles({structures[s - 1], surfaces[p]}), "Structure axis must add detail");
                if (p > 0) require(count > triangles({structures[s], surfaces[p - 1]}), "Surface axis must add detail");
            }
        }

        // Materials appear by structure level: silhouette silver only; frame adds hubs and a core.
        const auto materials = [](NaniteDetail detail)
        {
            std::array<bool, 4> seen{};
            for (const auto& vertex : buildNanite(detail).vertices_) seen[static_cast<int>(vertex.material_)] = true;
            return seen;
        };
        require(materials({NaniteStructure::Silhouette, NaniteSurface::Bevel}) == std::array{true, false, false, false},
            "Silhouette is one silver shell");
        require(materials({NaniteStructure::Frame, NaniteSurface::Flat}) == std::array{true, true, false, true},
            "Frame has silver rails, graphite hubs and a luminous core");
        require(materials({NaniteStructure::Shielded, NaniteSurface::Flat}) == std::array{true, true, true, true},
            "Shielded adds the housing around a separate heart");

        // Core, housing and heart are rigid: stretch and open never move them.
        for (const auto structure : {NaniteStructure::Frame, NaniteStructure::Shielded, NaniteStructure::Articulated})
        {
            const NaniteDetail detail{structure, NaniteSurface::Bevel};
            const auto rest = buildNanite(detail);
            const auto moved = buildNanite(detail, {1.0F, 1.0F, 0.0F});
            for (const auto material : {NaniteMaterial::Housing, NaniteMaterial::Heart})
            {
                require(positionsOf(rest, material) == positionsOf(moved, material), name(detail) + ": core must stay rigid");
            }
            require(positionsOf(rest, NaniteMaterial::Silver) != positionsOf(moved, NaniteMaterial::Silver),
                name(detail) + ": stretch and open must move the shell");
        }
        require(positionsOf(buildNanite({NaniteStructure::Frame, NaniteSurface::Flat}), NaniteMaterial::Silver)
            == positionsOf(buildNanite({NaniteStructure::Frame, NaniteSurface::Flat}, {0.0F, 1.0F, 0.0F}), NaniteMaterial::Silver),
            "A frame has no shields to open");

        // The heart stays clear inside the housing's inscribed sphere for any turn; the
        // housing stays inside the compact shell.
        const Vec centre{0.0F, 0.81649658F * 0.25F, 0.0F};
        for (const float turn : {0.0F, 0.7F, 2.5F, 5.9F, 40.0F})
        {
            for (const auto& p : positionsOf(buildNanite({NaniteStructure::Articulated, NaniteSurface::Hero}, {0, 0, turn}),
                     NaniteMaterial::Heart))
            {
                const Vec d = sub(p, centre);
                require(std::sqrt(dot(d, d)) < 0.122F * 0.8F, "Heart must keep clearance inside its housing");
            }
        }
        const auto shell = buildNanite({NaniteStructure::Silhouette, NaniteSurface::Flat});
        for (const auto& p : positionsOf(buildNanite({NaniteStructure::Shielded, NaniteSurface::Hero}), NaniteMaterial::Housing))
        {
            for (std::size_t index = 0; index < shell.indices_.size(); index += 3)
            {
                const auto& a = shell.vertices_[shell.indices_[index]].position_;
                const Vec normal = cross(sub(shell.vertices_[shell.indices_[index + 1]].position_, a),
                    sub(shell.vertices_[shell.indices_[index + 2]].position_, a));
                require(dot(sub(p, a), normal) < 0.0F, "Housing must stay inside the shell");
            }
        }

        // Identical inputs give identical bytes; nonfinite postures fall back to rest.
        const auto first = buildNanite({NaniteStructure::Articulated, NaniteSurface::Hero}, {0.3F, 0.4F, 1.0F});
        const auto second = buildNanite({NaniteStructure::Articulated, NaniteSurface::Hero}, {0.3F, 0.4F, 1.0F});
        bool same = first.indices_ == second.indices_ && first.vertices_.size() == second.vertices_.size();
        for (std::size_t index = 0; same && index < first.vertices_.size(); ++index)
        {
            const auto& a = first.vertices_[index];
            const auto& b = second.vertices_[index];
            same = std::memcmp(a.position_.data(), b.position_.data(), sizeof(a.position_)) == 0
                && std::memcmp(a.normal_.data(), b.normal_.data(), sizeof(a.normal_)) == 0 && a.material_ == b.material_;
        }
        require(same, "Generation must be deterministic");
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const auto clamped = buildNanite({NaniteStructure::Articulated, NaniteSurface::Chamfer}, {nan, nan, nan});
        const auto rest = buildNanite({NaniteStructure::Articulated, NaniteSurface::Chamfer});
        require(clamped.indices_ == rest.indices_ && clamped.vertices_.size() == rest.vertices_.size(),
            "Nonfinite posture must build the rest pose");

        // Presets rise in hero detail and never give the swarm more than the hero.
        const auto presets = naniteDevicePresets();
        require(presets.size() == 5, "Five proposed hardware classes");
        std::size_t previous = 0;
        for (const auto& preset : presets)
        {
            const auto hero = triangles(preset.hero_), swarm = triangles(preset.swarm_);
            require(hero >= previous && swarm <= hero, std::string(preset.name_) + ": preset order");
            previous = hero;
        }

        std::cout << "triangles (vertices) by structure x surface\n";
        for (const auto structure : structures)
        {
            for (const auto surface : surfaces)
            {
                const auto mesh = buildNanite({structure, surface});
                std::cout << "  " << name({structure, surface}) << ' ' << mesh.indices_.size() / 3 << " ("
                          << mesh.vertices_.size() << ")";
            }
            std::cout << '\n';
        }
        std::cout << "Procedural nanite model PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
