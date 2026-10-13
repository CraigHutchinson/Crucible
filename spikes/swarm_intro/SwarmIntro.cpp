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
constexpr float kClimbBrake = 100.0F;  // climber deceleration assumed for cornering, of the 140 the motion allows
constexpr float kCornerRunOn = 4.0F;   // run-on past a waypoint allowed on a straight continuation
constexpr float kBranchLength = 26.0F;  // lane tail over which units branch off toward their own destinations
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
    slots_ = buildWordmark(settings_.wordmark, &wordmarkStats_);
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
        // Lane width scales with the tiles it carries: depth layers multiply the flow through
        // the same three lanes, and flow is density x width x speed.
        mind.laneOffset = (unit(seed, 3) - 0.5F) * 8.0F * static_cast<float>(std::max(1, settings_.wordmark.layers));
        entities_.push_back(world_.create(Identity{ i, seed }, bed[i], Motion{}, mind, HiddenTier{ 0, {} }));
        gTier_[i] = static_cast<std::uint8_t>(Tier::Hidden);
    }
    planAssignment(bed);
    firstX_ = bed[first_].x;
    firstY_ = bed[first_].y;
    titleMinX_ = std::numeric_limits<float>::max();
    titleMaxX_ = std::numeric_limits<float>::lowest();
    float lowY = std::numeric_limits<float>::max(), highY = std::numeric_limits<float>::lowest();
    for (const Slot& slot : slots_)
    {
        titleMinX_ = std::min(titleMinX_, slot.x);
        titleMaxX_ = std::max(titleMaxX_, slot.x);
        lowY = std::min(lowY, slot.y);
        highY = std::max(highY, slot.y);
    }
    // Upright, a letter stands on the wall line, its height on screen plus the tilted depth.
    titleMidY_ = oblique() ? 0.5F * settings_.wordmark.capHeight + settings_.viewTilt * wallY_ : 0.5F * (lowY + highY);
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
    // 3D tessellation: adjacent layers bond face to face. A wedge overlaps the two or three
    // wedges of the next layer whose centroids lie within 0.6 side (staggered lattices put
    // them 0.5 and 0.87 units apart); those are face links, so a front tile can be held by
    // the built layer behind it as well as from below, and depth counts through the bond.
    const float faceLink = 0.6F * tile;
    for (std::uint32_t a = 0; a < count; ++a)
    {
        for (std::uint32_t b = a + 1; b < count; ++b)
        {
            if (slots_[a].letter != slots_[b].letter) continue;
            const float d = std::hypot(slots_[a].x - slots_[b].x, slots_[a].y - slots_[b].y);
            const int layers = std::abs(static_cast<int>(slots_[a].layer) - static_cast<int>(slots_[b].layer));
            if ((layers == 0 && d <= link) || (layers == 1 && d <= faceLink))
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
    // Per-letter build mode. Hybrid, automatic: raise the letters deeper than the median,
    // because the deepest support chain is the climb's critical path.
    std::array<std::uint32_t, 8> maxDepth{};
    for (std::uint32_t a = 0; a < count; ++a) maxDepth[slots_[a].letter] = std::max(maxDepth[slots_[a].letter], slotDepth_[a]);
    std::array<std::uint32_t, 8> sorted = maxDepth;
    std::sort(sorted.begin(), sorted.end());
    const std::uint32_t median = (sorted[3] + sorted[4]) / 2;
    // Per-tile mode and drawbridge panels.
    slotClimbs_.assign(count, 0);
    slotPanel_.assign(count, -1);
    slotStanding_.assign(count, 0);
    std::array<std::int16_t, 8> letterPanel{};
    letterPanel.fill(-1);
    std::vector<std::pair<std::uint8_t, std::int16_t>> segmentPanels;   // (letter, segment) -> panel index
    auto panelFor = [&](std::uint8_t letter, std::int16_t segment) -> std::int16_t {
        for (std::size_t k = 0; k < segmentPanels.size(); ++k)
        {
            if (segmentPanels[k].first == letter && segmentPanels[k].second == segment) return static_cast<std::int16_t>(k);
        }
        return -1;
    };
    std::vector<std::int16_t> segmentPanelIndex;
    for (std::uint8_t letter = 0; letter < 8; ++letter)
    {
        const bool hybridRaise = settings_.hybridRaise != 0 ? ((settings_.hybridRaise >> letter) & 1u) != 0 : maxDepth[letter] > median;
        bool hasScaffold = false;
        for (std::uint32_t a : letterSlots_[letter]) hasScaffold = hasScaffold || slots_[a].scaffold;
        const Build build = settings_.build;
        const bool wholeRaise = build == Build::Raise || (build == Build::Hybrid && hybridRaise) ||
                                (build == Build::Scaffold && !hasScaffold && hybridRaise);
        for (std::uint32_t a : letterSlots_[letter])
        {
            if (build == Build::Lying) continue;
            if (wholeRaise)
            {
                if (letterPanel[letter] < 0)
                {
                    letterPanel[letter] = static_cast<std::int16_t>(panels_.size());
                    panels_.push_back(Panel{ letter });
                }
                slotPanel_[a] = letterPanel[letter];
            }
            else if (build == Build::Scaffold && slots_[a].scaffold)
            {
                std::int16_t k = panelFor(letter, slots_[a].segment);
                if (k < 0)
                {
                    k = static_cast<std::int16_t>(segmentPanels.size());
                    segmentPanels.push_back({ letter, slots_[a].segment });
                    segmentPanelIndex.push_back(static_cast<std::int16_t>(panels_.size()));
                    panels_.push_back(Panel{ letter });
                }
                slotPanel_[a] = segmentPanelIndex[static_cast<std::size_t>(k)];
            }
            else
            {
                slotClimbs_[a] = 1;
            }
            if (slotPanel_[a] >= 0) ++panels_[static_cast<std::size_t>(slotPanel_[a])].tiles;
            letterClimbs_[letter] = letterClimbs_[letter] || slotClimbs_[a];
        }
        anyClimb_ = anyClimb_ || letterClimbs_[letter];
    }
    if (!climbing()) return;
    // Stand the title up: height above the ground becomes the wall sheet's coordinate.
    // Lowest centroids sit a third of a row up (triangles) or half a unit up (rows).
    const float footing = tile > 0.0F ? rowHeight / 3.0F : 0.55F * settings_.diameter;
    slotWallY_.assign(count, 0.0F);
    for (Slot& slot : slots_)
    {
        const std::size_t k = static_cast<std::size_t>(&slot - slots_.data());
        // Every tile's standing position on the wall sheet; drawbridge tiles reach it when
        // their panel is upright, and climbers route over them there.
        slotWallY_[k] = wallBand_ + (slot.y - minY) + footing;
        if (slotClimbs_[k]) slot.y = slotWallY_[k];
    }
}

bool SwarmIntro::faceBacked(std::size_t slot) const
{
    // Flat assembly (planned slots) builds layer by layer: a front wedge seats only on a
    // locked wedge of the layer behind in the same flat body (letter or panel). A front wedge
    // on a panel's edge with none of its own panel behind it overhangs, carried by the rigid
    // interlocked panel; waiting on a climbed tile there would deadlock the panel's rise.
    if (slots_[slot].layer == 0) return true;
    bool backed = false;
    for (std::uint32_t b : slotNeighbours_[slot])
    {
        if (slots_[b].layer + 1 != slots_[slot].layer || slotClimbs_[b] || slotPanel_[b] != slotPanel_[slot]) continue;
        if (slotLocked_[b]) return true;
        backed = true;
    }
    return !backed;
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
    if (!slotClimbs_[static_cast<std::size_t>(mind.goal)])
    {
        gx = slot.x;
        gy = slot.y;
        return;
    }
    // Under the slot, clamped to its letter's footprint, so arrivals spread along the base;
    // then onto the nearest ground-row tile a climber can stand on: one that is climbed, or a
    // drawbridge tile already upright. Never into a gap between strokes.
    const Slot& goalSlot = slots_[static_cast<std::size_t>(mind.goal)];
    const float want = std::clamp(goalSlot.x, letterBaseMin_[goalSlot.letter], letterBaseMax_[goalSlot.letter]);
    gx = want;
    float best = std::numeric_limits<float>::max();
    for (std::uint32_t a : letterSlots_[goalSlot.letter])
    {
        if (slotDepth_[a] != 0 || !(slotClimbs_[a] || slotStanding_[a])) continue;
        const float d = std::abs(slots_[a].x - want);
        if (d < best)
        {
            best = d;
            gx = slots_[a].x;
        }
    }
    gy = wallY_ - 1.0F;
}

float SwarmIntro::panelAngle(std::size_t panel) const
{
    const float complete = panels_[panel].completeAt;
    if (complete < 0.0F) return 0.0F;
    return smooth(0.0F, kPi * 0.5F, (time_ - complete - settings_.raiseDelay) / settings_.raiseSeconds);
}

void SwarmIntro::worldOf(const Pose& pose, const Mind& mind, float& wx, float& wy, float& wz, float& plane) const
{
    wx = pose.x;
    plane = 0.0F;
    // 3D tessellation: a unit bound for a title tile stands that tile's layer in front of the
    // back face, along the face normal (up when flat, toward the camera when upright).
    const bool titled = !mind.ribbon && mind.goal >= 0 &&
                        (mind.act == Act::Locked || mind.act == Act::Dock || mind.act == Act::Climb);
    const float layer = titled ? static_cast<float>(slots_[static_cast<std::size_t>(mind.goal)].layer) * settings_.body.height : 0.0F;
    if (onWall(mind))
    {
        // Wall sheet: height is the sheet coordinate; lift is a stand-off in front of the face.
        wy = wallY_ - pose.z - layer;
        wz = pose.y - wallBand_;
        plane = kPi * 0.5F;
        return;
    }
    if (mind.act == Act::Locked && !mind.ribbon && slotPanel_[static_cast<std::size_t>(mind.goal)] >= 0)
    {
        // Hinge the flat letter about its baseline (the wall line), top swinging up.
        plane = panelAngle(static_cast<std::size_t>(slotPanel_[static_cast<std::size_t>(mind.goal)]));
        const float r = pose.y - wallY_;
        wy = wallY_ + r * std::cos(plane) - layer * std::sin(plane);
        wz = r * std::sin(plane) + (pose.z + layer) * std::cos(plane);
        return;
    }
    wy = pose.y;
    wz = pose.z + layer;
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
    hopDelay_ = std::max(stepSeconds(), settings_.answerWindow / static_cast<float>(maxHop));

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
        const auto& climbs = slotClimbs_;
        const float baseY = wallY_;
        std::stable_sort(ls.begin(), ls.end(), [&](std::uint32_t a, std::uint32_t b) {
            const std::uint32_t ka = climbs[a] ? slotDepth_[a] : slots_[a].layer;
            const std::uint32_t kb = climbs[b] ? slotDepth_[b] : slots_[b].layer;
            if (ka != kb) return ka < kb;   // climbed: shallow first; flat: back layer first
            const float ay = climbs[a] ? baseY : slots_[a].y;
            const float by = climbs[b] ? baseY : slots_[b].y;
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
            if (slotClimbs_[ls[m]]) mind->letter = static_cast<std::int8_t>(slots_[ls[m]].letter);
            else mind->goal = static_cast<std::int32_t>(ls[m]);
        }
    }

    // Ribbon moorings: the next units in wake order take the moorings farthest from the bed
    // centre, matched by x rank so no unit crosses the stage (a crossing unit was the last
    // to moor, measured at pace 2/3). Rank matching minimises total and worst travel along x.
    std::vector<std::uint32_t> ribbonOrder(ribbon_.size());
    for (std::uint32_t r = 0; r < ribbon_.size(); ++r) ribbonOrder[r] = r;
    std::stable_sort(ribbonOrder.begin(), ribbonOrder.end(), [&](std::uint32_t a, std::uint32_t b) {
        return std::abs(ribbon_[a].x) > std::abs(ribbon_[b].x);
    });
    std::vector<std::uint32_t> mooringUnits(order.begin() + static_cast<std::ptrdiff_t>(slotted),
                                            order.begin() + static_cast<std::ptrdiff_t>(slotted + moored));
    std::vector<std::uint32_t> moorings(ribbonOrder.begin(), ribbonOrder.begin() + static_cast<std::ptrdiff_t>(moored));
    std::stable_sort(mooringUnits.begin(), mooringUnits.end(), [&](std::uint32_t a, std::uint32_t b) { return bed[a].x < bed[b].x; });
    std::stable_sort(moorings.begin(), moorings.end(), [&](std::uint32_t a, std::uint32_t b) { return ribbon_[a].x < ribbon_[b].x; });
    for (std::size_t m = 0; m < moored; ++m)
    {
        Mind* mind = world_.find<Mind>(entities_[mooringUnits[m]]);
        mind->ribbon = true;
        mind->goal = static_cast<std::int32_t>(moorings[m]);
    }
}

constexpr float kMinFidelity = 0.1F;   // the budget controller never drops fidelity below this

// ---- Camera -----------------------------------------------------------------------------

namespace
{
// The close-up tour (cameraAt): it starts following the stream at kTourFrom and pans across
// the title until kTourEnd before the build's sequence ends; the closing pull-out then
// frames the whole title and holds for kOpenSeconds past the sequence.
constexpr float kTourFrom = 2.2F;
constexpr float kTourEnd = 4.0F;
constexpr float kOpenSeconds = 1.5F;
constexpr float kPaceEase = 0.6F;   // choreography seconds easing into and out of closePace
} // namespace

float SwarmIntro::sequenceSeconds() const
{
    return (climbing() ? 15.0F : 12.0F) + kOpenSeconds;
}

float SwarmIntro::paceAt(float t) const
{
    const float tourTo = sequenceSeconds() - kOpenSeconds - kTourEnd;
    const float in = std::clamp((t - kTourFrom) / kPaceEase, 0.0F, 1.0F);
    const float out = std::clamp((tourTo - t) / kPaceEase, 0.0F, 1.0F);
    return smooth(settings_.pace, settings_.closePace, std::min(in, out));
}

float SwarmIntro::presentAt(float t) const
{
    // Integrated in fine fixed steps; deterministic, for reports and frame headers only.
    constexpr float h = 1.0F / 1200.0F;
    double shown = 0.0;
    float s = 0.0F;
    for (; s + h <= t; s += h) shown += static_cast<double>(h) / static_cast<double>(paceAt(s + 0.5F * h));
    shown += static_cast<double>(t - s) / static_cast<double>(paceAt(s));
    return static_cast<float>(shown);
}

Camera SwarmIntro::cameraAt(float t) const
{
    // Orthographic stand-in for the brief's lens path. A macro on the first unit to wake,
    // then a close-up tour: follow the stream to the title's left end, dolly along the
    // letters as they build while the oblique view turns from one side to the other, then
    // pull out to the whole title, hold, and open toward the menu.
    const float fx = firstX_;
    const float fy = oblique() ? settings_.viewTilt * firstY_ : firstY_;
    const float span = titleMaxX_ - titleMinX_;
    const float left = titleMinX_ + 0.10F * span;
    const float right = titleMaxX_ - 0.10F * span;
    const float closeW = 0.13F * span;   // about two letters across
    const float sequence = sequenceSeconds() - kOpenSeconds;
    const float tourTo = sequence - kTourEnd;
    struct Key
    {
        float t, x, y, w, yaw;
    };
    const float wideY = oblique() ? 6.0F : -3.0F;
    const float openY = oblique() ? -4.0F : -20.0F;
    const float turn = oblique() ? 28.0F : 0.0F;   // degrees; the top view does not turn
    const std::array<Key, 9> keys{ { { 0.0F, fx, fy, 3.0F, 0.0F },
                                     { 1.8F, fx, fy + 0.4F, 4.0F, 0.0F },
                                     { kTourFrom, fx, fy + 1.0F, 12.0F, 0.4F * turn },   // widen on the bed; the follow camera takes over
                                     { kTourFrom + 1.4F, left, titleMidY_, closeW, turn },
                                     { tourTo, right, titleMidY_, closeW, -turn },
                                     { tourTo + 1.8F, 0.0F, wideY, 80.0F, -0.3F * turn },
                                     { sequence, 0.0F, wideY, 79.0F, 0.0F },
                                     { sequence + 0.75F * kOpenSeconds, 0.0F, openY, 96.0F, 0.0F },
                                     { sequence + kOpenSeconds, 0.0F, openY, 96.0F, 0.0F } } };
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
    cam.yaw = smooth(a.yaw, b.yaw, f);
    return cam;
}

void SwarmIntro::updateCamera()
{
    const Camera scripted = cameraAt(time_);
    const float tourTo = sequenceSeconds() - kOpenSeconds - kTourEnd;
    const float blend = smooth(0.0F, 1.0F, std::min((time_ - kTourFrom) / 0.8F, (tourTo - time_) / 1.0F));
    if (blend <= 0.0F)
    {
        camera_ = scripted;
        followX_ = scripted.cx;
        followY_ = scripted.cy;
        return;
    }
    const float span = titleMaxX_ - titleMinX_;
    const float left = titleMinX_ + 0.10F * span;
    const float right = titleMaxX_ - 0.10F * span;
    const float closeW = 0.13F * span;
    // Where is the title being built? Count docking, climbing and just-locked units per title
    // section; the camera holds its section until another is clearly busier.
    constexpr int kBins = 16;
    std::array<std::uint32_t, kBins> count{};
    std::array<float, kBins> sumX{}, sumY{};
    float headY = std::numeric_limits<float>::lowest();
    world_.each<Identity, Pose, Motion, Mind>([&](Identity&, Pose& pose, Motion&, Mind& mind) {
        if (mind.ribbon || mind.goal < 0) return;
        const bool building = mind.act == Act::Dock || mind.act == Act::Climb || (mind.act == Act::Locked && time_ - mind.actSince < 0.6F);
        if (!building) return;
        float vx, vy;
        viewOf(pose, mind, vx, vy);
        const int bin = std::clamp(static_cast<int>((vx - titleMinX_) / span * kBins), 0, kBins - 1);
        ++count[static_cast<std::size_t>(bin)];
        sumX[static_cast<std::size_t>(bin)] += vx;
        sumY[static_cast<std::size_t>(bin)] += vy;
    });
    // A section is two bins wide (about the close-up's width).
    auto busy = [&](int b) { return count[static_cast<std::size_t>(b)] + (b + 1 < kBins ? count[static_cast<std::size_t>(b + 1)] : 0U); };
    int best = 0;
    for (int b = 1; b < kBins; ++b)
        if (busy(b) > busy(best)) best = b;
    if (focusBin_ < 0 || busy(best) * 2 > busy(focusBin_) * 3 + 8) focusBin_ = best;
    float targetX = followX_, targetY = followY_;
    const std::uint32_t n = busy(focusBin_);
    if (n >= 6)
    {
        const auto b = static_cast<std::size_t>(focusBin_);
        const float sx = sumX[b] + (b + 1 < kBins ? sumX[b + 1] : 0.0F);
        const float sy = sumY[b] + (b + 1 < kBins ? sumY[b + 1] : 0.0F);
        targetX = sx / static_cast<float>(n);
        targetY = 0.5F * (sy / static_cast<float>(n) + titleMidY_);   // keep the letter in frame
    }
    else
    {
        // Nothing building yet: frame the head of the stream as it approaches the title.
        float sx = 0.0F, sy = 0.0F;
        std::uint32_t m = 0;
        world_.each<Identity, Pose, Motion, Mind>([&](Identity&, Pose& pose, Motion&, Mind& mind) {
            if (restAct(mind.act) || mind.ribbon) return;
            float vx, vy;
            viewOf(pose, mind, vx, vy);
            headY = std::max(headY, vy);
        });
        world_.each<Identity, Pose, Motion, Mind>([&](Identity&, Pose& pose, Motion&, Mind& mind) {
            if (restAct(mind.act) || mind.ribbon) return;
            float vx, vy;
            viewOf(pose, mind, vx, vy);
            if (vy < headY - 0.5F * closeW) return;
            sx += vx;
            sy += vy;
            ++m;
        });
        if (m > 0)
        {
            targetX = sx / static_cast<float>(m);
            targetY = sy / static_cast<float>(m) - 0.25F * closeW;   // the head toward the top, the stream behind
        }
    }
    targetX = std::clamp(targetX, left, right);
    // Critically smooth pursuit (time constant 0.6 choreography seconds).
    const float follow = 1.0F - std::exp(-stepSeconds() / 0.6F);
    followX_ += (targetX - followX_) * follow;
    followY_ += (targetY - followY_) * follow;
    // Orbit: the view turns with the camera's place along the title, from one side to the other.
    const float turn = oblique() ? 28.0F : 0.0F;
    const float along = std::clamp((followX_ - left) / std::max(1e-3F, right - left), 0.0F, 1.0F);
    camera_ = scripted;
    camera_.cx = scripted.cx + (followX_ - scripted.cx) * blend;
    camera_.cy = scripted.cy + (followY_ - scripted.cy) * blend;
    camera_.halfWidth = std::exp(std::log(scripted.halfWidth) + (std::log(closeW) - std::log(scripted.halfWidth)) * blend);
    camera_.yaw = scripted.yaw + (turn * (1.0F - 2.0F * along) - scripted.yaw) * blend;
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
    if (!climber(mind)) return false;
    if (mind.act == Act::Climb || (mind.act == Act::Seek && mind.goal >= 0 && pose.y > wallY_ - kHeapDepth)) return true;
    // Depth layers: a letter's waiting crowd outgrows its staging band (a two-layer U sends
    // about 300 climbers in two seconds), so the waiting crowd heaps too, a mound in front
    // of the letter, rather than pressing into contact (measured 6-8% of visible movers).
    return settings_.wordmark.layers > 1 && (mind.act == Act::Queue || mind.act == Act::Seek) && pose.y > wallY_ - kHeapDepth - stagingBand();
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
    time_ += stepSeconds();   // pace eases slower while the camera tours close up
    updateCamera();

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
    // Wall support: a locked tile stands once it is in the wall plane. Climbed tiles stand on
    // locking; drawbridge panel tiles only when their panel is upright.
    for (std::size_t q = 0; q < slots_.size(); ++q)
    {
        const std::int16_t panel = slotPanel_[q];
        slotStanding_[q] = slotLocked_[q] && (panel < 0 || panelAngle(static_cast<std::size_t>(panel)) >= kPi * 0.5F - 1e-4F) ? 1 : 0;
    }
    std::copy(slotStanding_.begin(), slotStanding_.end(), gSlotLocked_.begin());
    // Pipelined interlock: a slot whose owner is nearly seated already counts as a support for
    // motion (not for locking), so the next level starts sliding in without waiting a full dock.
    std::copy(slotStanding_.begin(), slotStanding_.end(), gSlotSeating_.begin());
    if (climbing())
    {
        world_.each<Identity, Pose, Motion, Mind>([this](Identity&, Pose& pose, Motion&, Mind& mind) {
            if (mind.act != Act::Dock || !climber(mind)) return;
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
                distance = static_cast<std::uint64_t>(std::hypot(slots_[q].x - ox, slotWallY_[q] - oy) * 1000.0F);
            }
            diagnostics_->supported(tickIndex_, q, slots_[q].letter, slotDepth_[q], owner, ownerAct, distance);
        }
    }
    if (climbing()) updateClimbable();
    gLetterFrontier_.fill(-1);
    for (std::size_t a = 0; a < slots_.size(); ++a)
    {
        if (gSlotLocked_[a]) gLetterFrontier_[slots_[a].letter] = std::max(gLetterFrontier_[slots_[a].letter], static_cast<int>(slotDepth_[a]));
    }
    // Freeze the tick-start epoch: every system reads these, and writes only its own row.
    gPushers_ = 0;
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
        gPushers_ += gPush_[i];
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
        // Project onto the lane centre line inside a short forward window; progress is
        // monotonic. Distance falls to the nearest point and then rises, so the search stops
        // at the first rise (S0n profile: the full window was 13% of a tick).
        float bestS = mind.laneS;
        float bestD = std::numeric_limits<float>::max();
        for (float s = mind.laneS; s <= std::min(end, mind.laneS + 8.0F); s += 0.5F)
        {
            lanePoint(lane, s, lx, ly, tx, ty);
            const float dd = (lx - pose.x) * (lx - pose.x) + (ly - pose.y) * (ly - pose.y);
            if (dd >= bestD) break;
            bestD = dd;
            bestS = s;
        }
        mind.laneS = bestS;
        // Branch toward the slot over the lane's tail (shot03): each unit leaves for where it is
        // going while still at full lane width, so the stream fans out sorted by destination.
        // A funnel narrowing to one exit point (the S0 rule) packed three letters' worth of
        // units together; it caused nearly all ground contacts.
        if (mind.laneS >= end - kBranchLength)
        {
            mind.act = Act::Seek;
            mind.actSince = time_;
            return desire(id, pose, mind);
        }
        lanePoint(lane, mind.laneS + kLookahead, lx, ly, tx, ty);
        const float aimX = lx - ty * mind.laneOffset;
        const float aimY = ly + tx * mind.laneOffset;
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
        if (climber(mind) && mind.act != Act::Dock && climbDesire(id.index, pose, mind, d)) return d;
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
                                  : std::min(speedCap, dist / stepSeconds() * 0.5F + settings_.cruiseSpeed * 0.25F);
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
        // At the base within reach of a ground tile it can stand on (never over a gap).
        const bool atBase = std::abs(pose.y - by) < 1.2F && std::abs(pose.x - bx) < 1.2F * settings_.diameter;
        // Nothing to climb yet: wait in front of the wall rather than hang on it. Ground-row
        // tiles can always be started; a slot reachable only over a scaffold panel waits until
        // that panel is upright (flat-locked panel tiles count as locked but carry nobody up).
        if (!atBase || (slotDepth_[goal] > 0 && gLetterFrontier_[letterIndex] < 0) || !slotClimbable_[goal]) return true;
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
    // edge on the slot's side, so clinging stays on the built surface.
    std::int64_t anchor = -1;
    float anchorDistance = std::numeric_limits<float>::max();
    for (std::uint32_t b : slotNeighbours_[goal])
    {
        if (!gSlotSeating_[b]) continue;
        const float db = std::hypot(slots_[b].x - pose.x, slotWallY_[b] - pose.y);
        if (db < anchorDistance) { anchorDistance = db; anchor = b; }
    }
    if (anchor >= 0 && toGoal < 3.0F * settings_.wordmark.pitch)
    {
        setMagnet(unit, mind, MagnetMode::Lift, true);
        const Slot& a = slots_[static_cast<std::size_t>(anchor)];
        // At most 0.9 units out from the anchor's centre, toward the slot: within its edge.
        const float ay = slotWallY_[static_cast<std::size_t>(anchor)];
        const float span = std::max(std::hypot(slot.x - a.x, slot.y - ay), 1e-3F);
        const float reach = std::min(0.9F * settings_.diameter, span * 0.5F);
        steer(a.x + (slot.x - a.x) / span * reach, ay + (slot.y - ay) / span * reach, climbSpeed);
        d.faceHeading = seatHeading(slot.heading, pose.heading);
        d.hasFace = true;
        return true;
    }
    // Route over the built surface: step to the next locked unit on a path toward the slot.
    // Lift magnetism: in contact with the built face, the structure draws the climber up.
    std::int64_t after = -1;
    const std::int64_t step = surfaceStep(pose, goal, &after);
    const bool contact = step >= 0 && std::hypot(slots_[static_cast<std::size_t>(step)].x - pose.x,
                                                  slotWallY_[static_cast<std::size_t>(step)] - pose.y) < 2.0F * settings_.wordmark.pitch;
    const float speed = contact ? climbSpeed * kLiftBoost : climbSpeed;
    setMagnet(unit, mind, MagnetMode::Lift, contact);
    // Waypoints carry momentum into a straight continuation but brake into a corner: the
    // unit must be able to stop within the waypoint distance plus a run-on that shrinks with
    // the turn, or its momentum carries it off the stroke (measured: a climber turning from
    // the R's base up its stem overshot three units into the counter).
    if (step >= 0)
    {
        const std::size_t s0 = static_cast<std::size_t>(step);
        // The last waypoint before the goal is treated as a corner: what follows is the cling
        // or the dock, both slow.
        const float ix = slots_[s0].x - pose.x, iy = slotWallY_[s0] - pose.y;
        const float in = std::hypot(ix, iy);
        float straight = 0.0F;
        if (after >= 0)
        {
            const std::size_t s1 = static_cast<std::size_t>(after);
            const float ox = slots_[s1].x - slots_[s0].x, oy = slotWallY_[s1] - slotWallY_[s0];
            const float out = std::hypot(ox, oy);
            straight = in > 1e-3F && out > 1e-3F ? std::max(0.0F, (ix * ox + iy * oy) / (in * out)) : 1.0F;
        }
        const float cap = std::min(speed, std::sqrt(2.0F * kClimbBrake * (in + kCornerRunOn * straight)));
        steer(slots_[s0].x, slotWallY_[s0], cap, false);
    }
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

void SwarmIntro::updateClimbable()
{
    // A slot can be climbed toward once some path from the ground reaches it without crossing
    // a scaffold panel that is still flat or rising: climbed tiles count (climbers build them),
    // panel tiles only once standing. Recomputed when a panel stands; one BFS over the slots.
    std::size_t standingPanelTiles = 0;
    for (std::size_t a = 0; a < slots_.size(); ++a) standingPanelTiles += slotPanel_[a] >= 0 && slotStanding_[a] ? 1u : 0u;
    if (standingPanelTiles == climbableEpoch_ && !slotClimbable_.empty()) return;
    climbableEpoch_ = standingPanelTiles;
    slotClimbable_.assign(slots_.size(), 0);
    std::deque<std::uint32_t> frontier;
    const auto passable = [&](std::uint32_t a) { return slotPanel_[a] < 0 || slotStanding_[a] != 0; };
    for (std::uint32_t a = 0; a < slots_.size(); ++a)
    {
        if (slotDepth_[a] == 0 && passable(a))
        {
            slotClimbable_[a] = 1;
            frontier.push_back(a);
        }
    }
    while (!frontier.empty())
    {
        const std::uint32_t a = frontier.front();
        frontier.pop_front();
        for (std::uint32_t b : slotNeighbours_[a])
        {
            if (slotClimbable_[b] || !passable(b)) continue;
            slotClimbable_[b] = 1;
            frontier.push_back(b);
        }
    }
}

std::int64_t SwarmIntro::surfaceStep(const Pose& pose, std::size_t goal, std::int64_t* after)
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
        const float d = std::hypot(slots_[a].x - pose.x, slotWallY_[a] - pose.y);
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
            const float d = std::hypot(slots_[a].x - pose.x, slotWallY_[a] - pose.y);
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
        const float cost = routeCost_[a] + std::hypot(slots_[a].x - slots_[b].x, slotWallY_[a] - slotWallY_[b]);
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
        const float d = std::hypot(slots_[a].x - slots_[goal].x, slotWallY_[a] - slotWallY_[goal]);
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
    // Walk back to the first step after the standing slot, noting the step that follows it.
    std::uint32_t at = target;
    std::int64_t next = -1;
    while (at != start && routeParent_[at] != start)
    {
        next = at;
        at = routeParent_[at];
    }
    if (after) *after = next;
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
    const bool structural = mind.ribbon || (!slotClimbs_[goal] && faceBacked(goal)) || supported(goal, slotStanding_);
    if (structural && std::hypot(sx - pose.x, sy - pose.y) < 0.12F && std::abs(wrapAngle(sh - pose.heading)) < 0.15F)
    {
        pose = { sx, sy, 0.0F, sh };
        mind.act = mind.ribbon ? Act::Moored : Act::Locked;
        mind.actSince = time_;
        setMagnet(unit, mind, MagnetMode::Seat, false);
        if (!mind.ribbon)
        {
            ++metrics_.dockClicks;
            if (diagnostics_) diagnostics_->lock(tickIndex_, unit, goal, slots_[goal].letter, slotDepth_[goal], slotClimbs_[goal]);
            if (slotClimbs_[goal]) --letterInFlight_[slots_[goal].letter];
            if (slotClimbs_[goal] && !supported(goal, slotStanding_)) ++metrics_.unsupported;
            if (slotPanel_[goal] >= 0)
            {
                Panel& panel = panels_[static_cast<std::size_t>(slotPanel_[goal])];
                if (++panel.locked == panel.tiles) panel.completeAt = time_;
            }
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
    const float dt = stepSeconds();
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
    const float dt = stepSeconds();

    // Hero: full per-unit decision making (sampled RVO, anticipation, climb, nuanced turns).
    // System 2 deliberates every heroPeriod ticks, staggered by identity; on the ticks between,
    // System 1 holds the last decision relative to the current desire and separates
    // reactively. Docking and flowing (layered) units have nothing to deliberate.
    const std::uint32_t period = std::max<std::uint32_t>(1, settings_.levers.heroPeriod);
    world_.each<Identity, Pose, Motion, Mind, HeroTier>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind, HeroTier& hero) {
        if (restAct(mind.act)) return;
        const Desire want = desire(id, pose, mind);
        float vx = want.vx, vy = want.vy;
        pushAway(id.index, pose, vx, vy);
        if (mind.act != Act::Dock && !flows(pose, mind))
        {
            const float speed = std::hypot(vx, vy);
            if (!hero.planned || (tickIndex_ + id.index) % period == 0)
            {
                const float before = std::atan2(vy, vx);
                heroChoose(id, pose, vx, vy);
                ++metrics_.deliberations;
                const float after = std::hypot(vx, vy);
                hero.planTurn = speed > 0.5F && after > 1e-3F ? wrapAngle(std::atan2(vy, vx) - before) : 0.0F;
                hero.planScale = speed > 0.5F ? after / speed : 1.0F;
                hero.planned = true;
            }
            else
            {
                const float c = std::cos(hero.planTurn), s = std::sin(hero.planTurn);
                const float hx = (c * vx - s * vy) * hero.planScale;
                const float hy = (s * vx + c * vy) * hero.planScale;
                vx = hx;
                vy = hy;
                separate(id.index, pose, mind, vx, vy);
            }
        }
        integrate(pose, motion, vx, vy, want, 9.0F);
        pose.z += (liftOf(id, pose, mind) - pose.z) * std::min(1.0F, dt * 12.0F);
        advanceAct(id.index, pose, mind);
    });

    // Near: reactive boids-style separation from movers; climb over the rest layer.
    world_.each<Identity, Pose, Motion, Mind, NearTier>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind, NearTier&) {
        if (restAct(mind.act)) return;
        const Desire want = desire(id, pose, mind);
        float vx = want.vx, vy = want.vy;
        // Docking units hold their line; climbers flow in layers instead of separating.
        const bool committed = mind.act == Act::Dock || flows(pose, mind);
        if (!committed)
        {
            pushAway(id.index, pose, vx, vy);
        }
        separate(id.index, pose, mind, vx, vy);
        integrate(pose, motion, vx, vy, want, 14.0F);
        pose.z += (liftOf(id, pose, mind) - pose.z) * std::min(1.0F, dt * 12.0F);
        advanceAct(id.index, pose, mind);
    });

    // Far: O(1) crowd response from the per-cell mover density gradient (continuum-style).
    world_.each<Identity, Pose, Motion, Mind, FarTier>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind, FarTier& tier) {
        if (restAct(mind.act)) return;
        const Desire want = plannedDesire(id, pose, mind, tier.route);
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
        pose.z = flows(pose, mind) ? settings_.body.height : 0.0F;
        advanceAct(id.index, pose, mind);
    });

    // Hidden: goal motion only; no neighbour queries and no overlap handling. It shares the
    // other tiers' motion model (acceleration limit, turn rate), so off-screen units reach the
    // same goals: setting velocity directly made units overshoot a waypoint every tick and
    // turn back forever, and two climb slots were never filled in isolation (SW-F31).
    world_.each<Identity, Pose, Motion, Mind, HiddenTier>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind, HiddenTier& tier) {
        if (restAct(mind.act)) return;
        const Desire want = plannedDesire(id, pose, mind, tier.route);
        integrate(pose, motion, want.vx, want.vy, want, 20.0F);
        pose.z = flows(pose, mind) ? settings_.body.height : 0.0F;
        advanceAct(id.index, pose, mind);
    });
}

float SwarmIntro::liftOf(const Identity& id, const Pose& pose, const Mind& mind) const
{
    // Lift off the sheet: over the rest layer one body height up; a climber over a
    // lower-identity climber one more (a deterministic stacking order).
    const float d = settings_.diameter;
    bool over = false;
    bool stacked = false;
    forNeighbours(pose.x, pose.y, 0.8F * d, [&](std::uint32_t j) {
        const Act other = static_cast<Act>(gAct_[j]);
        over = over || restAct(other);
        stacked = stacked || (flows(pose, mind) && j < id.index && gLayer_[j]);
    });
    const float layer = settings_.body.height;
    return stacked ? 2.0F * layer : (over || flows(pose, mind) ? layer : 0.0F);
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
            // Never swap across build modes; climbed tiles swap only within one depth.
            const bool climbI = slotClimbs_[static_cast<std::size_t>(mi->goal)] != 0;
            const bool climbJ = slotClimbs_[static_cast<std::size_t>(mj->goal)] != 0;
            const bool panelsMatch = slotPanel_[static_cast<std::size_t>(mi->goal)] == slotPanel_[static_cast<std::size_t>(mj->goal)];
            const bool sameDepth = climbI == climbJ && panelsMatch &&
                                   (!climbI || slotDepth_[static_cast<std::size_t>(mi->goal)] == slotDepth_[static_cast<std::size_t>(mj->goal)]);
            if (sameDepth && swapped + 0.5F < now)
            {
                std::swap(mi->goal, mj->goal);
                if (climbI)
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
    // The band deepens with the letter's depth layers: each layer brings as many climbers to
    // the same base width.
    y = wallY_ - 4.0F - stagingBand() * unit(mix(index), 8);
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

void SwarmIntro::separate(std::uint32_t unit, const Pose& pose, const Mind& mind, float& vx, float& vy) const
{
    // Boids-style separation from ground movers; docking units hold their line and climbers
    // flow in layers instead.
    if (mind.act == Act::Dock || flows(pose, mind)) return;
    const float reach = 1.6F * settings_.diameter;
    forNeighbours(pose.x, pose.y, reach, [&](std::uint32_t j) {
        if (j == unit || !gMover_[j] || gLayer_[j]) return;
        const float dx = pose.x - gx_[j], dy = pose.y - gy_[j];
        const float dist = std::max(std::hypot(dx, dy), 1e-3F);
        const float push = (reach - dist) / reach * settings_.cruiseSpeed * 0.8F;
        vx += dx / dist * push;
        vy += dy / dist * push;
    });
}

SwarmIntro::Desire SwarmIntro::plannedDesire(const Identity& id, Pose& pose, Mind& mind, RouteCache& route)
{
    // Only lane cruising is calm enough to hold a plan: arrival, docking and climbing steer
    // to a point and would overshoot it on a stale plan (as hidden motion once did, SW-F31).
    const std::uint32_t period = routePeriod_;
    if (mind.act == Act::Lane && route.valid && period > 1 && (tickIndex_ + id.index) % period != 0)
    {
        Desire held;
        held.vx = route.vx;
        held.vy = route.vy;
        held.faceHeading = route.face;
        held.hasFace = route.hasFace;
        return held;
    }
    const Desire want = desire(id, pose, mind);
    route = { want.vx, want.vy, want.faceHeading, want.hasFace, mind.act == Act::Lane };
    ++metrics_.routes;
    return want;
}

float SwarmIntro::activityOf(const Mind& mind)
{
    if (mind.stuckSince != 0) return 1.0F;
    if (mind.pushTicks > 0) return 0.95F;
    switch (mind.act)
    {
    case Act::Dock: return 0.85F;
    case Act::Climb: return 0.75F;
    case Act::Queue: return 0.65F;
    case Act::Seek: return 0.55F;
    case Act::Lane:
    case Act::Moor: return 0.15F;
    case Act::Waking: return 0.10F;
    default: return 0.0F;
    }
}

void SwarmIntro::pushAway(std::uint32_t unit, const Pose& pose, float& vx, float& vy) const
{
    // A pushing neighbour's repulsion: movers step aside to clear a stuck unit's path. Most
    // ticks nobody pushes, and then there is nothing to query (S0n profile: 5% of a tick).
    if (gPushers_ == 0) return;
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
    // claims in flight per depth layer (a two-layer letter has twice the tiles at each
    // level). Serial and in (arrival, identity) order, so it is deterministic.
    if (!climbing()) return;
    const std::uint32_t claimCap = settings_.claimCap * static_cast<std::uint32_t>(std::max(1, settings_.wordmark.layers));
    for (std::uint8_t letter = 0; letter < 8; ++letter)
    {
        if (!letterClimbs_[letter] || letterInFlight_[letter] >= claimCap) continue;
        queueScratch_.clear();
        world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose&, Motion&, Mind& mind) {
            if (mind.act == Act::Queue && mind.letter == static_cast<std::int8_t>(letter)) queueScratch_.push_back({ mind.arrived, id.index });
        });
        if (queueScratch_.empty()) continue;
        std::sort(queueScratch_.begin(), queueScratch_.end());
        std::uint32_t rank = 0;
        for (const auto& [arrived, unitIndex] : queueScratch_)
        {
            if (letterInFlight_[letter] >= claimCap) break;
            Pose* pose = world_.find<Pose>(entities_[unitIndex]);
            Mind* mind = world_.find<Mind>(entities_[unitIndex]);
            std::int64_t best = -1;
            float bestScore = std::numeric_limits<float>::max();
            for (std::uint32_t a : letterSlots_[letter])
            {
                if (!slotClimbs_[a] || slotOwner_[a] >= 0 || slotLocked_[a]) continue;
                bool claimable = slotDepth_[a] == 0;
                for (std::uint32_t b : slotNeighbours_[a])
                {
                    // A locked drawbridge tile counts even while flat: its climbers gather at the
                    // base and go up the moment the panel stands.
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
    // Stuck intelligence. A unit whose progress stalls for stuckSeconds decides what to do:
    // Wait when a senior unit beside it is still moving (it is simply queued behind it);
    // otherwise Push, switching on repelling magnetism briefly so juniors and idle units step
    // aside. Still stuck after three times as long, a seeker or climber Reclaims: it gives up
    // its slot and takes a free one (rejoining the queue on the ground). Serial, identity order.
    const auto ticksFor = [&](float seconds) { return std::max(1u, static_cast<std::uint32_t>(seconds / stepSeconds() + 0.5F)); };
    const std::uint32_t limit = ticksFor(settings_.stuckSeconds);
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        const std::uint32_t i = id.index;
        if (mind.pushTicks > 0 && --mind.pushTicks == 0) setMagnet(i, mind, MagnetMode::Push, false);
        float value = 0.0F;
        const bool tracked = progressOf(pose, mind, value);
        const auto act = static_cast<std::uint8_t>(mind.act);
        // A new act or a new goal (a negotiation swap can hand over a farther slot) starts a new
        // progress baseline: measured against the old goal, a unit moving at full speed toward
        // its new one looked stalled (the S0h lane branching made swaps common).
        if (!tracked || act != mind.progressAct || mind.goal != mind.progressGoal)
        {
            if (mind.stuckSince != 0 && diagnostics_) diagnostics_->recovered(tickIndex_, i, act, tickIndex_ - mind.stuckSince);
            if (mind.stuckSince != 0) { ++metrics_.recovered; ++recoveredTotal_; }
            mind.stuckSince = 0;
            mind.stall = 0;
            mind.progressAct = act;
            mind.progressGoal = mind.goal;
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
                mind.pushTicks = static_cast<std::uint16_t>(std::min(65535u, ticksFor(settings_.pushSeconds)));
                setMagnet(i, mind, MagnetMode::Push, true);
                ++metrics_.pushes;
            }
            mind.stuckSince = tickIndex_;
            ++metrics_.stuck;
            ++stuckTotal_;
            if (diagnostics_) diagnostics_->stuck(tickIndex_, i, act, mind.stall, choice);
        }
        else if (mind.stall == 3 * limit && climber(mind) && mind.goal >= 0 &&
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
                    if (a == goal || !slotClimbs_[a] || slotOwner_[a] >= 0 || slotLocked_[a] || !supported(a, gSlotSeating_)) continue;
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
        // Camera fidelity under a budget: below half fidelity, far units off screen keep only
        // their density-gradient separation; nobody can see their contacts (S0n).
        if (include[i] && fidelity_ < 0.5F && gTier_[i] == static_cast<std::uint8_t>(Tier::Far))
        {
            float vx, vy;
            viewOf(pose, mind, vx, vy);
            if (!inView(vx, vy, 0.15F * camera_.halfWidth + 4.0F * settings_.diameter)) include[i] = 0;
        }
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
    // Decision LOD (lod mode): who earns full per-unit intelligence this tick.
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
        case LodMode::AllHidden: want = Tier::Hidden; break;
        case LodMode::Blend:
        {
            // Two drivers, weighed by the levers: camera focus asks for actor fidelity of
            // motion and collision where the viewer looks (size on screen and centrality);
            // activity asks for decisions where interaction is complex. Off screen, units
            // with calm acts (lane, ribbon) go hidden; building units never do (SW-F30).
            const LodLevers& lv = settings_.levers;
            float vx, vy;
            viewOf(pose, mind, vx, vy);
            const bool seen = inView(vx, vy, margin);
            float view = 0.0F;
            if (seen)
            {
                const float rx = (vx - cam.cx) / cam.halfWidth;
                const float ry = (vy - cam.cy) / cam.halfHeight();
                view = std::clamp(1.0F - std::hypot(rx, ry), 0.0F, 1.0F) * std::clamp(ppu / 10.0F, 0.0F, 1.0F);
            }
            const float activity = activityOf(mind);
            const float weight = std::max(1e-3F, lv.viewWeight + lv.activityWeight);
            const float score = (lv.viewWeight * view + lv.activityWeight * activity) / weight;
            // The budget controller lowers fidelity by raising the bar for the near tier.
            const float nearMin = lv.nearMin + (1.0F - fidelity_) * (1.0F - lv.nearMin) * 0.5F;
            if (!seen && activity < 0.5F) want = Tier::Hidden;
            else want = score >= nearMin ? Tier::Near : Tier::Far;
            if (!restAct(mind.act) && score >= lv.heroMin) heroCandidates_.push_back({ 1.0F - score, i });
            break;
        }
        case LodMode::Activity:
        {
            // What the unit is doing decides how much it thinks; the camera does not. Building
            // acts get separation (near) and compete for the hero cap by how contended they
            // are; cruising and mooring units take the O(1) crowd response (far). Nothing is
            // hidden: off-screen units run the same logic as on-screen ones.
            const bool building = !mind.ribbon && (mind.act == Act::Seek || mind.act == Act::Queue ||
                                                   mind.act == Act::Climb || mind.act == Act::Dock);
            want = building ? Tier::Near : Tier::Far;
            if (!building || restAct(mind.act)) break;
            // Contention first: stuck, pushing, docking, climbing, queueing, seeking.
            float rank = 5.0F;
            if (mind.stuckSince != 0) rank = 0.0F;
            else if (mind.pushTicks > 0) rank = 1.0F;
            else if (mind.act == Act::Dock) rank = 2.0F;
            else if (mind.act == Act::Climb) rank = 3.0F;
            else if (mind.act == Act::Queue) rank = 4.0F;
            // Within a rank, the longest in its act first (fraction keeps ranks apart).
            const float age = std::clamp((time_ - mind.actSince) / 100.0F, 0.0F, 0.99F);
            heroCandidates_.push_back({ rank + (1.0F - age), i });
            break;
        }
        case LodMode::Camera:
        {
            float vx, vy;
            viewOf(pose, mind, vx, vy);
            if (!inView(vx, vy, margin))
            {
                // Activity floor: units building the title (seeking, queueing, climbing,
                // docking) keep at least the far tier off screen. Hidden-tier motion has no
                // separation, and a close-up camera leaving part of the title unseen let
                // hidden climbers and the seekers below them stall until it pulled out (SW-F30).
                const bool building = !mind.ribbon && (mind.act == Act::Seek || mind.act == Act::Queue ||
                                                       mind.act == Act::Climb || mind.act == Act::Dock);
                want = building ? Tier::Far : Tier::Hidden;
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
    // The budget scales the hero cap with fidelity (at least 8 heroes stay).
    const std::size_t heroCapNow = settings_.levers.budget > 0.0F
                                       ? std::max<std::size_t>(std::min<std::size_t>(8, settings_.levers.heroCap),
                                                               static_cast<std::size_t>(static_cast<float>(settings_.levers.heroCap) * fidelity_))
                                       : settings_.levers.heroCap;
    if (settings_.lod == LodMode::Camera || settings_.lod == LodMode::Activity || settings_.lod == LodMode::Blend)
    {
        const std::size_t cap = std::min(heroCapNow, heroCandidates_.size());
        std::partial_sort(heroCandidates_.begin(), heroCandidates_.begin() + static_cast<std::ptrdiff_t>(cap), heroCandidates_.end());
        for (std::size_t k = 0; k < cap; ++k) desiredTier_[heroCandidates_[k].second] = static_cast<std::uint8_t>(Tier::Hero);
    }

    // Hysteresis: a change must persist for a few ticks, except leaving Hidden, which is
    // immediate so units are interacting before they reach the visible frame. Hero
    // promotions respect the cap, after demotions.
    std::uint32_t heroes = 0;
    for (std::uint8_t t : gTier_) heroes += t == static_cast<std::uint8_t>(Tier::Hero) ? 1u : 0u;
    const std::size_t heroCap = settings_.lod == LodMode::AllHero ? gTier_.size() : heroCapNow;
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
        case Tier::Far: world_.add(e, FarTier{ tickIndex_, {} }); break;
        case Tier::Hidden: world_.add(e, HiddenTier{ tickIndex_, {} }); break;
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
            const bool immediate = from == Tier::Hidden || settings_.lod == LodMode::AllHero || settings_.lod == LodMode::AllNear ||
                                   settings_.lod == LodMode::AllFar || settings_.lod == LodMode::AllHidden;
            if (!immediate && pendingTicks_[i] < 6 && round == 0) continue;
            if (to == Tier::Hero && heroes >= heroCap) continue;
            migrate(i, from, to);
        }
    }

    // Predicted decision cost of this assignment for the next tick (cost units), and the
    // budget controller: deterministic, from tier populations, never from wall time, so runs
    // stay replayable. Over budget, fidelity drops quickly; well under, it recovers slowly.
    const LodLevers& lv = settings_.levers;
    std::array<std::uint32_t, 4> movers{};
    std::uint32_t farUnseen = 0;   // far movers off screen: unresolved below half fidelity
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion&, Mind& mind) {
        if (restAct(mind.act)) return;
        ++movers[gTier_[id.index]];
        if (lv.budget > 0.0F && gTier_[id.index] == static_cast<std::uint8_t>(Tier::Far))
        {
            float vx, vy;
            viewOf(pose, mind, vx, vy);
            farUnseen += inView(vx, vy, margin) ? 0U : 1U;
        }
    });
    const std::uint32_t period = std::max<std::uint32_t>(1, lv.heroPeriod);
    const float heroMovers = static_cast<float>(movers[static_cast<std::size_t>(Tier::Hero)]);
    const float deliberating = heroMovers / static_cast<float>(period);
    metrics_.predictedCost = deliberating * lv.costHero + (heroMovers - deliberating) * lv.costHeroHold +
                             static_cast<float>(movers[static_cast<std::size_t>(Tier::Near)]) * lv.costNear +
                             static_cast<float>(movers[static_cast<std::size_t>(Tier::Far)] - (fidelity_ < 0.5F ? farUnseen : 0U)) * lv.costFar +
                             static_cast<float>(fidelity_ < 0.5F ? farUnseen : 0U) * lv.costFarUnseen +
                             static_cast<float>(movers[static_cast<std::size_t>(Tier::Hidden)]) * lv.costHidden;
    if (lv.budget > 0.0F)
    {
        if (metrics_.predictedCost > lv.budget) fidelity_ = std::max(kMinFidelity, fidelity_ - 0.05F);
        else if (metrics_.predictedCost < 0.85F * lv.budget) fidelity_ = std::min(1.0F, fidelity_ + 0.01F);
    }
    else
    {
        fidelity_ = 1.0F;
    }
    metrics_.fidelity = fidelity_;
    // Below half fidelity the controller also stretches the route cadence of calm units, to 4.
    const std::uint32_t stretch = fidelity_ >= 0.5F ? 1U : (fidelity_ >= 0.3F ? 2U : 4U);
    routePeriod_ = std::max<std::uint32_t>(1, std::max(lv.routePeriod, stretch));
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
                            std::hypot(slots_[sIdx].x - pose.x, slotWallY_[sIdx] - pose.y) <= reach;
            }
            if (!onSurface) ++metrics_.offSurface;
        }
        metrics_.moored += mind.act == Act::Moored ? 1u : 0u;
        if (!restAct(mind.act)) ++metrics_.tierCount[gTier_[i]];   // movers: the units that spend decisions
    });
    for (std::size_t k = 0; k < panels_.size(); ++k)
    {
        const bool upright = panelAngle(k) >= kPi * 0.5F - 1e-4F;
        metrics_.raised += upright ? 1u : 0u;
        if (upright && !panels_[k].uprightLogged)
        {
            panels_[k].uprightLogged = true;
            if (diagnostics_) diagnostics_->letter(tickIndex_, panels_[k].letter, 2);
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
    world_.each<Identity, Pose, Motion, Mind>([&](Identity& id, Pose& pose, Motion& motion, Mind& mind) {
        // World coordinates, z up. A wall-sheet unit stands in the wall plane, its lift
        // becoming a stand-off in front of the face.
        float wx, wy, wz, plane;
        worldOf(pose, mind, wx, wy, wz, plane);
        const bool shaped = mind.act == Act::Locked && !mind.ribbon && settings_.wordmark.tile > 0.0F;
        const std::uint8_t layer = !mind.ribbon && mind.goal >= 0 ? slots_[static_cast<std::size_t>(mind.goal)].layer : 0;
        out[id.index] = { wx, wy, wz, pose.heading, plane, mind.act, static_cast<Tier>(gTier_[id.index]), mind.actSince, mind.magnet,
                          layer, shaped, shaped ? slots_[static_cast<std::size_t>(mind.goal)].shape : std::array<float, 6>{},
                          std::hypot(motion.vx, motion.vy) };
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
