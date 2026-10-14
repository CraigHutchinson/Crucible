#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <numeric>
#include <stdexcept>
#include <SwarmIntro.hpp>
#include <vector>

#include "crucible/presentation/production/detail/mesh_validation.hpp"
#include "crucible/presentation/production/detail/swarm_model_mesh.hpp"
#include "crucible/presentation/production/nanite_model.hpp"

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
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    try { return ::operator new(size); }
    catch (const std::bad_alloc&) { return nullptr; }
}
void* operator new[](std::size_t size, const std::nothrow_t& tag) noexcept
{
    return ::operator new(size, tag);
}
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete(void* memory, const std::nothrow_t&) noexcept { ::operator delete(memory); }
void operator delete[](void* memory, const std::nothrow_t&) noexcept { ::operator delete[](memory); }

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
    const auto length = std::sqrt(dot(v, v));
    require(std::isfinite(length) && length > 0, "Invalid receiving normal/basis");
    for (auto& component : v) component /= length;
    return v;
}
Vec convert(const std::array<float, 3>& v) { return {v[0], v[1], v[2]}; }
bool close(Vec a, Vec b, double tolerance = 5e-5)
{
    for (std::size_t axis = 0; axis < 3; ++axis) if (std::abs(a[axis] - b[axis]) > tolerance) return false;
    return true;
}
NaniteBody modelProfile()
{
    NaniteBody body;
    body.height = naniteCompactEnvelope()[3][1] * body.span;
    body.lean = 0;
    return body;
}
RenderRow freeRow()
{
    RenderRow row{};
    row.act = Act::Lane;
    row.tier = Tier::Hero;
    return row;
}
Camera nearCamera()
{
    Camera camera;
    camera.halfWidth = 2;
    camera.pixelsWide = 4096;
    return camera;
}
Vec transformModel(Vec model, const RenderRow& row, double scale)
{
    // Independent composition: MODEL(x,y,z)->SOURCE(z,x,y), source heading,
    // hinge, then SOURCE(x,y,z)->NATIVE(x,z,-y). The cyclic mapping is proper.
    const double a = model[2] * scale, b = model[0] * scale, h = model[1] * scale;
    const double c = std::cos(static_cast<double>(row.heading)), s = std::sin(static_cast<double>(row.heading));
    const double pa = c * a - s * b, pb = s * a + c * b;
    const double cp = std::cos(static_cast<double>(row.plane)), sp = std::sin(static_cast<double>(row.plane));
    return {row.x + pa, row.z + pb * sp + h * cp, -row.y - pb * cp + h * sp};
}
Vec transformNormal(Vec normal, const RenderRow& row)
{
    auto directions = row;
    directions.x = directions.y = directions.z = 0;
    return normalize(transformModel(normal, directions, 1));
}
NaniteMaterialRole roleFor(NaniteMaterial material)
{
    if (material == NaniteMaterial::Silver) return NaniteMaterialRole::shield;
    if (material == NaniteMaterial::Graphite) return NaniteMaterialRole::hub;
    require(material == NaniteMaterial::Heart, "Housing must be explicitly omitted");
    return NaniteMaterialRole::core;
}
std::size_t selectedSingleRung(const detail::SwarmModelMesh::Stats& stats)
{
    require(stats.visibleUnits_ == 1 && stats.culledUnits_ == 0, "Single source unit unexpectedly culled");
    for (std::size_t rung = 0; rung < stats.actualRungCounts_.size(); ++rung)
        if (stats.actualRungCounts_[rung] == 1) return rung;
    throw std::runtime_error("No actual single-unit rung receipt");
}
void compareFreeSource(detail::SwarmModelMesh& producer)
{
    const auto body = modelProfile();
    const auto camera = nearCamera();
    std::array<RenderRow, 1> rows{freeRow()};
    for (const float plane : {0.0F, 0.7F, 1.57079632679F})
    {
        rows[0].heading = .43F;
        rows[0].plane = plane;
        require(producer.tryBuild(rows, body, camera, .4F, 1), "M3 free unit receiving refused");
        const auto stats = producer.getStats();
        const auto rung = selectedSingleRung(stats);
        require(rung == 5, "Close-up did not consume the actual rich M3 ladder prefix");
        const auto source = buildNanite(naniteLodLadder()[rung].detail_, {}, 0);
        const auto frame = producer.getFrame();
        require(frame.normalMode_ == NaniteNormalMode::smooth && detail::isValidFrame(frame), "Smooth M3 packet fails its declared normal contract");
        auto falselyFlat = frame;
        falselyFlat.normalMode_ = NaniteNormalMode::flat;
        require(!detail::isValidFrame(falselyFlat), "Legacy flat-normal threshold silently weakened");
        auto invalidMode = frame;
        invalidMode.normalMode_ = static_cast<NaniteNormalMode>(99);
        require(!detail::isValidFrame(invalidMode), "Unknown normal policy accepted");
        std::size_t actual{}, housing{};
        for (std::size_t index = 0; index < source.indices_.size(); index += 3)
        {
            const auto& a = source.vertices_[source.indices_[index]];
            const auto& b = source.vertices_[source.indices_[index + 1]];
            const auto& c = source.vertices_[source.indices_[index + 2]];
            const auto face = cross(subtract(convert(b.position_), convert(a.position_)), subtract(convert(c.position_), convert(a.position_)));
            if (dot(face, face) <= 1e-16) continue;
            if (a.material_ == NaniteMaterial::Housing) { ++housing; continue; }
            for (std::size_t corner = 0; corner < 3; ++corner)
            {
                require(actual < frame.vertices_.size(), "M3 producer lost an authoritative triangle");
                const auto& reference = source.vertices_[source.indices_[index + corner]];
                const auto& emitted = frame.vertices_[actual++];
                require(close(convert(emitted.position_), transformModel(convert(reference.position_), rows[0], body.span)),
                    "Free rest geometry differs from the actual new model/proper-axis transform");
                require(close(convert(emitted.normal_), transformNormal(convert(reference.normal_), rows[0]), 2e-4),
                    "M3 smooth normals were flattened or incorrectly transformed");
                require(emitted.role_ == roleFor(reference.material_) && std::abs(emitted.intensity_ - 1) < 2e-6,
                    "M3 material role/act intensity values were reinterpreted");
            }
        }
        require(actual == frame.vertices_.size() && housing > 0 && stats.omittedHousingTriangles_ == housing,
            "New model omission/count receipt disagrees with its authoritative mesh");
        const Vec eye = convert(frame.camera_.eye_), target = convert(frame.camera_.target_);
        const auto forward = normalize(subtract(target, eye));
        const auto right = normalize(cross(forward, {0, 1, 0})), up = cross(right, forward);
        require(frame.camera_.orthographic_.has_value(), "Swarm camera became perspective");
        for (const auto& vertex : frame.vertices_)
        {
            const auto relative = subtract(convert(vertex.position_), target);
            // Native z=-source depth; zero source yaw for this fixture.
            const double expectedX = vertex.position_[0] / camera.halfWidth;
            const double expectedY = (vertex.position_[1] - .4 * vertex.position_[2]) / (camera.halfWidth / camera.aspect);
            require(std::abs(dot(relative, right) / frame.camera_.orthographic_->halfWidth_ - expectedX) < 5e-5 &&
                std::abs(dot(relative, up) / frame.camera_.orthographic_->halfHeight_ - expectedY) < 5e-5,
                "Actual viewport did not preserve source affine projection");
        }
    }
}
void verifyStockSourceRefusal(detail::SwarmModelMesh& producer)
{
    IntroSettings settings;
    settings.build = Build::Scaffold;
    settings.nanites = 1536;
    settings.body = modelProfile();
    SwarmIntro source{settings};
    require(source.trySetViewport(1280.0F / 864.0F, 1280), "Stock refusal source viewport refused");
    std::vector<RenderRow> rows;
    rows.reserve(NaniteMeshFrame::maxUnits);
    producer.reset();
    // Preserve the actual actors, morphed shapes and camera at the first failing
    // desktop tick, with a short receiving prefix to retain adjacent LOD history.
    for (unsigned tick = 1; tick <= 439; ++tick)
    {
        source.tick();
        if (tick < 430) continue;
        source.snapshot(rows);
        require(rows.size() == settings.nanites, "Stock refusal fixture changed the source population");
        require(producer.tryBuild(rows, settings.body, source.camera(), settings.viewTilt, source.time(), settings.diameter),
            "Stock source M3 packet refused near desktop tick 439");
        require(detail::isValidFrame(producer.getFrame()), "Stock source packet failed the original mesh validator");
        const auto stats = producer.getStats();
        require(stats.visibleUnits_ + stats.culledUnits_ == settings.nanites &&
            std::accumulate(stats.actualRungCounts_.begin(), stats.actualRungCounts_.end(), std::size_t{0}) == stats.visibleUnits_,
            "Stock refusal receiving lost a source identity");
    }
}
void verifyIntermediateStretch(detail::SwarmModelMesh& producer)
{
    const auto body = modelProfile();
    constexpr std::array<std::size_t, 6> identities{0, 1, 2, 3, 1070, 118};
    constexpr std::array<std::size_t, 2> rungs{3, 5};
    std::array<std::array<std::vector<NaniteMaterialRole>, 2>, 4> expectedRoles;
    for (std::size_t variant = 0; variant < expectedRoles.size(); ++variant)
    {
        for (std::size_t selection = 0; selection < rungs.size(); ++selection)
        {
            const auto source = buildNanite(naniteLodLadder()[rungs[selection]].detail_, {}, static_cast<std::uint32_t>(variant));
            auto& roles = expectedRoles[variant][selection];
            for (std::size_t triangle = 0; triangle < source.indices_.size(); triangle += 3)
            {
                const auto& a = source.vertices_[source.indices_[triangle]];
                const auto& b = source.vertices_[source.indices_[triangle + 1]];
                const auto& c = source.vertices_[source.indices_[triangle + 2]];
                const auto face = cross(subtract(convert(b.position_), convert(a.position_)), subtract(convert(c.position_), convert(a.position_)));
                if (a.material_ == NaniteMaterial::Housing || dot(face, face) <= 1e-16) continue;
                for (std::size_t corner = 0; corner < 3; ++corner) roles.push_back(roleFor(a.material_));
            }
        }
    }
    // Identity is the row index, not a field in RenderRow. Predecessors stay
    // outside the view so the original failing variant retains its identity.
    std::vector<RenderRow> rows(1071, freeRow());
    for (auto& row : rows) row.x = 200;
    for (const auto identity : identities)
    {
        rows[identity] = freeRow();
        auto& row = rows[identity];
        row.x = identity == 118 ? 31.99038F : 1.39230442F;
        row.y = identity == 118 ? -10.0F : 7.5F;
        row.z = identity == 118 ? 10.0F : 0.0F;
        for (std::size_t selection = 0; selection < rungs.size(); ++selection)
        {
            const std::array<float, 3> speeds{9.5F, 10.5368996F, 18.0F};
            // The wider view holds Frame/Bevel at the formerly refused speed;
            // close-ups also exercise two additional intermediate stretches.
            const auto speedCount = selection == 0 ? std::size_t{1} : speeds.size();
            for (std::size_t speedIndex = 0; speedIndex < speedCount; ++speedIndex)
            {
                producer.reset();
                row.speed = identity == 118 && speedIndex == 0 ? 10.5368996F : speeds[speedIndex];
                row.heading = selection == 0 ? 0.0F : .43F;
                row.plane = selection == 0 ? 0.0F : .7F;
                auto camera = nearCamera();
                camera.cx = row.x;
                camera.cy = row.z + .4F * row.y;
                if (selection == 0) { camera.halfWidth = 8; camera.pixelsWide = 400; }
                else if (identity == 1070 || identity == 118)
                {
                    // Copied camera receipts; heading/plane remain deliberate
                    // test poses because the failure log did not record them.
                    camera.cx = identity == 1070 ? -3.68165922F : 27.0489235F;
                    camera.cy = identity == 1070 ? 4.2228303F : 7.45619059F;
                    camera.halfWidth = identity == 1070 ? 10.6312847F : 10.7426529F;
                    camera.aspect = 1.48148143F;
                    camera.pixelsWide = 1280;
                }
                require(producer.tryBuild(rows, body, camera, .4F, 2), "Intermediate-stretch M3 packet refused");
                const auto stats = producer.getStats();
                require(stats.visibleUnits_ == 1 && stats.culledUnits_ == rows.size() - 1 &&
                    stats.actualRungCounts_[rungs[selection]] == 1, "Regression did not receive its stable identity/selected rung");
                const auto frame = producer.getFrame();
                const auto& roles = expectedRoles[identity % expectedRoles.size()][selection];
                require(frame.normalMode_ == NaniteNormalMode::smooth && detail::isValidFrame(frame),
                    "Intermediate-stretch M3 packet fails the unchanged receiver contract");
                require(frame.vertices_.size() == roles.size(), "Normal repair discarded or duplicated an authoritative triangle");
                for (std::size_t triangle = 0; triangle < frame.vertices_.size(); triangle += 3)
                {
                    const auto face = normalize(cross(
                        subtract(convert(frame.vertices_[triangle + 1].position_), convert(frame.vertices_[triangle].position_)),
                        subtract(convert(frame.vertices_[triangle + 2].position_), convert(frame.vertices_[triangle].position_))));
                    for (std::size_t corner = 0; corner < 3; ++corner)
                    {
                        const auto& vertex = frame.vertices_[triangle + corner];
                        const auto normal = convert(vertex.normal_);
                        require(vertex.role_ == roles[triangle + corner], "Normal repair reordered authoritative material topology");
                        require(std::isfinite(dot(normal, normal)) && std::abs(dot(normal, normal) - 1) <= .002 &&
                            dot(face, normal) >= .01, "Mapped normal is not finite, unit length and in its face hemisphere");
                    }
                }
            }
        }
        row = freeRow();
        row.x = 200;
    }
}
std::vector<NaniteMeshVertex> heartOf(const NaniteMeshFrame& frame)
{
    std::vector<NaniteMeshVertex> result;
    for (const auto& vertex : frame.vertices_) if (vertex.role_ == NaniteMaterialRole::core) result.push_back(vertex);
    require(!result.empty(), "New model heart absent");
    return result;
}
void requireSameHeart(const std::vector<NaniteMeshVertex>& before, const std::vector<NaniteMeshVertex>& after)
{
    require(before.size() == after.size(), "Rigid-heart topology changed");
    for (std::size_t i = 0; i < before.size(); ++i)
        require(close(convert(before[i].position_), convert(after[i].position_), 2e-6) &&
            close(convert(before[i].normal_), convert(after[i].normal_), 2e-6), "Speed stretched the physical heart");
}
void verifyRigidityAndTile(detail::SwarmModelMesh& producer)
{
    const auto body = modelProfile();
    auto camera = nearCamera();
    std::array<RenderRow, 1> rows{freeRow()};
    require(producer.tryBuild(rows, body, camera, .4F, 1), "Compact M3 heart receiving failed");
    const auto compact = heartOf(producer.getFrame());
    rows[0].speed = 24;
    require(producer.tryBuild(rows, body, camera, .4F, 1), "Stretched M3 heart receiving failed");
    requireSameHeart(compact, heartOf(producer.getFrame()));
    rows[0].shaped = true;
    rows[0].act = Act::Locked;
    rows[0].shape = {-.62F, -.42F, .95F, -.50F, .10F, .80F};
    rows[0].heading = 1.1F;
    require(producer.tryBuild(rows, body, camera, .4F, 12), "Morphed M3 tile receiving failed");
    require(detail::isValidFrame(producer.getFrame()), "Morphed M3 inverse-transpose/winding failed");
    const auto shapedHeart = heartOf(producer.getFrame());
    const std::vector<NaniteMeshVertex> shaped(producer.getFrame().vertices_.begin(), producer.getFrame().vertices_.end());
    rows[0].heading = -1.7F;
    rows[0].speed = 0;
    require(producer.tryBuild(rows, body, camera, .4F, 12), "Stable morphed tile receiving failed");
    requireSameHeart(shapedHeart, heartOf(producer.getFrame()));
    require(shaped.size() == producer.getFrame().vertices_.size(), "Morphed tile topology changed with irrelevant heading/speed");
    for (std::size_t i = 0; i < shaped.size(); ++i)
        require(close(convert(shaped[i].position_), convert(producer.getFrame().vertices_[i].position_), 2e-6), "Morphed tile rotated with ignored heading");
    // The fitted physical heart stays clear of the COMPACT tile, not merely its stretched shell.
    const Vec apex{(.10 + -.62 + .95) / 3, body.height, -(.80 + -.42 + -.50) / 3};
    const std::array<Vec, 4> tetra{{{.10, 0, -.80}, {-.62, 0, .42}, {.95, 0, .50}, apex}};
    constexpr std::array<std::array<std::size_t, 3>, 4> faces{{{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}}};
    for (const auto& face : faces)
    {
        auto normal = normalize(cross(subtract(tetra[face[1]], tetra[face[0]]), subtract(tetra[face[2]], tetra[face[0]])));
        const auto other = 6 - face[0] - face[1] - face[2];
        if (dot(normal, subtract(tetra[other], tetra[face[0]])) > 0) for (auto& axis : normal) axis = -axis;
        for (const auto& vertex : shapedHeart)
            require(dot(subtract(convert(vertex.position_), tetra[face[0]]), normal) <= -.04 + 5e-5, "Physical heart violates compact tile clearance");
    }
    rows[0] = freeRow();
    camera.halfWidth = 500; camera.pixelsWide = 1;
    require(producer.tryBuild(rows, body, camera, .4F, 1), "Silhouette facade receiving failed");
    require(selectedSingleRung(producer.getStats()) == 0, "Distant unit failed to use actual model silhouette");
    const auto facade = heartOf(producer.getFrame());
    rows[0].speed = 24;
    require(producer.tryBuild(rows, body, camera, .4F, 1), "Stretched silhouette receiving failed");
    const auto stretchedFacade = heartOf(producer.getFrame());
    bool moved{};
    for (std::size_t i = 0; i < std::min(facade.size(), stretchedFacade.size()); ++i)
        moved = moved || !close(convert(facade[i].position_), convert(stretchedFacade[i].position_), 2e-6);
    require(moved, "Silhouette luminous facade was incorrectly treated as the physical rigid heart");
}
void verifyBudgetAndCulling(detail::SwarmModelMesh& producer)
{
    const auto body = modelProfile();
    const auto camera = nearCamera();
    std::vector<RenderRow> rows(4096, freeRow());
    const auto capacity = producer.getCapacity();
    allocationCalls.store(0, std::memory_order_relaxed);
    bool accepted{};
    {
        struct AllocationWindow
        {
            AllocationWindow() { countAllocations.store(true, std::memory_order_relaxed); }
            ~AllocationWindow() { countAllocations.store(false, std::memory_order_relaxed); }
        } window;
        accepted = producer.tryBuild(rows, body, camera, .4F, 1);
    }
    require(accepted && allocationCalls.load(std::memory_order_relaxed) == 0, "Maximum M3 frame failed or allocated ordinary hot-loop storage");
    const auto stats = producer.getStats();
    require(stats.visibleUnits_ == 4096 && stats.culledUnits_ == 0 &&
        std::accumulate(stats.actualRungCounts_.begin(), stats.actualRungCounts_.end(), std::size_t{0}) == 4096,
        "Visible/rung accounting changed the actual source population");
    require(producer.getDetailedCount() <= 128 && producer.getDetailedCount() > 0 && stats.actualRungCounts_[0] >= 4096 - 128,
        "Rich candidates escaped the balanced per-frame cap");
    require(producer.getFrame().vertices_.size() <= 1048576 && producer.getFrame().vertices_.size() <= NaniteMeshFrame::maxVertices &&
        stats.cacheBytes_ <= 24 * 1024 * 1024 && producer.getCapacity() == capacity, "M3 producer/frame/cache exceeded received storage");
    require(detail::isValidFrame(producer.getFrame()), "Worst-count M3 packet failed smooth outward-normal validation");
    const std::vector<NaniteMeshVertex> first(producer.getFrame().vertices_.begin(), producer.getFrame().vertices_.end());
    require(producer.tryBuild(rows, body, camera, .4F, 1), "Repeated maximum frame refused");
    require(producer.getStats().actualRungCounts_ == stats.actualRungCounts_ && producer.getFrame().vertices_.size() == first.size(),
        "Equal-distance stable identity budget selection changed");
    for (std::size_t i = 0; i < first.size(); ++i)
        require(first[i].position_ == producer.getFrame().vertices_[i].position_ && first[i].role_ == producer.getFrame().vertices_[i].role_,
            "Repeatable variant/identity geometry changed");
    producer.reset();
    require(producer.getFrame().vertices_.empty() && producer.getStats().visibleUnits_ == 0 &&
        producer.getStats().cacheBytes_ == stats.cacheBytes_ && producer.getCapacity() == capacity,
        "Replay reset released cold caches or retained an old frame");
    require(producer.tryBuild(rows, body, camera, .4F, 1) && producer.getStats().actualRungCounts_ == stats.actualRungCounts_,
        "Replay reset changed fresh high-size identity selection");
    // A low, wholly outside body is culled; a boundary-crossing body remains whole.
    std::array<RenderRow, 2> outside{freeRow(), freeRow()};
    outside[0].x = 50;
    outside[1].x = camera.halfWidth + .1F;
    require(producer.tryBuild(outside, body, camera, .4F, 1), "Render-only viewport culling refused valid bodies");
    require(producer.getStats().visibleUnits_ == 1 && producer.getStats().culledUnits_ == 1,
        "Body center culling discarded a mapped tetrahedron crossing the viewport");
    rows.push_back(freeRow());
    require(!producer.tryBuild(rows, body, camera, .4F, 1), "Over-bound population admitted");
    std::array<RenderRow, 1> invalid{freeRow()};
    invalid[0].x = std::numeric_limits<float>::quiet_NaN();
    require(!producer.tryBuild(invalid, body, camera, .4F, 1), "Nonfinite source row admitted");
    invalid[0] = freeRow(); invalid[0].shaped = true; invalid[0].shape.fill(0);
    require(!producer.tryBuild(invalid, body, camera, .4F, 1), "Degenerate compact docking envelope admitted");
}
}

int main()
{
    try
    {
        detail::SwarmModelMesh producer;
        compareFreeSource(producer);
        verifyStockSourceRefusal(producer);
        verifyIntermediateStretch(producer);
        verifyRigidityAndTile(producer);
        verifyBudgetAndCulling(producer);
        std::cout << "Authoritative M3 mesh/source transforms, smooth normals, rigid interior and balanced frame received\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
