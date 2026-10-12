#include "SwarmIntro.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace crucible::spike::swarm
{
namespace
{
constexpr float kPi = std::numbers::pi_v<float>;
constexpr int kResolvePasses = 8;
constexpr float kLiftBoost = 1.6F;
constexpr float kHeapDepth = 4.0F;   // climb build: depth in front of the wall where seekers heap and flow   // climb speed multiplier while drawn up by the built face

/** splitmix32-style hash: deterministic personality without a shared RNG stream. */
std::uint32_t mix(std::uint32_t v)
{
    v += 0x9E3779B9u;
    v = (v ^ (v >> 16)) * 0x85EBCA6Bu;
    v = (v ^ (v >> 13)) * 0xC2B2AE35u;
    return v ^ (v >> 16);
}

/** Uniform [0,1) from a seed and a channel. */
float unit(std::uint32_t seed, std::uint32_t channel)
{
    return static_cast<float>(mix(seed ^ (channel * 0x632BE5ABu)) >> 8) * (1.0F / 16777216.0F);
}

float wrapAngle(float a)
{
    while (a > kPi) a -= 2.0F * kPi;
    while (a < -kPi) a += 2.0F * kPi;
    return a;
}

float smooth(float a, float b, float t)
{
    t = std::clamp(t, 0.0F, 1.0F);
    t = t * t * (3.0F - 2.0F * t);
    return a + (b - a) * t;
}

bool restAct(Act act)
{
    return act == Act::Dormant || act == Act::Locked || act == Act::Moored || act == Act::Reserve;
}

using Clock = std::chrono::steady_clock;
float microsSince(Clock::time_point start)
{
    return std::chrono::duration<float, std::micro>(Clock::now() - start).count();
}
} // namespace

// ---- Construction -----------------------------------------------------------------------

SwarmIntro::SwarmIntro(IntroSettings settings)
    : settings_(settings)
{
    if (settings_.nanites == 0 || settings_.nanites > sub0ecs::Entity::kMaxEntities)
    {
        throw std::length_error("nanite count outside the ECS identity domain");
    }
    const std::size_t n = settings_.nanites;
    slots_ = buildWordmark(settings_.wordmark);
    // Climb build: the wall stands where the lying title's lower edge was; its face is a second
    // interaction sheet stored above every ground position, so the sheets never interact.
    wallY_ = -settings_.wordmark.capHeight * 0.5F;
    wallBand_ = 40.0F;
    buildConstruction();
    const std::vector<Pose> bed = buildBed();
    float bedTop = -std::numeric_limits<float>::max();
    for (const Pose& p : bed) bedTop = std::max(bedTop, p.y);
    buildLanes(bedTop);
    buildRibbon();

    // Stage bounds for the grid: wordmark, ribbon, lanes and bed, plus a margin.
    float minX = -90.0F, maxX = 90.0F, minY = -20.0F;
    const float maxY = climbing() ? wallBand_ + settings_.wordmark.capHeight + 10.0F : 20.0F;
    for (const Pose& p : bed)
    {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
    }
    cell_ = 2.0F * settings_.diameter;
    gridMinX_ = minX - 20.0F;
    gridMinY_ = minY - 20.0F;
    gridW_ = static_cast<int>((maxX + 20.0F - gridMinX_) / cell_) + 1;
    gridH_ = static_cast<int>((maxY + 20.0F - gridMinY_) / cell_) + 1;
    const std::size_t cells = static_cast<std::size_t>(gridW_) * static_cast<std::size_t>(gridH_);
    cellStart_.assign(cells + 1, 0);
    cellCount_.assign(cells, 0);
    cellMovers_.assign(cells, 0);
    cellItems_.assign(n, 0);

    for (auto* v : { &gx_, &gy_, &gvx_, &gvy_, &gWake_, &correctionX_, &correctionY_ }) v->assign(n, 0.0F);
    for (auto* v : { &gAct_, &gMover_, &gTier_, &desiredTier_, &pendingTicks_, &include_ }) v->assign(n, 0);
    allOnes_.assign(n, 1);
    wakeTime_.assign(n, -1.0F);
    gStall_.assign(n, 0);
    gPush_.assign(n, 0);
    gLayer_.assign(n, 0);
    queueScratch_.reserve(n);
    slotOwner_.assign(slots_.size(), -1);
    slotSupportLogged_.assign(slots_.size(), 0);
    gGoal_.assign(n, -1);
    heroCandidates_.reserve(n);

    world_.reserve(n);
    entities_.reserve(n);
    for (std::uint32_t i = 0; i < n; ++i)
    {
        const std::uint32_t seed = mix(settings_.seed ^ (i * 0x9E3779B1u));
        Mind mind;
        mind.speedScale = 0.9F + 0.2F * unit(seed, 1);
        mind.reaction = 0.05F + 0.2F * unit(seed, 2);
        mind.laneOffset = (unit(seed, 3) - 0.5F) * 8.0F;
        entities_.push_back(world_.create(Identity{ i, seed }, bed[i], Motion{}, mind, HiddenTier{ 0 }));
        gTier_[i] = static_cast<std::uint8_t>(Tier::Hidden);
    }
    planAssignment(bed);
    firstX_ = bed[first_].x;
    firstY_ = bed[first_].y;
    camera_ = cameraAt(0.0F);
}

void SwarmIntro::buildConstruction()
{
    // Build graph over the title's slots: adjacency within a letter, depth from the ground
    // row by multi-source BFS, and a parent toward the ground. In the climb build, a slot is supported
    // by the ground or by a locked neighbour of lower depth, so letters grow bottom-up and
    // every locked unit is connected to the ground through locked units (TERMES-style).
    const std::size_t count = slots_.size();
    // Rows: neighbours within 1.6 pitch. Tessellated: triangles sharing an edge or a vertex
    // (centroids within one side), so an up triangle can rest on the row below.
    const float tile = settings_.wordmark.tile;
    const float rowHeight = tile > 0.0F ? tile * std::sqrt(3.0F) * 0.5F : settings_.wordmark.pitch;
    const float link = tile > 0.0F ? 1.01F * tile : 1.6F * settings_.wordmark.pitch;
    slotNeighbours_.assign(count, {});
    float minY = std::numeric_limits<float>::max();
    for (const Slot& slot : slots_) minY = std::min(minY, slot.y);
    for (std::uint32_t a = 0; a < count; ++a)
    {
        for (std::uint32_t b = a + 1; b < count; ++b)
        {
            if (slots_[a].letter == slots_[b].letter && std::hypot(slots_[a].x - slots_[b].x, slots_[a].y - slots_[b].y) <= link)
            {
                slotNeighbours_[a].push_back(b);
                slotNeighbours_[b].push_back(a);
            }
        }
    }
    constexpr std::uint32_t kUnset = std::numeric_limits<std::uint32_t>::max();
    slotDepth_.assign(count, kUnset);
    slotParent_.assign(count, kUnset);
    slotRoot_.assign(count, kUnset);
    std::deque<std::uint32_t> frontier;
    std::array<float, 8> letterMin{};
    letterMin.fill(std::numeric_limits<float>::max());
    for (const Slot& slot : slots_) letterMin[slot.letter] = std::min(letterMin[slot.letter], slot.y);
    for (std::uint32_t a = 0; a < count; ++a)
    {
        if (slots_[a].y - letterMin[slots_[a].letter] < (tile > 0.0F ? 0.5F * rowHeight : 0.6F * settings_.wordmark.pitch))
        {
            slotDepth_[a] = 0;
            slotParent_[a] = a;
            slotRoot_[a] = a;
            frontier.push_back(a);
        }
    }
    while (!frontier.empty())
    {
        const std::uint32_t a = frontier.front();
        frontier.pop_front();
        for (std::uint32_t b : slotNeighbours_[a])
        {
            if (slotDepth_[b] != kUnset) continue;
            slotDepth_[b] = slotDepth_[a] + 1;
            slotParent_[b] = a;
            slotRoot_[b] = slotRoot_[a];
            frontier.push_back(b);
        }
    }
    for (std::uint32_t a = 0; a < count; ++a)
    {
        if (slotDepth_[a] == kUnset) throw std::logic_error("title slot not connected to the ground row");
    }
    slotLocked_.assign(count, 0);
    gSlotLocked_.assign(count, 0);
    gSlotSeating_.assign(count, 0);
    letterLocked_.fill(0);
    letterCompleteAt_.fill(-1.0F);
    letterSlots_.assign(8, {});
    for (std::uint32_t a = 0; a < count; ++a) letterSlots_[slots_[a].letter].push_back(a);
    // Each letter's footprint on the ground (x span of its ground row).
    letterBaseMin_.fill(std::numeric_limits<float>::max());
    letterBaseMax_.fill(-std::numeric_limits<float>::max());
    for (std::uint32_t a = 0; a < count; ++a)
    {
        if (slotDepth_[a] != 0) continue;
        letterBaseMin_[slots_[a].letter] = std::min(letterBaseMin_[slots_[a].letter], slots_[a].x);
        letterBaseMax_[slots_[a].letter] = std::max(letterBaseMax_[slots_[a].letter], slots_[a].x);
    }
    routeParent_.assign(count, 0);
    routeCost_.assign(count, 0.0F);
    routeHeap_.reserve(count * 4);
    if (!climbing()) return;
    // Stand the title up: height above the ground becomes the wall sheet's coordinate.
    // Lowest centroids sit a third of a row up (triangles) or half a unit up (rows).
    const float footing = tile > 0.0F ? rowHeight / 3.0F : 0.55F * settings_.diameter;
    for (Slot& slot : slots_) slot.y = wallBand_ + (slot.y - minY) + footing;
}

bool SwarmIntro::supported(std::size_t slot, std::span<const std::uint8_t> locked) const
{
    if (slotDepth_[slot] == 0) return true;
    for (std::uint32_t b : slotNeighbours_[slot])
    {
        if (locked[b] && slotDepth_[b] < slotDepth_[slot]) return true;
    }
    return false;
}

void SwarmIntro::goalApproach(const Mind& mind, float& gx, float& gy) const
{
    // Where a seeker heads on the ground: its slot, or (climb build) the wall base below its root.
    const Slot& slot = slots_[static_cast<std::size_t>(mind.goal)];
    if (!climbing())
    {
        gx = slot.x;
        gy = slot.y;
        return;
    }
    // Under the slot, clamped to its letter's footprint, so arrivals spread along the base.
    const Slot& goalSlot = slots_[static_cast<std::size_t>(mind.goal)];
    gx = std::clamp(goalSlot.x, letterBaseMin_[goalSlot.letter], letterBaseMax_[goalSlot.letter]);
    gy = wallY_ - 1.0F;
}

float SwarmIntro::raiseAngle(std::uint8_t letter) const
{
    const float complete = letterCompleteAt_[letter];
    if (settings_.build != Build::Raise || complete < 0.0F) return 0.0F;
    return smooth(0.0F, kPi * 0.5F, (time_ - complete - settings_.raiseDelay) / settings_.raiseSeconds);
}

void SwarmIntro::worldOf(const Pose& pose, const Mind& mind, float& wx, float& wy, float& wz, float& plane) const
{
    wx = pose.x;
    plane = 0.0F;
    if (onWall(mind))
    {
        // Wall sheet: height is the sheet coordinate; lift is a stand-off in front of the face.
        wy = wallY_ - pose.z;
        wz = pose.y - wallBand_;
        plane = kPi * 0.5F;
        return;
    }
    if (settings_.build == Build::Raise && mind.act == Act::Locked && !mind.ribbon)
    {
        // Hinge the flat letter about its baseline (the wall line), top swinging up.
        plane = raiseAngle(slots_[static_cast<std::size_t>(mind.goal)].letter);
        const float r = pose.y - wallY_;
        wy = wallY_ + r * std::cos(plane);
        wz = r * std::sin(plane) + pose.z;
        return;
    }
    wy = pose.y;
    wz = pose.z;
}

void SwarmIntro::viewOf(const Pose& pose, const Mind& mind, float& vx, float& vy) const
{
    if (!oblique())
    {
        vx = pose.x;
        vy = pose.y;
        return;
    }
    // Oblique front camera: height up the screen, depth foreshortened by the tilt.
    float wy, wz, plane;
    worldOf(pose, mind, vx, wy, wz, plane);
    vy = wz + settings_.viewTilt * wy;
}

std::vector<Pose> SwarmIntro::buildBed() const
{
    // Loose hex packing, nearest-first from the bed centre, so the bed is a disc.
    const std::size_t n = settings_.nanites;
    const float spacing = 1.25F * settings_.diameter;
    const float radius = std::sqrt(static_cast<float>(n) * spacing * spacing * 0.866F / kPi) + spacing;
    const float cy = -40.0F - radius;
    const int rings = static_cast<int>(radius / spacing) + 3;
    std::vector<Pose> points;
    for (int r = -rings; r <= rings; ++r)
    {
        for (int q = -rings; q <= rings; ++q)
        {
            const float x = (static_cast<float>(q) + 0.5F * static_cast<float>(r & 1)) * spacing;
            const float y = static_cast<float>(r) * spacing * 0.866F;
            points.push_back({ x, cy + y, 0.0F, 0.0F });
        }
    }
    std::sort(points.begin(), points.end(), [cy](const Pose& a, const Pose& b) {
        const float da = a.x * a.x + (a.y - cy) * (a.y - cy);
        const float db = b.x * b.x + (b.y - cy) * (b.y - cy);
        return da != db ? da < db : (a.y != b.y ? a.y < b.y : a.x < b.x);
    });
    points.resize(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        const std::uint32_t seed = mix(settings_.seed ^ static_cast<std::uint32_t>(i * 0x27D4EB2Fu));
        points[i].x += (unit(seed, 4) - 0.5F) * 0.2F;
        points[i].y += (unit(seed, 5) - 0.5F) * 0.2F;
        points[i].heading = (unit(seed, 6) - 0.5F) * 2.0F * kPi;
    }
    return points;
}

void SwarmIntro::buildLanes(float bedTop)
{
    // Three authored lanes (shot02): from the bed's leading edge to below each letter group.
    const std::vector<float> centres = letterCentres(settings_.wordmark);
    const std::array<float, 3> exits{ (centres[0] + centres[2]) * 0.5F, (centres[3] + centres[5]) * 0.5F,
                                      (centres[6] + centres[7]) * 0.5F };
    const std::array<float, 3> starts{ -8.0F, 0.0F, 8.0F };
    const float exitY = -settings_.wordmark.capHeight * 0.5F - 4.0F;
    for (std::size_t k = 0; k < lanes_.size(); ++k)
    {
        Lane& lane = lanes_[k];
        const float x0 = starts[k], y0 = bedTop + 1.0F;
        const float x1 = starts[k] * 1.5F, y1 = exitY - 10.0F;
        const float x2 = exits[k], y2 = exitY;
        constexpr int kSteps = 32;
        float length = 0.0F;
        for (int i = 0; i <= kSteps; ++i)
        {
            const float t = static_cast<float>(i) / kSteps;
            const float u = 1.0F - t;
            const float x = u * u * x0 + 2.0F * u * t * x1 + t * t * x2;
            const float y = u * u * y0 + 2.0F * u * t * y1 + t * t * y2;
            if (!lane.xs.empty()) length += std::hypot(x - lane.xs.back(), y - lane.ys.back());
            lane.xs.push_back(x);
            lane.ys.push_back(y);
            lane.cumulative.push_back(length);
        }
    }
}

void SwarmIntro::buildRibbon()
{
    // Surplus units moor in a quiet ribbon under the title, toward the menu backdrop (shot05).
    const float pitch = settings_.wordmark.pitch * 1.05F;
    const float baseY = -settings_.wordmark.capHeight * 0.5F - 20.0F;
    for (int row = 0; row < 3; ++row)
    {
        for (float x = -72.0F; x <= 72.0F; x += pitch)
        {
            const float wave = 2.0F * std::sin(x * 0.06F);
            const float slope = 0.12F * std::cos(x * 0.06F);
            ribbon_.push_back({ x, baseY + wave - static_cast<float>(row) * pitch, 0.0F, std::atan(slope) });
        }
    }
}

void SwarmIntro::lanePoint(const Lane& lane, float s, float& x, float& y, float& tx, float& ty) const
{
    const std::size_t last = lane.xs.size() - 1;
    s = std::clamp(s, 0.0F, lane.cumulative[last]);
    const auto it = std::upper_bound(lane.cumulative.begin(), lane.cumulative.end(), s);
    const std::size_t i = std::min<std::size_t>(static_cast<std::size_t>(std::max<std::ptrdiff_t>(it - lane.cumulative.begin() - 1, 0)), last - 1);
    const float span = lane.cumulative[i + 1] - lane.cumulative[i];
    const float f = span > 0.0F ? (s - lane.cumulative[i]) / span : 0.0F;
    const float dx = lane.xs[i + 1] - lane.xs[i];
    const float dy = lane.ys[i + 1] - lane.ys[i];
    x = lane.xs[i] + dx * f;
    y = lane.ys[i] + dy * f;
    const float len = std::max(std::hypot(dx, dy), 1e-6F);
    tx = dx / len;
    ty = dy / len;
}

// ---- Planner ----------------------------------------------------------------------------

void SwarmIntro::planAssignment(std::span<const Pose> bed)
{
    // The planner is centralised and runs once at load, like a film's art-directed rig: it
    // predicts the answer wave with the same neighbour rule the signal system executes, then
    // pairs wake order with a far-first slot order so early arrivals never block later paths.
    const std::size_t n = bed.size();
    const std::size_t lanesN = lanes_.size();
    float topY = -std::numeric_limits<float>::max();
    for (std::uint32_t i = 0; i < n; ++i)
    {
        const float score = bed[i].y - std::abs(bed[i].x) * 0.25F;
        if (score > topY)
        {
            topY = score;
            first_ = i;
        }
    }

    // BFS hop gradient over the bed (Kilobot-style gradient from a seed robot).
    hop_.assign(n, std::numeric_limits<std::uint32_t>::max());
    {
        std::vector<std::uint8_t> all(n, 1);
        std::vector<float> xs(n), ys(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            xs[i] = bed[i].x;
            ys[i] = bed[i].y;
        }
        rebuildGrid(xs, ys, all);
        std::copy(xs.begin(), xs.end(), gx_.begin());
        std::copy(ys.begin(), ys.end(), gy_.begin());
        std::deque<std::uint32_t> frontier{ first_ };
        hop_[first_] = 0;
        const float reach = 1.6F * settings_.diameter;
        while (!frontier.empty())
        {
            const std::uint32_t i = frontier.front();
            frontier.pop_front();
            forNeighbours(xs[i], ys[i], reach, [&](std::uint32_t j) {
                if (hop_[j] == std::numeric_limits<std::uint32_t>::max())
                {
                    hop_[j] = hop_[i] + 1;
                    frontier.push_back(j);
                }
            });
        }
    }

    std::vector<std::uint32_t> order(n);
    for (std::uint32_t i = 0; i < n; ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](std::uint32_t a, std::uint32_t b) { return hop_[a] < hop_[b]; });

    const std::size_t slotted = std::min(n, slots_.size());
    const std::size_t moored = std::min(n - slotted, ribbon_.size());
    assignedSlots_ = slotted;
    std::uint32_t maxHop = 1;
    for (std::size_t k = 0; k < slotted; ++k) maxHop = std::max(maxHop, hop_[order[k]]);
    hopDelay_ = std::max(settings_.tickSeconds, settings_.answerWindow / static_cast<float>(maxHop));

    // Letter groups per lane: C R U | C I B | L E.
    auto laneOfLetter = [](std::uint8_t letter) -> std::uint8_t { return letter < 3 ? 0 : (letter < 6 ? 1 : 2); };
    std::array<std::vector<std::uint32_t>, 3> laneSlots;
    for (std::uint32_t s = 0; s < slots_.size(); ++s) laneSlots[laneOfLetter(slots_[s].letter)].push_back(s);

    // Split the slotted units across lanes by bed x so the lanes do not cross.
    std::vector<std::uint32_t> units(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(slotted));
    std::stable_sort(units.begin(), units.end(), [&](std::uint32_t a, std::uint32_t b) { return bed[a].x < bed[b].x; });
    std::size_t cursor = 0;
    for (std::size_t k = 0; k < lanesN; ++k)
    {
        const std::size_t take = std::min(laneSlots[k].size(), units.size() - cursor);
        std::vector<std::uint32_t> laneUnits(units.begin() + static_cast<std::ptrdiff_t>(cursor),
                                             units.begin() + static_cast<std::ptrdiff_t>(cursor + take));
        cursor += take;
        std::stable_sort(laneUnits.begin(), laneUnits.end(), [&](std::uint32_t a, std::uint32_t b) { return hop_[a] < hop_[b]; });
        const Lane& lane = lanes_[k];
        const float ex = lane.xs.back(), ey = lane.ys.back();
        std::vector<std::uint32_t>& ls = laneSlots[k];
        // Lying: far-first, so early arrivals never block later paths. Climb build: bottom-up by
        // build depth first (support), then far-first along the wall's base.
        const bool bottomUp = climbing();
        const float baseY = wallY_;
        std::stable_sort(ls.begin(), ls.end(), [&](std::uint32_t a, std::uint32_t b) {
            if (bottomUp && slotDepth_[a] != slotDepth_[b]) return slotDepth_[a] < slotDepth_[b];
            const float ay = bottomUp ? baseY : slots_[a].y;
            const float by = bottomUp ? baseY : slots_[b].y;
            const float da = std::hypot(slots_[a].x - ex, ay - ey);
            const float db = std::hypot(slots_[b].x - ex, by - ey);
            return da > db;
        });
        for (std::size_t m = 0; m < laneUnits.size(); ++m)
        {
            Mind* mind = world_.find<Mind>(entities_[laneUnits[m]]);
            mind->lane = static_cast<std::uint8_t>(k);
            // Climb build: no preset slot. The unit is bound for a letter and claims its slot
            // at that letter's base, in arrival order.
            if (climbing()) mind->letter = static_cast<std::int8_t>(slots_[ls[m]].letter);
            else mind->goal = static_cast<std::int32_t>(ls[m]);
        }
    }

    // Ribbon moorings: next units in wake order, far-first along x from the bed centre.
    std::vector<std::uint32_t> ribbonOrder(ribbon_.size());
    for (std::uint32_t r = 0; r < ribbon_.size(); ++r) ribbonOrder[r] = r;
    std::stable_sort(ribbonOrder.begin(), ribbonOrder.end(), [&](std::uint32_t a, std::uint32_t b) {
        return std::abs(ribbon_[a].x) > std::abs(ribbon_[b].x);
    });
    for (std::size_t m = 0; m < moored; ++m)
    {
        Mind* mind = world_.find<Mind>(entities_[order[slotted + m]]);
        mind->ribbon = true;
        mind->goal = static_cast<std::int32_t>(ribbonOrder[m]);
    }
}

// ---- Camera -----------------------------------------------------------------------------

Camera SwarmIntro::cameraAt(float t) const
{
    // Orthographic stand-in for the brief's lens path: macro, pullback, rise, hold, open.
    const float fx = firstX_;
    const float fy = oblique() ? settings_.viewTilt * firstY_ : firstY_;
    struct Key
    {
        float t, x, y, w;
    };
    // Upright builds: an oblique front view that rises to frame the standing title.
    const std::array<Key, 7> uprightKeys{ { { 0.0F, fx, fy, 3.0F },
                                            { 1.8F, fx, fy + 0.4F, 4.0F },
                                            { 4.2F, 0.0F, fy + 6.0F, 50.0F },
                                            { 6.8F, 0.0F, 4.0F, 82.0F },
                                            { 8.4F, 0.0F, 6.0F, 80.0F },
                                            { 10.0F, 0.0F, 6.0F, 79.0F },
                                            { 12.0F, 0.0F, -4.0F, 96.0F } } };
    const std::array<Key, 7> lyingKeys{ { { 0.0F, fx, fy, 3.0F },
                                          { 1.8F, fx, fy + 0.5F, 4.0F },
                                          { 4.2F, 0.0F, fy + 10.0F, 50.0F },
                                          { 6.8F, 0.0F, -6.0F, 82.0F },
                                          { 8.4F, 0.0F, -3.0F, 80.0F },
                                          { 10.0F, 0.0F, -4.0F, 79.0F },
                                          { 12.0F, 0.0F, -20.0F, 96.0F } } };
    // Climb build: the user-approved fifteen-second sequence holds the build longer.
    const std::array<Key, 7> climbKeys{ { { 0.0F, fx, fy, 3.0F },
                                          { 1.8F, fx, fy + 0.4F, 4.0F },
                                          { 4.2F, 0.0F, fy + 6.0F, 50.0F },
                                          { 7.0F, 0.0F, 4.0F, 82.0F },
                                          { 12.5F, 0.0F, 6.0F, 80.0F },
                                          { 13.5F, 0.0F, 6.0F, 79.0F },
                                          { 15.0F, 0.0F, -4.0F, 96.0F } } };
    const std::array<Key, 7>& keys = climbing() ? climbKeys : (oblique() ? uprightKeys : lyingKeys);
    Camera cam;
    std::size_t k = 0;
    while (k + 2 < keys.size() && t > keys[k + 1].t) ++k;
    const Key& a = keys[k];
    const Key& b = keys[k + 1];
    const float f = (t - a.t) / (b.t - a.t);
    cam.cx = smooth(a.x, b.x, f);
    cam.cy = smooth(a.y, b.y, f);
    // Zoom interpolates in log space so the pullback reads as a steady dolly.
    cam.halfWidth = std::exp(smooth(std::log(a.w), std::log(b.w), f));
    return cam;
}

bool SwarmIntro::inView(float x, float y, float margin) const
{
    return std::abs(x - camera_.cx) <= camera_.halfWidth + margin && std::abs(y - camera_.cy) <= camera_.halfHeight() + margin;
}

bool SwarmIntro::flows(const Pose& pose, const Mind& mind) const
{
    // Climb build: climbers on the face, and seekers heaped at the wall's base, flow over
    // each other in layers (as the Big Hero 6 microbots slide over one another) instead of
    // colliding; the heap is where the swarm pours onto the wall.
    if (!climbing() || mind.ribbon) return false;
    return mind.act == Act::Climb || (mind.act == Act::Seek && mind.goal >= 0 && pose.y > wallY_ - kHeapDepth);
}

float SwarmIntro::seatHeading(float slotHeading, float current) const
{
    // An equilateral tile looks the same every 120 degrees, so a unit seats at whichever
    // equivalent heading is nearest: an up/down flip costs 60 degrees, not 180.
    if (settings_.wordmark.tile <= 0.0F) return slotHeading;
    float best = slotHeading;
    for (int k = -1; k <= 1; ++k)
    {
        const float candidate = wrapAngle(slotHeading + static_cast<float>(k) * 2.0F * kPi / 3.0F);
        if (std::abs(wrapAngle(candidate - current)) < std::abs(wrapAngle(best - current))) best = candidate;
    }
    return best;
}

bool SwarmIntro::isMover(Act act) const
{
    // Docking units are committed: others glide over them as over the rest layer.
    return !restAct(act) && act != Act::Waking && act != Act::Dock;
}

// ---- Grid -------------------------------------------------------------------------------

void SwarmIntro::rebuildGrid(std::span<const float> xs, std::span<const float> ys, std::span<const std::uint8_t> include)
{
    std::fill(cellCount_.begin(), cellCount_.end(), 0u);
    const std::size_t n = xs.size();
    auto cellOf = [this](float x, float y) {
        const int cx = std::clamp(static_cast<int>((x - gridMinX_) / cell_), 0, gridW_ - 1);
        const int cy = std::clamp(static_cast<int>((y - gridMinY_) / cell_), 0, gridH_ - 1);
        return static_cast<std::size_t>(cy) * static_cast<std::size_t>(gridW_) + static_cast<std::size_t>(cx);
    };
    for (std::size_t i = 0; i < n; ++i)
    {
        if (include[i]) ++cellCount_[cellOf(xs[i], ys[i])];
    }
    std::uint32_t running = 0;
    for (std::size_t c = 0; c < cellCount_.size(); ++c)
    {
        cellStart_[c] = running;
        running += cellCount_[c];
    }
    cellStart_[cellCount_.size()] = running;
    std::fill(cellCount_.begin(), cellCount_.end(), 0u);
    for (std::uint32_t i = 0; i < n; ++i)
    {
        if (!include[i]) continue;
        const std::size_t c = cellOf(xs[i], ys[i]);
        cellItems_[cellStart_[c] + cellCount_[c]++] = i;
    }
}

template <typename F>
void SwarmIntro::forNeighbours(float x, float y, float radius, F&& f) const
{
    const int x0 = std::clamp(static_cast<int>((x - radius - gridMinX_) / cell_), 0, gridW_ - 1);
    const int x1 = std::clamp(static_cast<int>((x + radius - gridMinX_) / cell_), 0, gridW_ - 1);
    const int y0 = std::clamp(static_cast<int>((y - radius - gridMinY_) / cell_), 0, gridH_ - 1);
    const int y1 = std::clamp(static_cast<int>((y + radius - gridMinY_) / cell_), 0, gridH_ - 1);
    const float r2 = radius * radius;
    for (int cy = y0; cy <= y1; ++cy)
    {
        for (int cx = x0; cx <= x1; ++cx)
        {
            const std::size_t c = static_cast<std::size_t>(cy) * static_cast<std::size_t>(gridW_) + static_cast<std::size_t>(cx);
            for (std::uint32_t k = cellStart_[c]; k < cellStart_[c + 1]; ++k)
            {
                const std::uint32_t j = cellItems_[k];
                const float dx = gx_[j] - x;
                const float dy = gy_[j] - y;
                if (dx * dx + dy * dy <= r2) f(j);
            }
        }
    }
}

// ---- Tick -------------------------------------------------------------------------------

void SwarmIntro::tick()
{
    metrics_ = TickMetrics{};
    time_ = static_cast<float>(tickIndex_ + 1) * settings_.tickSeconds;
    camera_ = cameraAt(time_);

    auto start = Clock::now();
    gather();
    metrics_.systemMicros[0] = microsSince(start);

    start = Clock::now();
    signalSystem();
    claimSystem();
    metrics_.systemMicros[1] = microsSince(start);

    start = Clock::now();
    behaviourSystems();
    negotiationSystem();
    contingencySystem();
    metrics_.systemMicros[2] = microsSince(start);

    start = Clock::now();
    resolveSystem();
    metrics_.systemMicros[3] = microsSince(start);

    // Structural changes (tier migration) only after every system has finished.
    start = Clock::now();
    lodSystem();
    metrics_.systemMicros[4] = microsSince(start);

    start = Clock::now();
    metricsSystem();
    metrics_.systemMicros[5] = microsSince(start);
    metrics_.time = time_;
    ++tickIndex_;
}

void SwarmIntro::gather()
{
    std::copy(slotLocked_.begin(), slotLocked_.end(), gSlotLocked_.begin());
    // Pipelined interlock: a slot whose owner is nearly seated already counts as a support for
    // motion (not for locking), so the next level starts sliding in without waiting a full dock.
    std::copy(slotLocked_.begin(), slotLocked_.end(), gSlotSeating_.begin());
    if (climbing())
    {
        world_.each<Identity, Pose, Motion, Mind>([this](Identity&, Pose& pose, Motion&, Mind& mind) {
            if (mind.act != Act::Dock || mind.ribbon) return;
            const Slot& slot = slots_[static_cast<std::size_t>(mind.goal)];
            if (std::hypot(slot.x - pose.x, slot.y - pose.y) < 1.2F * settings_.diameter) gSlotSeating_[static_cast<std::size_t>(mind.goal)] = 1;
        });
    }
    // Climb build: record the moment each slot first becomes supported, and where its owner
    // is then. Support-to-lock latency per level is the climb's critical path.
    if (diagnostics_ && climbing())
    {
        for (std::size_t q = 0; q < slots_.size(); ++q)
        {
            if (slotSupportLogged_[q] || !supported(q, gSlotSeating_)) continue;
            slotSupportLogged_[q] = 1;
            std::uint64_t owner = std::numeric_limits<std::uint64_t>::max(), ownerAct = 0, distance = 0;
            if (slotOwner_[q] >= 0)
            {
                owner = static_cast<std::uint64_t>(slotOwner_[q]);
                const Pose* p = world_.find<Pose>(entities_[owner]);
                const Mind* m = world_.find<Mind>(entities_[owner]);
                ownerAct = static_cast<std::uint64_t>(m->act);
                float ox = p->x, oy = p->y;
                if (m->act == Act::Seek || m->act == Act::Queue) oy = wallBand_;   // on the ground: measure from the base
                distance = static_cast<std::uint64_t>(std::hypot(slots_[q].x - ox, slots_[q].y - oy) * 1000.0F);
            }
            diagnostics_->supported(tickIndex_, q, slots_[q].letter, slotDepth_[q], owner, ownerAct, distance);
        }
    }
    gLetterFrontier_.fill(-1);
    for (std::size_t a = 0; a < slots_.size(); ++a)
    {
        if (gSlotLocked_[a]) gLetterFrontier_[slots_[a].letter] = std::max(gLetterFrontier_[slots_[a].letter], static_cast<int>(slotDepth_[a]));
    }
    // Freeze the tick-start epoch: every system reads these, and writes only its own row.
    world_.each<Identity, Pose, Motion, Mind>([this](Identity& id, Pose& pose, Motion& motion, Mind& mind) {
        const std::uint32_t i = id.index;
        gx_[i] = pose.x;
        gy_[i] = pose.y;
        gvx_[i] = motion.vx;
        gvy_[i] = motion.vy;
        gAct_[i] = static_cast<std::uint8_t>(mind.act);
        gMover_[i] = isMover(mind.act) ? 1 : 0;
        gWake_[i] = wakeTime_[i];
        gStall_[i] = mind.stall;
        gLayer_[i] = flows(pose, mind) ? 1 : 0;
        gPush_[i] = mind.pushTicks > 0 ? 1 : 0;
        gGoal_[i] = (mind.act == Act::Seek || (mind.act == Act::Lane && mind.laneS > 0.0F)) ? mind.goal : -1;
    });
    rebuildGrid(gx_, gy_, allOnes_);
    std::fill(cellMovers_.begin(), cellMovers_.end(), 0u);
    for (std::size_t c = 0; c < cellMovers_.size(); ++c)
    {
        for (std::uint32_t k = cellStart_[c]; k < cellStart_[c + 1]; ++k) cellMovers_[c] += gMover_[cellItems_[k]];
    }
}

void SwarmIntro::signalSystem()
{
    // The answer wave: a dormant unit wakes once a neighbour has been awake for one hop delay.
    // Reads only the frozen gather, so the wave is independent of iteration order.
    const float t = time_;
    const float reach = 1.6F * settings_.diameter;
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        const std::uint32_t i = id.index;
        if (mind.act == Act::Dormant)
        {
            bool wake = (i == first_ && t >= settings_.wakeAt);
            if (!wake && t >= settings_.answerAt)
            {
                forNeighbours(pose.x, pose.y, reach, [&](std::uint32_t j) {
                    if (gWake_[j] >= 0.0F && t - gWake_[j] >= hopDelay_ - 1e-4F) wake = true;
                });
            }
            if (wake)
            {
                wakeTime_[i] = t;
                mind.act = Act::Waking;
                mind.actSince = t;
            }
            return;
        }
        if (mind.act == Act::Waking && t >= settings_.answerAt && t - mind.actSince >= mind.reaction)
        {
            // Surplus units hold until the title is nearly formed (shot05), keeping lanes clear.
            if (mind.ribbon && t < settings_.ribbonAt) return;
            if (mind.goal < 0 && mind.letter < 0) mind.act = Act::Reserve;
            else mind.act = mind.ribbon ? Act::Moor : Act::Lane;
            mind.actSince = t;
        }
    });
}

SwarmIntro::Desire SwarmIntro::desire(const Identity& id, Pose& pose, Mind& mind)
{
    Desire d;
    const float cruise = settings_.cruiseSpeed * mind.speedScale;
    switch (mind.act)
    {
    case Act::Waking:
    {
        // Anticipation: a woken unit turns toward where it is about to go before it moves.
        if (mind.goal >= 0 || mind.letter >= 0)
        {
            const float tx = mind.ribbon ? ribbon_[static_cast<std::size_t>(mind.goal)].x : lanes_[mind.lane].xs.front();
            const float ty = mind.ribbon ? ribbon_[static_cast<std::size_t>(mind.goal)].y : lanes_[mind.lane].ys.front();
            d.faceHeading = std::atan2(ty - pose.y, tx - pose.x);
            d.hasFace = true;
        }
        break;
    }
    case Act::Lane:
    {
        const Lane& lane = lanes_[mind.lane];
        constexpr float kLookahead = 4.0F;
        float lx, ly, tx, ty;
        const float end = lane.cumulative.back();
        // Project onto the lane centre line inside a short forward window; progress is monotonic.
        float bestS = mind.laneS;
        float bestD = std::numeric_limits<float>::max();
        for (float s = mind.laneS; s <= std::min(end, mind.laneS + 8.0F); s += 0.5F)
        {
            lanePoint(lane, s, lx, ly, tx, ty);
            const float dd = (lx - pose.x) * (lx - pose.x) + (ly - pose.y) * (ly - pose.y);
            if (dd < bestD)
            {
                bestD = dd;
                bestS = s;
            }
        }
        mind.laneS = bestS;
        // Branch toward the slot once the lane's end is near (shot03).
        if (mind.laneS >= end - 3.0F)
        {
            mind.act = Act::Seek;
            mind.actSince = time_;
            return desire(id, pose, mind);
        }
        lanePoint(lane, mind.laneS + kLookahead, lx, ly, tx, ty);
        // Lane width tapers at the exit so the stream funnels into its letter group.
        const float taper = std::clamp((end - mind.laneS) / 12.0F, 0.35F, 1.0F);
        const float aimX = lx - ty * mind.laneOffset * taper;
        const float aimY = ly + tx * mind.laneOffset * taper;
        const float dx = aimX - pose.x, dy = aimY - pose.y;
        const float dist = std::max(std::hypot(dx, dy), 1e-4F);
        d.vx = dx / dist * cruise;
        d.vy = dy / dist * cruise;
        break;
    }
    case Act::Seek:
    case Act::Climb:
    case Act::Queue:
    case Act::Dock:
    case Act::Moor:
    {
        const bool ribbon = mind.ribbon;
        if (climbing() && !ribbon && mind.act != Act::Dock && climbDesire(id.index, pose, mind, d)) return d;
        const float sx = ribbon ? ribbon_[static_cast<std::size_t>(mind.goal)].x : slots_[static_cast<std::size_t>(mind.goal)].x;
        const float sy = ribbon ? ribbon_[static_cast<std::size_t>(mind.goal)].y : slots_[static_cast<std::size_t>(mind.goal)].y;
        const float sh = ribbon ? ribbon_[static_cast<std::size_t>(mind.goal)].heading
                                : seatHeading(slots_[static_cast<std::size_t>(mind.goal)].heading, pose.heading);
        const float dx = sx - pose.x, dy = sy - pose.y;
        const float dist = std::hypot(dx, dy);
        const float captureRadius = 1.5F * settings_.diameter;
        if ((mind.act == Act::Seek || mind.act == Act::Moor) && dist < captureRadius)
        {
            mind.act = Act::Dock;
            mind.actSince = time_;
        }
        if (mind.act == Act::Dock)
        {
            // Docking: slow, align with the stroke, slide in. Rotation leads translation.
            d.faceHeading = sh;
            d.hasFace = true;
            const float headingError = std::abs(wrapAngle(sh - pose.heading));
            const float speed = std::min(20.0F, dist * 24.0F) * (headingError > 0.6F ? 0.3F : 1.0F);
            if (dist > 1e-4F)
            {
                d.vx = dx / dist * speed;
                d.vy = dy / dist * speed;
            }
            return d;
        }
        // Arrival: slow down inside a short braking distance.
        const float speed = std::min(cruise, dist * 5.0F + 1.0F);
        if (dist > 1e-4F)
        {
            d.vx = dx / dist * speed;
            d.vy = dy / dist * speed;
        }
        if (ribbon && dist < 2.0F)
        {
            d.faceHeading = sh;
            d.hasFace = true;
        }
        break;
    }
    default:
        break;
    }
    return d;
}

bool SwarmIntro::climbDesire(std::uint32_t unit, Pose& pose, Mind& mind, Desire& d)
{
    // Climb-build title units: wait in a staging band until the slot is near its letter's
    // built frontier; walk to the wall base nearest the slot and step onto the wall face;
    // route over built units, never across open space. Returns false once the slot's support
    // is seated and the slot is within capture range: the caller then docks as usual.
    const float cruise = settings_.cruiseSpeed * mind.speedScale;
    const std::size_t goal = static_cast<std::size_t>(mind.goal);
    auto steer = [&](float tx, float ty, float speedCap, bool brake = true) {
        const float dx = tx - pose.x, dy = ty - pose.y;
        const float dist = std::hypot(dx, dy);
        // Waypoints keep momentum but never fling a unit past them within one tick's reach.
        const float speed = brake ? std::min(speedCap, dist * 5.0F + 0.5F)
                                  : std::min(speedCap, dist / settings_.tickSeconds * 0.5F + settings_.cruiseSpeed * 0.25F);
        if (dist > 1e-4F)
        {
            d.vx = dx / dist * speed;
            d.vy = dy / dist * speed;
        }
        return dist;
    };
    if (mind.goal < 0)
    {
        // Bound for a letter but no slot yet: walk to a staging spot in front of its base and
        // wait there (Queue) until the claim system gives it a slot, in arrival order.
        float qx, qy;
        stagingPoint(unit, mind, qx, qy);
        const float dist = steer(qx, qy, mind.act == Act::Queue ? cruise * 0.3F : cruise);
        if (mind.act == Act::Seek && dist < 2.0F)
        {
            mind.act = Act::Queue;
            mind.actSince = time_;
            mind.arrived = tickIndex_;
        }
        if (mind.act == Act::Queue)
        {
            d.faceHeading = kPi * 0.5F;   // face the wall
            d.hasFace = true;
        }
        return true;
    }
    if (mind.act == Act::Seek)
    {
        float bx, by;
        goalApproach(mind, bx, by);
        steer(bx, by, cruise);
        const std::uint8_t letterIndex = slots_[goal].letter;
        const bool atBase = std::abs(pose.y - by) < 1.2F && pose.x > letterBaseMin_[letterIndex] - 0.5F &&
                            pose.x < letterBaseMax_[letterIndex] + 0.5F;
        if (!atBase) return true;
        // Step across the seam wherever the unit meets the base: same world point, now on the
        // wall sheet, facing up.
        pose.y = wallBand_ + 0.05F;
        pose.heading = kPi * 0.5F;
        mind.act = Act::Climb;
        mind.actSince = time_;
        d = Desire{};
    }
    const float climbSpeed = cruise * 0.6F;
    const Slot& slot = slots_[goal];
    const float toGoal = std::hypot(slot.x - pose.x, slot.y - pose.y);
    if (supported(goal, gSlotSeating_) && toGoal < 2.0F * settings_.wordmark.pitch)
    {
        if (toGoal < 1.5F * settings_.diameter)
        {
            setMagnet(unit, mind, MagnetMode::Lift, false);
            setMagnet(unit, mind, MagnetMode::Seat, true);   // pulled into the supported slot
            mind.act = Act::Dock;
            mind.actSince = time_;
            return false;
        }
        steer(slot.x, slot.y, climbSpeed);
        return true;
    }
    // Close to an unsupported slot: cling one unit short of it, on the built side, ready to
    // seat the moment a neighbour below seats.
    // Close to its slot and the slot touches a seated or locked unit: cling to that unit's
    // edge, halfway toward the slot, so clinging stays on the built surface.
    std::int64_t anchor = -1;
    float anchorDistance = std::numeric_limits<float>::max();
    for (std::uint32_t b : slotNeighbours_[goal])
    {
        if (!gSlotSeating_[b]) continue;
        const float db = std::hypot(slots_[b].x - pose.x, slots_[b].y - pose.y);
        if (db < anchorDistance) { anchorDistance = db; anchor = b; }
    }
    if (anchor >= 0 && toGoal < 3.0F * settings_.wordmark.pitch)
    {
        setMagnet(unit, mind, MagnetMode::Lift, true);
        const Slot& a = slots_[static_cast<std::size_t>(anchor)];
        steer((a.x + slot.x) * 0.5F, (a.y + slot.y) * 0.5F, climbSpeed);
        d.faceHeading = seatHeading(slot.heading, pose.heading);
        d.hasFace = true;
        return true;
    }
    // Route over the built surface: step to the next locked unit on a path toward the slot.
    // Lift magnetism: in contact with the built face, the structure draws the climber up.
    const std::int64_t step = surfaceStep(pose, goal);
    const bool contact = step >= 0 && std::hypot(slots_[static_cast<std::size_t>(step)].x - pose.x,
                                                  slots_[static_cast<std::size_t>(step)].y - pose.y) < 2.0F * settings_.wordmark.pitch;
    const float speed = contact ? climbSpeed * kLiftBoost : climbSpeed;
    setMagnet(unit, mind, MagnetMode::Lift, contact);
    // Waypoints carry momentum; only the final approach brakes.
    if (step >= 0) steer(slots_[static_cast<std::size_t>(step)].x, slots_[static_cast<std::size_t>(step)].y, speed, false);
    else steer(slots_[slotRoot_[goal]].x, wallBand_ + 0.05F, climbSpeed);   // nothing built yet: wait at the base
    // Anticipation: close to its slot, a waiting climber already turns to the stroke's
    // heading, so it can slide in the moment its support seats.
    if (toGoal < 3.0F * settings_.wordmark.pitch)
    {
        d.faceHeading = seatHeading(slot.heading, pose.heading);
        d.hasFace = true;
    }
    return true;
}

std::int64_t SwarmIntro::surfaceStep(const Pose& pose, std::size_t goal)
{
    // Shortest-path search over the letter's locked slots (the frozen gather) and the ground
    // between its strokes, from the slot the climber stands on to a locked support of its goal, or failing that to the locked
    // slot nearest the goal. Returns the next slot on that path, or -1 when nothing is built.
    const std::vector<std::uint32_t>& letter = letterSlots_[slots_[goal].letter];
    const float pitch = settings_.wordmark.pitch;
    std::int64_t standing = -1;
    float best = 2.0F * pitch;
    for (std::uint32_t a : letter)
    {
        if (!gSlotSeating_[a]) continue;
        const float d = std::hypot(slots_[a].x - pose.x, slots_[a].y - pose.y);
        if (d < best)
        {
            best = d;
            standing = a;
        }
    }
    if (standing < 0)
    {
        // Between strokes on the base: route from the nearest built root, so walking along the
        // base continues toward the goal instead of returning to that root.
        float nearest = std::numeric_limits<float>::max();
        for (std::uint32_t a : letter)
        {
            if (!gSlotSeating_[a] || slotDepth_[a] != 0) continue;
            const float d = std::hypot(slots_[a].x - pose.x, slots_[a].y - pose.y);
            if (d < nearest)
            {
                nearest = d;
                standing = a;
            }
        }
        if (standing < 0) return -1;
    }
    // Dijkstra by path length (hop counts alone send climbers back and forth along a base).
    constexpr std::uint32_t kUnseen = std::numeric_limits<std::uint32_t>::max();
    for (std::uint32_t a : letter)
    {
        routeParent_[a] = kUnseen;
        routeCost_[a] = std::numeric_limits<float>::max();
    }
    using Entry = std::pair<float, std::uint32_t>;
    routeHeap_.clear();
    auto push = [this](float cost, std::uint32_t a) {
        routeHeap_.push_back({ cost, a });
        std::push_heap(routeHeap_.begin(), routeHeap_.end(), std::greater<Entry>{});
    };
    const auto start = static_cast<std::uint32_t>(standing);
    routeCost_[start] = 0.0F;
    routeParent_[start] = start;
    push(0.0F, start);
    std::uint32_t target = start;
    float targetDistance = std::numeric_limits<float>::max();
    auto relax = [&](std::uint32_t a, std::uint32_t b) {
        const float cost = routeCost_[a] + std::hypot(slots_[a].x - slots_[b].x, slots_[a].y - slots_[b].y);
        if (cost + 1e-5F >= routeCost_[b]) return;
        routeCost_[b] = cost;
        routeParent_[b] = a;
        push(cost, b);
    };
    while (!routeHeap_.empty())
    {
        std::pop_heap(routeHeap_.begin(), routeHeap_.end(), std::greater<Entry>{});
        const auto [cost, a] = routeHeap_.back();
        routeHeap_.pop_back();
        if (cost > routeCost_[a]) continue;
        if (std::find(slotNeighbours_[goal].begin(), slotNeighbours_[goal].end(), a) != slotNeighbours_[goal].end())
        {
            target = a;   // nearest supporter by path length
            break;
        }
        const float d = std::hypot(slots_[a].x - slots_[goal].x, slots_[a].y - slots_[goal].y);
        if (d < targetDistance)
        {
            targetDistance = d;
            target = a;
        }
        for (std::uint32_t b : slotNeighbours_[a])
        {
            if (gSlotSeating_[b]) relax(a, b);
        }
        // Strokes of one letter meet only at the ground: locked ground-row slots are linked
        // by walking along the wall's base.
        if (slotDepth_[a] != 0) continue;
        for (std::uint32_t b : letter)
        {
            if (slotDepth_[b] == 0 && gSlotSeating_[b]) relax(a, b);
        }
    }
    // Walk back to the first step after the standing slot.
    std::uint32_t at = target;
    while (at != start && routeParent_[at] != start) at = routeParent_[at];
    return at;
}

void SwarmIntro::advanceAct(std::uint32_t unit, Pose& pose, Mind& mind)
{
    if (onWall(mind)) pose.y = std::max(pose.y, wallBand_);   // the wall sheet's floor is the ground
    if (mind.act != Act::Dock) return;
    const std::size_t goal = static_cast<std::size_t>(mind.goal);
    const float sx = mind.ribbon ? ribbon_[goal].x : slots_[goal].x;
    const float sy = mind.ribbon ? ribbon_[goal].y : slots_[goal].y;
    const float sh = mind.ribbon ? ribbon_[goal].heading : seatHeading(slots_[goal].heading, pose.heading);
    const bool structural = mind.ribbon || !climbing() || supported(goal, slotLocked_);
    if (structural && std::hypot(sx - pose.x, sy - pose.y) < 0.12F && std::abs(wrapAngle(sh - pose.heading)) < 0.15F)
    {
        pose = { sx, sy, 0.0F, sh };
        mind.act = mind.ribbon ? Act::Moored : Act::Locked;
        mind.actSince = time_;
        setMagnet(unit, mind, MagnetMode::Seat, false);
        if (!mind.ribbon)
        {
            ++metrics_.dockClicks;
            if (diagnostics_) diagnostics_->lock(tickIndex_, unit, goal, slots_[goal].letter, slotDepth_[goal]);
            if (climbing()) --letterInFlight_[slots_[goal].letter];
            if (climbing() && !supported(goal, slotLocked_)) ++metrics_.unsupported;
            slotLocked_[goal] = 1;
            const std::uint8_t letter = slots_[goal].letter;
            if (++letterLocked_[letter] == letterSlots_[letter].size())
            {
                letterCompleteAt_[letter] = time_;
                if (diagnostics_) diagnostics_->letter(tickIndex_, letter, 1);
            }
        }
    }
}

void SwarmIntro::integrate(Pose& pose, Motion& motion, float vx, float vy, const Desire& d, float turnRate)
{
    const float dt = settings_.tickSeconds;
    const float accel = 140.0F * dt;
    float ax = vx - motion.vx, ay = vy - motion.vy;
    const float a = std::hypot(ax, ay);
    if (a > accel)
    {
        ax *= accel / a;
        ay *= accel / a;
    }
    motion.vx += ax;
    motion.vy += ay;
    pose.x += motion.vx * dt;
    pose.y += motion.vy * dt;
    const float speed = std::hypot(motion.vx, motion.vy);
    float face = pose.heading;
    if (d.hasFace) face = d.faceHeading;
    else if (speed > 0.5F) face = std::atan2(motion.vy, motion.vx);
    const float err = wrapAngle(face - pose.heading);
    const float step = turnRate * dt;
    pose.heading = wrapAngle(pose.heading + std::clamp(err, -step, step));
}

void SwarmIntro::heroChoose(const Identity& id, const Pose& pose, float& vx, float& vy)
{
    // Sampled velocity obstacles: score candidate velocities by progress and the predicted
    // time to collision with nearby senior movers. Yielding and side-stepping emerge.
    const float desiredSpeed = std::hypot(vx, vy);
    if (desiredSpeed < 0.5F) return;
    const float baseAngle = std::atan2(vy, vx);
    const float d = settings_.diameter;
    const float horizon = 0.8F;
    struct Neighbour
    {
        float px, py, vx, vy;
    };
    std::array<Neighbour, 24> near{};
    std::size_t count = 0;
    forNeighbours(pose.x, pose.y, 4.0F * d, [&](std::uint32_t j) {
        if (j == id.index || !gMover_[j] || gLayer_[j] || count == near.size()) return;
        // Right of way: yield only to units that answered the wave earlier. Mutual yielding
        // between equals freezes dense streams (measured: all-hero runs jam without this).
        const bool senior = gWake_[j] < gWake_[id.index] || (gWake_[j] == gWake_[id.index] && j < id.index);
        if (!senior) return;
        near[count++] = { gx_[j] - pose.x, gy_[j] - pose.y, gvx_[j], gvy_[j] };
    });
    if (count == 0) return;

    constexpr std::array<float, 13> kAngles{ 0.0F, 0.3F, -0.3F, 0.6F, -0.6F, 1.0F, -1.0F, 1.6F, -1.6F, 2.4F, -2.4F, kPi, 0.15F };
    constexpr std::array<float, 3> kSpeeds{ 1.0F, 0.6F, 0.3F };
    const float collisionWeight = desiredSpeed * desiredSpeed * 0.2F;
    float bestScore = std::numeric_limits<float>::max();
    float bestX = 0.0F, bestY = 0.0F;
    auto score = [&](float cx, float cy) {
        float ttc = horizon;
        for (std::size_t k = 0; k < count; ++k)
        {
            const Neighbour& n = near[k];
            const float wx = cx - n.vx;   // seniors do not reciprocate
            const float wy = cy - n.vy;
            const float b = n.px * wx + n.py * wy;
            if (b <= 0.0F) continue;   // separating
            const float c = n.px * n.px + n.py * n.py - d * d;
            if (c <= 0.0F)
            {
                ttc = 0.0F;
                break;
            }
            const float aa = wx * wx + wy * wy;
            const float disc = b * b - aa * c;
            if (disc < 0.0F) continue;
            ttc = std::min(ttc, (b - std::sqrt(disc)) / aa);
        }
        const float ex = cx - vx, ey = cy - vy;
        float s = ex * ex + ey * ey;
        if (ttc < horizon) s += collisionWeight / std::max(ttc, 0.05F);
        return s;
    };
    for (float angle : kAngles)
    {
        for (float scale : kSpeeds)
        {
            const float cx = std::cos(baseAngle + angle) * desiredSpeed * scale;
            const float cy = std::sin(baseAngle + angle) * desiredSpeed * scale;
            const float s = score(cx, cy);
            if (s < bestScore)
            {
                bestScore = s;
                bestX = cx;
                bestY = cy;
            }
        }
    }
    if (score(0.0F, 0.0F) < bestScore)
    {
        bestX = 0.0F;
        bestY = 0.0F;
    }
    if (std::hypot(bestX, bestY) < 0.4F * desiredSpeed) ++metrics_.heroYields;
    vx = bestX;
    vy = bestY;
}

void SwarmIntro::behaviourSystems()
{
    const float d = settings_.diameter;
    const float dt = settings_.tickSeconds;

    // Hero: full per-unit decision making (sampled RVO, anticipation, climb, nuanced turns).
    world_.each<Identity, Pose, Motion, Mind, HeroTier>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind, HeroTier&) {
        if (restAct(mind.act)) return;
        const Desire want = desire(id, pose, mind);
        float vx = want.vx, vy = want.vy;
        pushAway(id.index, pose, vx, vy);
        if (mind.act != Act::Dock && !flows(pose, mind)) heroChoose(id, pose, vx, vy);
        integrate(pose, motion, vx, vy, want, 9.0F);
        pose.z += (liftOf(id, pose, mind) - pose.z) * std::min(1.0F, dt * 12.0F);
        advanceAct(id.index, pose, mind);
    });

    // Near: reactive boids-style separation from movers; climb over the rest layer.
    world_.each<Identity, Pose, Motion, Mind, NearTier>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind, NearTier&) {
        if (restAct(mind.act)) return;
        const Desire want = desire(id, pose, mind);
        float vx = want.vx, vy = want.vy;
        const float reach = 1.6F * d;
        // Docking units hold their line; climbers flow in layers instead of separating.
        const bool committed = mind.act == Act::Dock || flows(pose, mind);
        if (!committed)
        {
            pushAway(id.index, pose, vx, vy);
        }
        forNeighbours(pose.x, pose.y, reach, [&](std::uint32_t j) {
            if (j == id.index || committed || !gMover_[j] || gLayer_[j]) return;
            const float dx = pose.x - gx_[j], dy = pose.y - gy_[j];
            const float dist = std::max(std::hypot(dx, dy), 1e-3F);
            const float push = (reach - dist) / reach * settings_.cruiseSpeed * 0.8F;
            vx += dx / dist * push;
            vy += dy / dist * push;
        });
        integrate(pose, motion, vx, vy, want, 14.0F);
        pose.z += (liftOf(id, pose, mind) - pose.z) * std::min(1.0F, dt * 12.0F);
        advanceAct(id.index, pose, mind);
    });

    // Far: O(1) crowd response from the per-cell mover density gradient (continuum-style).
    world_.each<Identity, Pose, Motion, Mind, FarTier>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind, FarTier&) {
        if (restAct(mind.act)) return;
        const Desire want = desire(id, pose, mind);
        float vx = want.vx, vy = want.vy;
        const int cx = std::clamp(static_cast<int>((pose.x - gridMinX_) / cell_), 1, gridW_ - 2);
        const int cy = std::clamp(static_cast<int>((pose.y - gridMinY_) / cell_), 1, gridH_ - 2);
        auto at = [&](int x, int y) { return static_cast<float>(cellMovers_[static_cast<std::size_t>(y) * static_cast<std::size_t>(gridW_) + static_cast<std::size_t>(x)]); };
        const float gradX = at(cx + 1, cy) - at(cx - 1, cy);
        const float gradY = at(cx, cy + 1) - at(cx, cy - 1);
        if (mind.act != Act::Dock && !flows(pose, mind))
        {
            vx -= gradX * 1.5F;
            vy -= gradY * 1.5F;
            pushAway(id.index, pose, vx, vy);
        }
        integrate(pose, motion, vx, vy, want, 20.0F);
        pose.z = flows(pose, mind) ? 0.55F : 0.0F;
        advanceAct(id.index, pose, mind);
    });

    // Hidden: analytic goal motion only; no neighbour queries and no overlap handling.
    world_.each<Identity, Pose, Motion, Mind, HiddenTier>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind, HiddenTier&) {
        if (restAct(mind.act)) return;
        const Desire want = desire(id, pose, mind);
        motion.vx = want.vx;
        motion.vy = want.vy;
        pose.x += motion.vx * dt;
        pose.y += motion.vy * dt;
        if (want.hasFace) pose.heading = want.faceHeading;
        else if (std::hypot(motion.vx, motion.vy) > 0.5F) pose.heading = std::atan2(motion.vy, motion.vx);
        pose.z = flows(pose, mind) ? 0.55F : 0.0F;
        advanceAct(id.index, pose, mind);
    });
}

float SwarmIntro::liftOf(const Identity& id, const Pose& pose, const Mind& mind) const
{
    // Lift off the sheet: over the rest layer one layer up; a climber over a lower-identity
    // climber one more layer (a deterministic stacking order).
    const float d = settings_.diameter;
    bool over = false;
    bool stacked = false;
    forNeighbours(pose.x, pose.y, 0.8F * d, [&](std::uint32_t j) {
        const Act other = static_cast<Act>(gAct_[j]);
        over = over || restAct(other);
        stacked = stacked || (flows(pose, mind) && j < id.index && gLayer_[j]);
    });
    return stacked ? 1.1F : (over || flows(pose, mind) ? 0.55F : 0.0F);
}

void SwarmIntro::negotiationSystem()
{
    // Decentralised goal exchange (d-CAPT style): two nearby seekers of the same lane trade
    // slots when that lowers their summed squared distance, so streams stop crossing.
    // A serial pass in identity order over the frozen gather; a unit trades at most once.
    const float reach = 2.0F * settings_.diameter;
    std::vector<std::uint8_t>& traded = include_;
    std::fill(traded.begin(), traded.end(), 0);
    for (std::uint32_t i = 0; i < gGoal_.size(); ++i)
    {
        if (gGoal_[i] < 0 || traded[i] || static_cast<Act>(gAct_[i]) != Act::Seek) continue;
        forNeighbours(gx_[i], gy_[i], reach, [&](std::uint32_t j) {
            if (traded[i] || j <= i || traded[j] || gGoal_[j] < 0) return;
            const Act aj = static_cast<Act>(gAct_[j]);
            if (aj != Act::Seek && aj != Act::Lane) return;
            Mind* mi = world_.find<Mind>(entities_[i]);
            Mind* mj = world_.find<Mind>(entities_[j]);
            if (mi->act != Act::Seek || (mj->act != Act::Seek && mj->act != Act::Lane) || mi->lane != mj->lane) return;
            Slot si, sj;
            goalApproach(*mi, si.x, si.y);
            goalApproach(*mj, sj.x, sj.y);
            auto d2 = [](float ax, float ay, const Slot& s) { return (ax - s.x) * (ax - s.x) + (ay - s.y) * (ay - s.y); };
            const float now = d2(gx_[i], gy_[i], si) + d2(gx_[j], gy_[j], sj);
            const float swapped = d2(gx_[i], gy_[i], sj) + d2(gx_[j], gy_[j], si);
            // In the climb build, a swap must not move a unit to a deeper slot ahead of its support order.
            const bool sameDepth = !climbing() || slotDepth_[static_cast<std::size_t>(mi->goal)] == slotDepth_[static_cast<std::size_t>(mj->goal)];
            if (sameDepth && swapped + 0.5F < now)
            {
                std::swap(mi->goal, mj->goal);
                if (climbing())
                {
                    slotOwner_[static_cast<std::size_t>(mi->goal)] = static_cast<std::int32_t>(i);
                    slotOwner_[static_cast<std::size_t>(mj->goal)] = static_cast<std::int32_t>(j);
                }
                traded[i] = traded[j] = 1;
                ++metrics_.goalSwaps;
            }
        });
    }
}

void SwarmIntro::stagingPoint(std::uint32_t index, const Mind& mind, float& x, float& y) const
{
    // Spread along the letter's base by a per-unit offset, a few units in front of the wall.
    const std::vector<std::uint32_t>& letter = letterSlots_[static_cast<std::size_t>(mind.letter)];
    float minX = std::numeric_limits<float>::max(), maxX = -minX;
    for (std::uint32_t a : letter)
    {
        minX = std::min(minX, slots_[a].x);
        maxX = std::max(maxX, slots_[a].x);
    }
    const float spread = unit(mix(index), 7);
    x = minX + (maxX - minX) * spread;
    y = wallY_ - 4.0F - 2.0F * unit(mix(index), 8);
}

void SwarmIntro::setMagnet(std::uint32_t unit, Mind& mind, MagnetMode mode, bool on)
{
    // Selective magnetism: one mode at a time; transitions are recorded.
    const auto code = static_cast<std::uint8_t>(mode);
    if (on == (mind.magnet == code)) return;
    if (!on && mind.magnet != code) return;
    if (on && mind.magnet != 0 && diagnostics_) diagnostics_->magnet(tickIndex_, unit, static_cast<MagnetMode>(mind.magnet), false);
    mind.magnet = on ? code : 0;
    if (diagnostics_) diagnostics_->magnet(tickIndex_, unit, mode, on);
}

void SwarmIntro::pushAway(std::uint32_t unit, const Pose& pose, float& vx, float& vy) const
{
    // A pushing neighbour's repulsion: movers step aside to clear a stuck unit's path.
    const float reach = 2.5F * settings_.diameter;
    forNeighbours(pose.x, pose.y, reach, [&](std::uint32_t j) {
        if (j == unit || !gPush_[j]) return;
        const float dx = pose.x - gx_[j], dy = pose.y - gy_[j];
        const float dist = std::max(std::hypot(dx, dy), 1e-3F);
        const float push = (reach - dist) / reach * settings_.cruiseSpeed * 1.2F;
        vx += dx / dist * push;
        vy += dy / dist * push;
    });
}

bool SwarmIntro::progressOf(const Pose& pose, const Mind& mind, float& value) const
{
    // A scalar that rises while a unit gets closer to finishing its current act. Units that
    // legitimately wait (queueing, waking, resting, climbers whose slot has no support yet)
    // are not tracked.
    switch (mind.act)
    {
    case Act::Lane:
    {
        // Lane progress plus closing in on the lane: units still leaving the bed are moving too.
        float lx, ly, tx, ty;
        lanePoint(lanes_[mind.lane], mind.laneS, lx, ly, tx, ty);
        value = mind.laneS - std::hypot(lx - pose.x, ly - pose.y);
        return true;
    }
    case Act::Seek:
    case Act::Dock:
    case Act::Moor:
    {
        if (mind.goal < 0) return false;
        float gx, gy;
        if (mind.ribbon) { gx = ribbon_[static_cast<std::size_t>(mind.goal)].x; gy = ribbon_[static_cast<std::size_t>(mind.goal)].y; }
        else if (mind.act == Act::Seek) goalApproach(mind, gx, gy);
        else { gx = slots_[static_cast<std::size_t>(mind.goal)].x; gy = slots_[static_cast<std::size_t>(mind.goal)].y; }
        value = -std::hypot(gx - pose.x, gy - pose.y);
        return true;
    }
    case Act::Climb:
    {
        const std::size_t goal = static_cast<std::size_t>(mind.goal);
        if (!supported(goal, gSlotSeating_)) return false;
        value = -std::hypot(slots_[goal].x - pose.x, slots_[goal].y - pose.y);
        return true;
    }
    default:
        return false;
    }
}

void SwarmIntro::claimSystem()
{
    // Climb build, first come first served: units queued at a letter's base claim slots in
    // arrival order. A slot is claimable once it could be supported soon: on the ground row,
    // or with a lower neighbour already locked or claimed. Each letter keeps at most claimCap
    // claims in flight. Serial and in (arrival, identity) order, so it is deterministic.
    if (!climbing()) return;
    for (std::uint8_t letter = 0; letter < 8; ++letter)
    {
        if (letterInFlight_[letter] >= settings_.claimCap) continue;
        queueScratch_.clear();
        world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose&, Motion&, Mind& mind) {
            if (mind.act == Act::Queue && mind.letter == static_cast<std::int8_t>(letter)) queueScratch_.push_back({ mind.arrived, id.index });
        });
        if (queueScratch_.empty()) continue;
        std::sort(queueScratch_.begin(), queueScratch_.end());
        std::uint32_t rank = 0;
        for (const auto& [arrived, unitIndex] : queueScratch_)
        {
            if (letterInFlight_[letter] >= settings_.claimCap) break;
            Pose* pose = world_.find<Pose>(entities_[unitIndex]);
            Mind* mind = world_.find<Mind>(entities_[unitIndex]);
            std::int64_t best = -1;
            float bestScore = std::numeric_limits<float>::max();
            for (std::uint32_t a : letterSlots_[letter])
            {
                if (slotOwner_[a] >= 0 || slotLocked_[a]) continue;
                bool claimable = slotDepth_[a] == 0;
                for (std::uint32_t b : slotNeighbours_[a])
                {
                    claimable = claimable || (slotDepth_[b] < slotDepth_[a] && (slotLocked_[b] || slotOwner_[b] >= 0));
                }
                if (!claimable) continue;
                // Lowest level first, then the slot nearest along the base.
                const float score = static_cast<float>(slotDepth_[a]) * 1000.0F + std::abs(slots_[a].x - pose->x);
                if (score < bestScore)
                {
                    bestScore = score;
                    best = a;
                }
            }
            if (best < 0) break;
            slotOwner_[static_cast<std::size_t>(best)] = static_cast<std::int32_t>(unitIndex);
            ++letterInFlight_[letter];
            mind->goal = static_cast<std::int32_t>(best);
            mind->act = Act::Seek;
            mind->actSince = time_;
            ++metrics_.claims;
            ++claimsTotal_;
            if (diagnostics_)
            {
                diagnostics_->claim(tickIndex_, unitIndex, static_cast<std::uint64_t>(best), letter, slotDepth_[static_cast<std::size_t>(best)],
                                    rank, tickIndex_ - arrived);
            }
            ++rank;
        }
    }
}

void SwarmIntro::contingencySystem()
{
    // Stuck intelligence. A unit whose progress stalls for stuckTicks decides what to do:
    // Wait when a senior unit beside it is still moving (it is simply queued behind it);
    // otherwise Push, switching on repelling magnetism briefly so juniors and idle units step
    // aside. Still stuck after three times as long, a seeker or climber Reclaims: it gives up
    // its slot and takes a free one (rejoining the queue on the ground). Serial, identity order.
    const std::uint32_t limit = settings_.stuckTicks;
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        const std::uint32_t i = id.index;
        if (mind.pushTicks > 0 && --mind.pushTicks == 0) setMagnet(i, mind, MagnetMode::Push, false);
        float value = 0.0F;
        const bool tracked = progressOf(pose, mind, value);
        const auto act = static_cast<std::uint8_t>(mind.act);
        if (!tracked || act != mind.progressAct)
        {
            if (mind.stuckSince != 0 && diagnostics_) diagnostics_->recovered(tickIndex_, i, act, tickIndex_ - mind.stuckSince);
            if (mind.stuckSince != 0) { ++metrics_.recovered; ++recoveredTotal_; }
            mind.stuckSince = 0;
            mind.stall = 0;
            mind.progressAct = act;
            mind.best = tracked ? value : 0.0F;
            return;
        }
        if (value > mind.best + 0.05F)
        {
            mind.best = value;
            mind.stall = 0;
            if (mind.stuckSince != 0)
            {
                if (diagnostics_) diagnostics_->recovered(tickIndex_, i, act, tickIndex_ - mind.stuckSince);
                ++metrics_.recovered;
                ++recoveredTotal_;
                mind.stuckSince = 0;
            }
            return;
        }
        ++mind.stall;
        if (mind.stall == limit)
        {
            bool seniorMoving = false;
            forNeighbours(pose.x, pose.y, 1.5F * settings_.diameter, [&](std::uint32_t j) {
                if (j == i || !gMover_[j]) return;
                const bool senior = gWake_[j] < gWake_[i] || (gWake_[j] == gWake_[i] && j < i);
                seniorMoving = seniorMoving || (senior && gStall_[j] < limit / 2);
            });
            const Contingency choice = seniorMoving ? Contingency::Wait : Contingency::Push;
            if (choice == Contingency::Push)
            {
                mind.pushTicks = 30;
                setMagnet(i, mind, MagnetMode::Push, true);
                ++metrics_.pushes;
            }
            mind.stuckSince = tickIndex_;
            ++metrics_.stuck;
            ++stuckTotal_;
            if (diagnostics_) diagnostics_->stuck(tickIndex_, i, act, mind.stall, choice);
        }
        else if (mind.stall == 3 * limit && climbing() && !mind.ribbon && mind.goal >= 0 &&
                 (mind.act == Act::Seek || mind.act == Act::Climb))
        {
            // Release the slot. A seeker rejoins its letter's queue; a climber stays on the face
            // and the claim system will hand it a slot again from the queue.
            const std::size_t goal = static_cast<std::size_t>(mind.goal);
            slotOwner_[goal] = -1;
            --letterInFlight_[slots_[goal].letter];
            setMagnet(i, mind, MagnetMode::Lift, false);
            if (mind.act == Act::Seek)
            {
                mind.goal = -1;
                mind.act = Act::Queue;
                mind.actSince = time_;
                mind.arrived = tickIndex_;
            }
            else
            {
                // Climber: take the nearest claimable free slot of its letter, if any.
                std::int64_t best = -1;
                float bestDistance = std::numeric_limits<float>::max();
                for (std::uint32_t a : letterSlots_[slots_[goal].letter])
                {
                    if (a == goal || slotOwner_[a] >= 0 || slotLocked_[a] || !supported(a, gSlotSeating_)) continue;
                    const float dd = std::hypot(slots_[a].x - pose.x, slots_[a].y - pose.y);
                    if (dd < bestDistance) { bestDistance = dd; best = a; }
                }
                const std::size_t next = best >= 0 ? static_cast<std::size_t>(best) : goal;
                slotOwner_[next] = static_cast<std::int32_t>(i);
                ++letterInFlight_[slots_[next].letter];
                mind.goal = static_cast<std::int32_t>(next);
            }
            ++metrics_.reclaims;
            if (diagnostics_) diagnostics_->stuck(tickIndex_, i, act, mind.stall, Contingency::Reclaim);
            mind.stall = 0;
            mind.best = -std::numeric_limits<float>::max();
        }
    });
}

void SwarmIntro::recordSummary()
{
    if (!diagnostics_) return;
    diagnostics_->summary(tickIndex_, slots_.size(), metrics_.locked, metrics_.moored, claimsTotal_, stuckTotal_, recoveredTotal_);
}

void SwarmIntro::resolveSystem()
{
    // Non-penetration: kResolvePasses Jacobi projection passes over visible-tier movers (position-based
    // dynamics). Hidden units skip it; the view margin gives them ticks to separate before
    // they can be seen.
    std::vector<std::uint8_t>& include = include_;
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        const std::uint32_t i = id.index;
        gx_[i] = pose.x;
        gy_[i] = pose.y;
        include[i] = (isMover(mind.act) && !flows(pose, mind) && gTier_[i] != static_cast<std::uint8_t>(Tier::Hidden)) ? 1 : 0;
    });
    rebuildGrid(gx_, gy_, include);
    const float d = settings_.diameter;
    for (int pass = 0; pass < kResolvePasses; ++pass)
    {
        std::fill(correctionX_.begin(), correctionX_.end(), 0.0F);
        std::fill(correctionY_.begin(), correctionY_.end(), 0.0F);
        for (std::size_t c = 0; c + 1 < cellStart_.size(); ++c)
        {
            for (std::uint32_t k = cellStart_[c]; k < cellStart_[c + 1]; ++k)
            {
                const std::uint32_t i = cellItems_[k];
                forNeighbours(gx_[i], gy_[i], d, [&](std::uint32_t j) {
                    if (j == i) return;
                    const float dx = gx_[i] - gx_[j], dy = gy_[i] - gy_[j];
                    const float dist = std::hypot(dx, dy);
                    if (dist >= d) return;
                    // Coincident centres separate along an identity-derived direction.
                    float nx = dx, ny = dy;
                    if (dist < 1e-5F)
                    {
                        const float a = static_cast<float>(i % 97u) * 0.37F;
                        nx = std::cos(a);
                        ny = std::sin(a);
                    }
                    else
                    {
                        nx /= dist;
                        ny /= dist;
                    }
                    const float push = (d - dist) * 0.5F;
                    correctionX_[i] += nx * push;
                    correctionY_[i] += ny * push;
                });
            }
        }
        for (std::size_t i = 0; i < gx_.size(); ++i)
        {
            gx_[i] += correctionX_[i];
            gy_[i] += correctionY_[i];
        }
    }
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        const std::uint32_t i = id.index;
        if (!include[i]) return;
        pose.x = gx_[i];
        pose.y = onWall(mind) ? std::max(gy_[i], wallBand_) : gy_[i];
    });
}

void SwarmIntro::lodSystem()
{
    // Decision LOD from the camera: who earns full per-unit intelligence this tick.
    const Camera& cam = camera_;
    const float margin = 0.15F * cam.halfWidth + 4.0F * settings_.diameter;
    const float ppu = cam.pixelsPerUnit();
    heroCandidates_.clear();
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        const std::uint32_t i = id.index;
        Tier want = Tier::Far;
        switch (settings_.lod)
        {
        case LodMode::AllHero: want = Tier::Hero; break;
        case LodMode::AllNear: want = Tier::Near; break;
        case LodMode::AllFar: want = Tier::Far; break;
        case LodMode::Camera:
        {
            float vx, vy;
            viewOf(pose, mind, vx, vy);
            if (!inView(vx, vy, margin))
            {
                want = Tier::Hidden;
                break;
            }
            const float rx = (vx - cam.cx) / cam.halfWidth;
            const float ry = (vy - cam.cy) / cam.halfHeight();
            const float r = std::hypot(rx, ry);
            want = (ppu >= 6.0F && r < 0.8F) ? Tier::Near : Tier::Far;
            if (ppu >= 10.0F && r < 0.45F && !restAct(mind.act)) heroCandidates_.push_back({ r, i });
            break;
        }
        }
        desiredTier_[i] = static_cast<std::uint8_t>(want);
    });
    if (settings_.lod == LodMode::Camera)
    {
        const std::size_t cap = std::min(settings_.heroCap, heroCandidates_.size());
        std::partial_sort(heroCandidates_.begin(), heroCandidates_.begin() + static_cast<std::ptrdiff_t>(cap), heroCandidates_.end());
        for (std::size_t k = 0; k < cap; ++k) desiredTier_[heroCandidates_[k].second] = static_cast<std::uint8_t>(Tier::Hero);
    }

    // Hysteresis: a change must persist for a few ticks, except leaving Hidden, which is
    // immediate so units are interacting before they reach the visible frame. Hero
    // promotions respect the cap, after demotions.
    std::uint32_t heroes = 0;
    for (std::uint8_t t : gTier_) heroes += t == static_cast<std::uint8_t>(Tier::Hero) ? 1u : 0u;
    const std::size_t heroCap = settings_.lod == LodMode::AllHero ? gTier_.size() : settings_.heroCap;
    auto migrate = [&](std::uint32_t i, Tier from, Tier to) {
        const sub0ecs::Entity e = entities_[i];
        switch (from)
        {
        case Tier::Hero: world_.remove<HeroTier>(e); --heroes; break;
        case Tier::Near: world_.remove<NearTier>(e); break;
        case Tier::Far: world_.remove<FarTier>(e); break;
        case Tier::Hidden: world_.remove<HiddenTier>(e); break;
        }
        switch (to)
        {
        case Tier::Hero: world_.add(e, HeroTier{ tickIndex_ }); ++heroes; break;
        case Tier::Near: world_.add(e, NearTier{ tickIndex_ }); break;
        case Tier::Far: world_.add(e, FarTier{ tickIndex_ }); break;
        case Tier::Hidden: world_.add(e, HiddenTier{ tickIndex_ }); break;
        }
        gTier_[i] = static_cast<std::uint8_t>(to);
        pendingTicks_[i] = 0;
        ++metrics_.migrations;
    };
    for (int round = 0; round < 2; ++round)   // round 0: everything but hero promotion
    {
        for (std::uint32_t i = 0; i < gTier_.size(); ++i)
        {
            const Tier from = static_cast<Tier>(gTier_[i]);
            const Tier to = static_cast<Tier>(desiredTier_[i]);
            if (from == to)
            {
                pendingTicks_[i] = 0;
                continue;
            }
            if ((to == Tier::Hero) != (round == 1)) continue;
            if (round == 0) pendingTicks_[i] = static_cast<std::uint8_t>(std::min(255, pendingTicks_[i] + 1));
            const bool immediate = from == Tier::Hidden || settings_.lod != LodMode::Camera;
            if (!immediate && pendingTicks_[i] < 6 && round == 0) continue;
            if (to == Tier::Hero && heroes >= heroCap) continue;
            migrate(i, from, to);
        }
    }
}

void SwarmIntro::metricsSystem()
{
    std::vector<std::uint8_t>& include = include_;
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        const std::uint32_t i = id.index;
        gx_[i] = pose.x;
        gy_[i] = pose.y;
        float vx, vy;
        viewOf(pose, mind, vx, vy);
        include[i] = (isMover(mind.act) && !flows(pose, mind) && inView(vx, vy, 0.0F)) ? 1 : 0;
        metrics_.locked += mind.act == Act::Locked ? 1u : 0u;
        metrics_.queued += mind.act == Act::Queue ? 1u : 0u;
        metrics_.climbers += mind.act == Act::Climb ? 1u : 0u;
        metrics_.lifted += mind.magnet == static_cast<std::uint8_t>(MagnetMode::Lift) ? 1u : 0u;
        metrics_.stuckNow += mind.stuckSince != 0 ? 1u : 0u;
        // Climb-build invariant: a climber high on the wall is always on the built surface
        // (locked units, or units seated in their slot this tick).
        if (mind.act == Act::Climb && pose.y - wallBand_ > 1.2F)
        {
            const std::uint8_t letter = slots_[static_cast<std::size_t>(mind.goal)].letter;
            // Attached when touching the built face: rows within 1.5 pitch of a built unit;
            // tiles within a tile's circumradius plus one unit (an overhang of at most a body).
            const float tile = settings_.wordmark.tile;
            const float reach = tile > 0.0F ? tile / std::sqrt(3.0F) + settings_.diameter : 1.5F * settings_.wordmark.pitch;
            bool onSurface = false;
            for (std::size_t sIdx = 0; sIdx < slots_.size() && !onSurface; ++sIdx)
            {
                onSurface = slots_[sIdx].letter == letter && gSlotSeating_[sIdx] &&
                            std::hypot(slots_[sIdx].x - pose.x, slots_[sIdx].y - pose.y) <= reach;
            }
            if (!onSurface) ++metrics_.offSurface;
        }
        metrics_.moored += mind.act == Act::Moored ? 1u : 0u;
        ++metrics_.tierCount[gTier_[i]];
    });
    for (std::uint8_t letter = 0; letter < 8; ++letter)
    {
        const bool upright = raiseAngle(letter) >= kPi * 0.5F - 1e-4F;
        metrics_.raised += upright ? 1u : 0u;
        if (upright && !letterUprightLogged_[letter])
        {
            letterUprightLogged_[letter] = true;
            if (diagnostics_) diagnostics_->letter(tickIndex_, letter, 2);
        }
    }
    rebuildGrid(gx_, gy_, include);
    float minGap = std::numeric_limits<float>::max();
    for (std::uint32_t i = 0; i < include.size(); ++i)
    {
        if (!include[i]) continue;
        ++metrics_.visibleMovers;
        forNeighbours(gx_[i], gy_[i], 2.0F * settings_.diameter, [&](std::uint32_t j) {
            if (j <= i) return;
            const float dist = std::hypot(gx_[i] - gx_[j], gy_[i] - gy_[j]);
            minGap = std::min(minGap, dist);
            if (dist < 0.8F * settings_.diameter) ++metrics_.visibleOverlaps;
        });
    }
    metrics_.visibleMinGap = minGap == std::numeric_limits<float>::max() ? -1.0F : minGap;   // -1: no pair
}

void SwarmIntro::snapshot(std::vector<RenderRow>& out)
{
    out.resize(entities_.size());
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        // World coordinates, z up. A wall-sheet unit stands in the wall plane, its lift
        // becoming a stand-off in front of the face.
        float wx, wy, wz, plane;
        worldOf(pose, mind, wx, wy, wz, plane);
        out[id.index] = { wx, wy, wz, pose.heading, plane, mind.act, static_cast<Tier>(gTier_[id.index]), mind.actSince, mind.magnet };
    });
}

std::uint64_t SwarmIntro::stateHash()
{
    std::vector<RenderRow> rows;
    snapshot(rows);
    std::uint64_t h = 1469598103934665603ull;
    auto feed = [&h](const void* data, std::size_t size) {
        const auto* bytes = static_cast<const unsigned char*>(data);
        for (std::size_t k = 0; k < size; ++k)
        {
            h ^= bytes[k];
            h *= 1099511628211ull;
        }
    };
    for (const RenderRow& r : rows)
    {
        feed(&r.x, sizeof r.x);
        feed(&r.y, sizeof r.y);
        feed(&r.heading, sizeof r.heading);
        feed(&r.act, sizeof r.act);
    }
    return h;
}
} // namespace crucible::spike::swarm
