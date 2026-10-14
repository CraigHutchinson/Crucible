#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <locale>
#include <new>
#include <stdexcept>
#include <string>
#include <SwarmIntro.hpp>
#include <vector>

#include "crucible/presentation/production/detail/nanite_proxy_mesh.hpp"
#include "crucible/presentation/production/nanite_mesh_frame.hpp"

namespace
{
std::atomic<bool> countAllocations{};
std::atomic<std::size_t> allocationCalls{};
}
void* operator new(std::size_t size)
{
    if (countAllocations.load(std::memory_order_relaxed)) allocationCalls.fetch_add(1, std::memory_order_relaxed);
    if (auto* memory = std::malloc(std::max(size, std::size_t{1}))) return memory;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace
{
using namespace crucible::presentation::production;
using namespace crucible::spike::swarm;
using Vec = std::array<double, 3>;
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
Vec subtract(Vec a, Vec b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
double dot(Vec a, Vec b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vec cross(Vec a, Vec b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
Vec normalize(Vec v)
{
    const double length = std::sqrt(dot(v, v));
    require(std::isfinite(length) && length > 0, "Invalid fixture camera vector");
    for (auto& value : v) value /= length;
    return v;
}
Vec convert(const std::array<float, 3>& v) { return {v[0], v[1], v[2]}; }
RenderRow freeRow() { RenderRow row{}; row.act = Act::Lane; row.tier = Tier::Hero; return row; }

void verifyTriangles(std::span<const NaniteMeshVertex> vertices)
{
    require(vertices.size() % 3 == 0, "Triangle-list remainder");
    for (std::size_t i = 0; i < vertices.size(); i += 3)
    {
        const auto geometric = cross(subtract(convert(vertices[i + 1].position_), convert(vertices[i].position_)),
            subtract(convert(vertices[i + 2].position_), convert(vertices[i].position_)));
        require(dot(geometric, geometric) >= 1e-16, "Degenerate packed float triangle");
        const auto normal = normalize(geometric);
        for (std::size_t c = 0; c < 3; ++c)
        {
            const auto supplied = convert(vertices[i + c].normal_);
            require(std::abs(dot(supplied, supplied) - 1) <= .002 && dot(normal, supplied) >= .995, "Outward/unit-normal mismatch");
            require(vertices[i + c].role_ == vertices[i].role_ && vertices[i + c].intensity_ == vertices[i].intensity_, "Mixed triangle material");
        }
    }
}

void verifyOracle(const std::filesystem::path& path)
{
    std::ifstream input{path};
    input.imbue(std::locale::classic());
    std::string magic;
    unsigned version{};
    std::size_t cases{};
    input >> magic >> version >> cases;
    require(input.good() && magic == "CRUCIBLE_NANITE_PROXY_ORACLE" && version == 1 && cases >= 20, "Required exact-source oracle missing/unsupported");
    detail::NaniteProxyMesh producer;
    for (std::size_t caseIndex = 0; caseIndex < cases; ++caseIndex)
    {
        std::string name;
        std::size_t count{};
        input >> name >> count;
        require(count > 0 && count <= NaniteMeshFrame::maxUnits, "Invalid oracle population");
        NaniteBody body;
        Camera camera;
        float tilt{}, time{}, diameter{};
        input >> body.height >> body.span >> body.lean >> body.core >> body.stretch >> body.stretchSpeed;
        input >> camera.cx >> camera.cy >> camera.halfWidth >> camera.aspect >> camera.pixelsWide >> camera.yaw;
        input >> tilt >> time >> diameter;
        std::vector<RenderRow> rows(count);
        for (auto& row : rows)
        {
            unsigned act{}, tier{}, magnet{}, layer{}, shaped{};
            input >> row.x >> row.y >> row.z >> row.heading >> row.plane >> act >> tier >> row.actSince >> magnet >> layer >> shaped;
            row.act = static_cast<Act>(act); row.tier = static_cast<Tier>(tier);
            row.magnet = static_cast<std::uint8_t>(magnet); row.layer = static_cast<std::uint8_t>(layer); row.shaped = shaped != 0;
            for (auto& value : row.shape) input >> value;
            input >> row.speed;
        }
        require(input.good(), "Truncated oracle input");
        require(producer.tryBuild(rows, body, camera, tilt, time, diameter), "Oracle body production refused");
        const auto frame = producer.getFrame();
        std::size_t vertices{}, detailCount{};
        input >> vertices >> detailCount;
        require(vertices == frame.vertices_.size() && detailCount == producer.getDetailedCount(), "Oracle detail/count mismatch");
        const auto eye = convert(frame.camera_.eye_), target = convert(frame.camera_.target_);
        const auto forward = normalize(subtract(target, eye));
        const auto right = normalize(cross(forward, {0, 1, 0}));
        const auto up = cross(right, forward);
        require(frame.camera_.orthographic_.has_value(), "Missing actual orthographic receiver");
        for (const auto& actual : frame.vertices_)
        {
            Vec position{}, normal{};
            unsigned role{};
            double intensity{}, expectedX{}, expectedY{};
            input >> position[0] >> position[1] >> position[2] >> normal[0] >> normal[1] >> normal[2] >> role >> intensity >> expectedX >> expectedY;
            require(input.good(), "Truncated oracle mesh");
            for (std::size_t axis = 0; axis < 3; ++axis)
                require(std::abs(actual.position_[axis] - position[axis]) <= 3e-5 && std::abs(actual.normal_[axis] - normal[axis]) <= 3e-5,
                    "Independent Python body/normal parity mismatch");
            require(static_cast<unsigned>(actual.role_) == role && std::abs(actual.intensity_ - intensity) <= 2e-6, "Source role/act hierarchy mismatch");
            const auto relative = subtract(convert(actual.position_), target);
            require(std::abs(dot(relative, right) / frame.camera_.orthographic_->halfWidth_ - expectedX) <= 3e-5 &&
                std::abs(dot(relative, up) / frame.camera_.orthographic_->halfHeight_ - expectedY) <= 3e-5, "Independent reference affine camera mismatch");
        }
        verifyTriangles(frame.vertices_);
        std::cout << "oracle=" << name << " vertices=" << vertices << '\n';
    }
    input >> std::ws;
    require(input.eof(), "Trailing oracle data");
}

void verifyBoundsAndRigidCore()
{
    detail::NaniteProxyMesh producer;
    const auto capacity = producer.getCapacity();
    require(capacity >= NaniteMeshFrame::maxProxyVertices, "Mesh startup reserve missing");
    NaniteBody body;
    Camera camera;
    camera.pixelsWide = 100;
    std::array<RenderRow, 1> row{freeRow()};
    require(producer.tryBuild(row, body, camera, .4F, 1), "Compact source body refused");
    const auto rest = producer.getFrame();
    require(rest.vertices_.size() == 84, "Basic body is not84vertices");
    std::array<NaniteMeshVertex, 12> core;
    std::copy(rest.vertices_.end() - 12, rest.vertices_.end(), core.begin());
    const std::array<Vec, 4> compactShell{convert(rest.vertices_[0].position_), convert(rest.vertices_[7].position_),
        convert(rest.vertices_[1].position_), convert(rest.vertices_[25].position_)};
    constexpr std::array<std::array<std::size_t, 3>, 4> faces{{{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}}};
    for (const auto& face : faces)
    {
        const auto outward = normalize(cross(subtract(compactShell[face[1]], compactShell[face[0]]),
            subtract(compactShell[face[2]], compactShell[face[0]])));
        for (const auto& corner : core)
            require(dot(subtract(convert(corner.position_), compactShell[face[0]]), outward) <= -.04 + 3e-5,
                "Rigid core violates independent compact-shell clearance");
    }
    const std::array<Vec, 4> coreCorners{convert(core[0].position_), convert(core[2].position_),
        convert(core[1].position_), convert(core[5].position_)};
    const auto firstEdge = subtract(coreCorners[1], coreCorners[0]);
    const double referenceEdgeSquared = dot(firstEdge, firstEdge);
    for (std::size_t a = 0; a < 4; ++a)
        for (std::size_t b = a + 1; b < 4; ++b)
        {
            const auto edge = subtract(coreCorners[b], coreCorners[a]);
            require(std::abs(dot(edge, edge) - referenceEdgeSquared) < 1e-5, "Core is not a regular tetrahedron");
        }
    row[0].speed = 24;
    require(producer.tryBuild(row, body, camera, .4F, 1), "Stretched body refused");
    const auto moving = producer.getFrame();
    for (std::size_t i = 0; i < 12; ++i)
        require(core[i].position_ == moving.vertices_[moving.vertices_.size() - 12 + i].position_, "Speed stretched the rigid core");
    row[0].shaped = true;
    row[0].shape = {-0.62F, -0.42F, .95F, -.5F, .1F, .8F};
    require(producer.tryBuild(row, body, camera, .4F, 1), "Morphed body refused");
    std::vector<NaniteMeshVertex> docked(producer.getFrame().vertices_.begin(), producer.getFrame().vertices_.end());
    row[0].heading = 2.1F;
    require(producer.tryBuild(row, body, camera, .4F, 1), "Docked heading body refused");
    for (std::size_t i = 0; i < docked.size(); ++i)
        require(docked[i].position_ == producer.getFrame().vertices_[i].position_, "Morphed tile incorrectly rotates with heading");
    std::vector<RenderRow> full(NaniteMeshFrame::maxUnits, freeRow());
    camera.pixelsWide = 1920;
    allocationCalls.store(0, std::memory_order_relaxed);
    bool fullAccepted{};
    {
        struct AllocationWindow
        {
            AllocationWindow() { countAllocations.store(true, std::memory_order_relaxed); }
            ~AllocationWindow() { countAllocations.store(false, std::memory_order_relaxed); }
        } window;
        fullAccepted = producer.tryBuild(full, body, camera, .4F, 1);
    }
    require(fullAccepted, "Maximum source population refused");
    require(allocationCalls.load(std::memory_order_relaxed) == 0, "Mesh evaluation allocated ordinary heap storage");
    require(producer.getDetailedCount() == 128 && producer.getFrame().vertices_.size() == NaniteMeshFrame::maxProxyVertices,
        "Detailed body multiplier escaped frozen cap");
    require(producer.getFrame().vertices_[127 * 228].role_ == NaniteMaterialRole::rail &&
        producer.getFrame().vertices_[128 * 228].role_ == NaniteMaterialRole::shield, "Stable identity detail tie-break failed");
    require(producer.getCapacity() == capacity, "Mesh evaluation reallocated storage");
    verifyTriangles(producer.getFrame().vertices_);
    full.push_back(freeRow());
    require(!producer.tryBuild(full, body, camera, .4F, 1), "Over-capacity population accepted");
    row[0].shape.fill(0);
    require(!producer.tryBuild(row, body, camera, .4F, 1), "Degenerate shaped body accepted");
    row[0] = freeRow(); row[0].x = std::numeric_limits<float>::quiet_NaN();
    require(!producer.tryBuild(row, body, camera, .4F, 1), "Nonfinite source row accepted");
}
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Supply the required checked-in reference oracle path");
        verifyOracle(argv[1]);
        verifyBoundsAndRigidCore();
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
