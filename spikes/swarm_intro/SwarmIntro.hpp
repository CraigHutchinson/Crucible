#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>
#include <vector>

#include <sub0ecs/sub0ecs.hpp>

#include "Wordmark.hpp"

namespace crucible::spike::swarm
{
// ---- Components -------------------------------------------------------------------------
// A view-owned ECS world: these entities are decorative intro nanites and never impersonate
// the authoritative gameplay population (phase15 plan, "Carried follow-ups and bounds").

/** Stable identity: index into the frozen gather arrays, plus a personality seed. */
struct Identity
{
    std::uint32_t index = 0;
    std::uint32_t seed = 0;
};

/** Pose on an interaction sheet, plus a lift off that sheet (units ride over resting ones).
 *  Ground mode has one sheet, the substrate (x, y). Upright mode adds the wall face: a
 *  second sheet stored at y >= wallBand, joined to the ground along the wall's base. */
struct Pose
{
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float heading = 0.0F;
};

struct Motion
{
    float vx = 0.0F;
    float vy = 0.0F;
};

/** What a nanite is doing. Rest acts (Dormant, Locked, Moored) form the rest layer that
 *  movers may glide over; movers never pass through one another. */
enum class Act : std::uint8_t
{
    Dormant,   ///< Resting on the bed, core dark.
    Waking,    ///< Core lit by the answer wave; reaction delay before moving.
    Lane,      ///< Following an authored lane toward a letter group.
    Seek,      ///< Left the lane, heading for its slot (Climb build: for the wall base under it).
    Climb,     ///< Climb build only: on the wall face, climbing over locked units toward its slot.
    Dock,      ///< Inside capture range: aligning and sliding into the slot.
    Locked,    ///< Interlocked into the title.
    Moor,      ///< Surplus unit travelling to the ribbon.
    Moored,    ///< Resting in the ribbon.
    Reserve,   ///< Awake but unassigned: stays on the bed.
};

/** Per-unit intent. Personality values come from the seed, so behaviour is reproducible. */
struct Mind
{
    Act act = Act::Dormant;
    std::uint8_t lane = 0;
    std::int32_t goal = -1;      ///< Slot index, or ribbon index for Moor/Moored.
    float actSince = 0.0F;       ///< Seconds at which the current act began.
    float laneS = 0.0F;          ///< Progress along the lane polyline.
    float laneOffset = 0.0F;     ///< Lateral position inside the lane.
    float speedScale = 1.0F;
    float reaction = 0.1F;       ///< Seconds between waking and moving.
    bool ribbon = false;         ///< Goal indexes the ribbon rather than the title.
};

/** LOD tier tags. Each tier is its own partition, so a tier system only touches its units
 *  and its cost scales with that tier's population, not the swarm's. */
struct HeroTier { std::uint32_t since = 0; };
struct NearTier { std::uint32_t since = 0; };
struct FarTier { std::uint32_t since = 0; };
struct HiddenTier { std::uint32_t since = 0; };

enum class Tier : std::uint8_t { Hero, Near, Far, Hidden };

// ---- Settings ---------------------------------------------------------------------------

/** How the title is built. Lying is S0. Both upright builds end with the title standing on
 *  its baseline: Climb assembles it in place bottom-up, units climbing the built face (S0b);
 *  Raise assembles each letter flat, then hinges it up on its bottom row (S0c). */
enum class Build : std::uint8_t { Lying, Climb, Raise };

/** How tiers are chosen. Uniform modes exist to prove LOD changes cost, not the outcome. */
enum class LodMode : std::uint8_t { Camera, AllHero, AllNear, AllFar };

struct IntroSettings
{
    std::size_t nanites = 1536;
    std::uint32_t seed = 0xC0C1B1E5u;
    float tickSeconds = 1.0F / 60.0F;
    float diameter = 1.0F;
    float cruiseSpeed = 30.0F;     ///< Lane speed, diameters per second.
    float wakeAt = 0.6F;           ///< First core pulse (shot01).
    float answerAt = 1.8F;         ///< Answer wave begins (shot02).
    float answerWindow = 1.9F;     ///< Wave reaches the last slotted unit by answerAt+window.
    float ribbonAt = 5.6F;         ///< Surplus units leave the bed for the ribbon (shot05 lead-in).
    std::size_t heroCap = 128;     ///< Upper bound on full per-unit decision makers.
    LodMode lod = LodMode::Camera;
    Build build = Build::Lying;
    float viewTilt = 0.4F;         ///< Upright builds' view: screen y = height + tilt * depth (oblique front camera).
    int climbWindow = 12;          ///< Climb build: levels a slot may be beyond its letter's frontier before its unit climbs.
    float raiseDelay = 0.15F;      ///< Raise build: pause between a letter's last lock and its rise.
    float raiseSeconds = 0.9F;     ///< Raise build: duration of the hinge from flat to upright.
    WordmarkSettings wordmark{};
};

/** Scripted camera: orthographic top-down stand-in for the director's lens path. */
struct Camera
{
    float cx = 0.0F;
    float cy = 0.0F;
    float halfWidth = 10.0F;   ///< World units from centre to the left/right edge.
    float aspect = 16.0F / 9.0F;
    float pixelsWide = 1920.0F;
    [[nodiscard]] float halfHeight() const { return halfWidth / aspect; }
    [[nodiscard]] float pixelsPerUnit() const { return pixelsWide / (2.0F * halfWidth); }
};

/** One tick's diagnostics; costs are host wall time and advisory only. */
struct TickMetrics
{
    float time = 0.0F;
    std::array<std::uint32_t, 4> tierCount{};
    std::uint32_t migrations = 0;
    std::uint32_t visibleMovers = 0;
    std::uint32_t visibleOverlaps = 0;   ///< Mover pairs closer than 0.8 diameter, in view.
    float visibleMinGap = -1.0F;         ///< Smallest mover-mover centre distance in view; -1 if no pair.
    std::uint32_t locked = 0;
    std::uint32_t moored = 0;
    std::uint32_t dockClicks = 0;
    std::uint32_t heroYields = 0;
    std::uint32_t goalSwaps = 0;         ///< Neighbouring seekers that traded slots (d-CAPT style).
    std::uint32_t unsupported = 0;       ///< Climb build: locks without a locked support (must stay 0).
    std::uint32_t offSurface = 0;        ///< Climb build: climbers above the base with no locked unit within reach.
    std::uint32_t raised = 0;            ///< Raise build: letters fully upright.
    std::array<float, 6> systemMicros{};   ///< gather, signal, behaviour+negotiation, resolve, lod, metrics
};

/** Per-unit render row for capture tooling, in world coordinates (z up). */
struct RenderRow
{
    float x;
    float y;
    float z;
    float heading;   ///< Within the unit's plane.
    float plane;     ///< Plane tilt about the x axis: 0 ground, pi/2 an upright wall face.
    Act act;
    Tier tier;
    float actSince;
};

// ---- The intro --------------------------------------------------------------------------

/** The twelve-second nanite title sequence as an ECS of decorative units.
 *  Owns its world and all scratch; every per-tick buffer is sized at construction.
 *  Not thread-safe; one tick runs to completion before the next. */
class SwarmIntro
{
public:
    /** Builds the bed, wordmark, lanes and ribbon; allocates all scratch.
     *  @param settings Fixed configuration; count must not exceed Entity::kMaxEntities. */
    explicit SwarmIntro(IntroSettings settings);

    /** Advances one fixed tick. Systems run in a declared order against a frozen gather. */
    void tick();

    /** @return Elapsed sequence time in seconds. */
    [[nodiscard]] float time() const { return time_; }
    /** @return The camera of the most recent tick. */
    [[nodiscard]] const Camera& camera() const { return camera_; }
    /** @return Diagnostics of the most recent tick. */
    [[nodiscard]] const TickMetrics& metrics() const { return metrics_; }
    /** @return Number of title slots. */
    [[nodiscard]] std::size_t slotCount() const { return slots_.size(); }
    /** @return Number of ribbon moorings. */
    [[nodiscard]] std::size_t ribbonCount() const { return ribbon_.size(); }
    /** @return Units given a slot by the planner. */
    [[nodiscard]] std::size_t assignedSlots() const { return assignedSlots_; }

    /** Copies every unit's render state, ordered by identity. */
    void snapshot(std::vector<RenderRow>& out);
    /** FNV-1a over the bit patterns of all poses and acts; equal across identical runs. */
    [[nodiscard]] std::uint64_t stateHash();

private:
    using World = sub0ecs::store::World<std::tuple<
        sub0ecs::Query<Identity, Pose, Motion, Mind>,
        sub0ecs::Query<Identity, Pose, Motion, Mind, HeroTier>,
        sub0ecs::Query<Identity, Pose, Motion, Mind, NearTier>,
        sub0ecs::Query<Identity, Pose, Motion, Mind, FarTier>,
        sub0ecs::Query<Identity, Pose, Motion, Mind, HiddenTier>>>;

    struct Lane
    {
        std::vector<float> xs;
        std::vector<float> ys;
        std::vector<float> cumulative;   // arc length at each vertex
    };

    // Planning and geometry.
    [[nodiscard]] std::vector<Pose> buildBed() const;
    void buildRibbon();
    void planAssignment(std::span<const Pose> bed);
    void buildLanes(float bedTop);
    void lanePoint(const Lane& lane, float s, float& x, float& y, float& tx, float& ty) const;
    [[nodiscard]] Camera cameraAt(float t) const;
    void buildConstruction();
    [[nodiscard]] bool supported(std::size_t slot, std::span<const std::uint8_t> locked) const;
    [[nodiscard]] bool climbing() const { return settings_.build == Build::Climb; }
    [[nodiscard]] bool oblique() const { return settings_.build != Build::Lying; }
    /** The sheet follows the act, never the position: climb-build title units are on the wall
     *  from the moment they step onto it until they lock. */
    [[nodiscard]] bool onWall(const Mind& mind) const
    {
        return climbing() && !mind.ribbon && (mind.act == Act::Climb || mind.act == Act::Dock || mind.act == Act::Locked);
    }
    /** Raise build: hinge angle of a letter, 0 flat to pi/2 upright. */
    [[nodiscard]] float raiseAngle(std::uint8_t letter) const;
    /** World position (z up) and plane tilt of a unit. */
    void worldOf(const Pose& pose, const Mind& mind, float& wx, float& wy, float& wz, float& plane) const;
    void viewOf(const Pose& pose, const Mind& mind, float& vx, float& vy) const;
    /** Upright: next locked slot to step onto, routing over the built surface toward a slot. */
    [[nodiscard]] std::int64_t surfaceStep(const Pose& pose, std::size_t goal);
    void goalApproach(const Mind& mind, float& gx, float& gy) const;

    // Systems.
    void gather();
    void rebuildGrid(std::span<const float> xs, std::span<const float> ys, std::span<const std::uint8_t> include);
    void signalSystem();
    void behaviourSystems();
    void negotiationSystem();
    void resolveSystem();
    void lodSystem();
    void metricsSystem();

    template <typename F>
    void forNeighbours(float x, float y, float radius, F&& f) const;

    // Behaviour helpers.
    struct Desire
    {
        float vx = 0.0F;
        float vy = 0.0F;
        float faceHeading = 0.0F;
        bool hasFace = false;
    };
    Desire desire(const Identity& id, Pose& pose, Mind& mind);
    bool climbDesire(Pose& pose, Mind& mind, Desire& d);
    void advanceAct(Pose& pose, Mind& mind);
    void integrate(Pose& pose, Motion& motion, float vx, float vy, const Desire& d, float turnRate);
    [[nodiscard]] float liftOf(const Identity& id, const Pose& pose, const Mind& mind) const;
    void heroChoose(const Identity& id, const Pose& pose, float& vx, float& vy);
    [[nodiscard]] bool isMover(Act act) const;
    [[nodiscard]] bool inView(float x, float y, float margin) const;

    IntroSettings settings_;
    World world_;
    std::vector<sub0ecs::Entity> entities_;
    std::vector<Slot> slots_;
    std::vector<Pose> ribbon_;
    std::array<Lane, 3> lanes_{};
    std::size_t assignedSlots_ = 0;
    float hopDelay_ = 0.05F;
    std::uint32_t tickIndex_ = 0;
    float time_ = 0.0F;
    Camera camera_{};
    TickMetrics metrics_{};
    std::uint32_t first_ = 0;   // the wedge that wakes first (shot01)
    // Climb construction: wall footprint on the ground, wall sheet offset, and the build
    // graph over slots (depth from the ground row, BFS parent, adjacency) with lock state.
    float wallY_ = 0.0F;
    float wallBand_ = 0.0F;
    std::vector<std::uint32_t> slotDepth_, slotParent_, slotRoot_, slotEntry_;
    std::vector<std::vector<std::uint32_t>> slotNeighbours_;
    std::vector<std::uint8_t> slotLocked_, gSlotLocked_, gSlotSeating_;
    std::array<std::uint32_t, 8> letterLocked_{};
    std::array<float, 8> letterCompleteAt_{};   // -1 until the letter's last slot locks
    std::vector<std::vector<std::uint32_t>> letterSlots_;
    std::array<int, 8> gLetterFrontier_{};   // deepest locked depth per letter at tick start; -1 none
    std::vector<std::uint32_t> routeParent_;   // surface-route scratch (serial systems only)
    std::vector<float> routeCost_;
    std::vector<std::pair<float, std::uint32_t>> routeHeap_;
    float firstX_ = 0.0F, firstY_ = 0.0F;
    std::vector<std::uint32_t> hop_;   // planner's BFS hop count from first_

    // Frozen gather (tick-start epoch), indexed by Identity::index.
    std::vector<float> gx_, gy_, gvx_, gvy_, gWake_;
    std::vector<std::uint8_t> gAct_, gMover_, gTier_;
    std::vector<std::uint8_t> include_, allOnes_;   // grid membership masks
    std::vector<std::int32_t> gGoal_;
    std::vector<float> wakeTime_;   // owned by the signal system; -1 while dormant

    // Uniform grid over the stage (counting sort, rebuilt per epoch).
    float gridMinX_ = 0.0F, gridMinY_ = 0.0F, cell_ = 2.0F;
    int gridW_ = 0, gridH_ = 0;
    std::vector<std::uint32_t> cellStart_, cellItems_, cellCount_, cellMovers_;

    // LOD bookkeeping.
    std::vector<std::uint8_t> desiredTier_, pendingTicks_;
    std::vector<std::pair<float, std::uint32_t>> heroCandidates_;
    std::vector<float> correctionX_, correctionY_;
};
} // namespace crucible::spike::swarm
