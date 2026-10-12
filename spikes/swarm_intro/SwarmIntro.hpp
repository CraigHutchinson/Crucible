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

/** Planar pose with a cosmetic climb height (units ride over resting neighbours). */
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
    Seek,      ///< Left the lane, heading for its slot.
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
    std::array<float, 6> systemMicros{};   ///< gather, signal, behaviour+negotiation, resolve, lod, metrics
};

/** Per-unit render row for capture tooling. */
struct RenderRow
{
    float x;
    float y;
    float z;
    float heading;
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
    void advanceAct(Pose& pose, Mind& mind);
    void integrate(Pose& pose, Motion& motion, float vx, float vy, const Desire& d, float turnRate);
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
