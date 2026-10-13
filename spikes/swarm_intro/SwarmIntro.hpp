#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>
#include <vector>

#include <sub0ecs/sub0ecs.hpp>

#include "NaniteBody.hpp"
#include "SwarmDiagnostics.hpp"
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
    Queue,     ///< Climb build: at its letter's base, waiting to claim a slot in arrival order.
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
    std::int8_t letter = -1;     ///< Climb build: destination letter; the slot is claimed at its base.
    std::uint8_t magnet = 0;     ///< Active MagnetMode, 0 when off.
    std::uint16_t pushTicks = 0; ///< Remaining ticks of a push contingency.
    std::uint32_t arrived = 0;   ///< Climb build: tick the unit joined its letter's queue.
    std::uint32_t stall = 0;     ///< Ticks without progress toward the current aim.
    std::uint32_t stuckSince = 0;   ///< Tick a stuck episode began, 0 when not stuck.
    std::uint8_t progressAct = 0;   ///< Act the progress measure belongs to.
    std::int32_t progressGoal = -1; ///< Goal the progress baseline was measured against; a swap resets it.
    float best = 0.0F;           ///< Best progress value seen in that act.
};

/** LOD tier tags. Each tier is its own partition, so a tier system only touches its units
 *  and its cost scales with that tier's population, not the swarm's. */
struct HeroTier { std::uint32_t since = 0; };
struct NearTier { std::uint32_t since = 0; };
struct FarTier { std::uint32_t since = 0; };
struct HiddenTier { std::uint32_t since = 0; };

enum class Tier : std::uint8_t { Hero, Near, Far, Hidden };

// ---- Settings ---------------------------------------------------------------------------

/** How the title is built. Lying is S0. The upright builds end with the title standing on
 *  its baseline: Climb assembles it in place bottom-up, units climbing the built face (S0b,
 *  S0d); Raise assembles each letter flat, then hinges it up on its bottom row (S0c); Hybrid
 *  climbs some letters and raises the others like drawbridges (S0e); Scaffold raises the tall
 *  grounded stems as drawbridge panels first, then the swarm climbs them to finish arms and
 *  bowls, raising stemless deep letters whole (S0f). */
enum class Build : std::uint8_t { Lying, Climb, Raise, Hybrid, Scaffold };

/** How tiers are chosen. Uniform modes exist to prove LOD changes cost, not the outcome. */
enum class LodMode : std::uint8_t { Camera, AllHero, AllNear, AllFar };

struct IntroSettings
{
    std::size_t nanites = 1536;
    std::uint32_t seed = 0xC0C1B1E5u;
    float tickSeconds = 1.0F / 60.0F;   ///< Presentation (wall-clock) seconds per tick.
    /** Choreography seconds per presentation second. Every beat, speed and timer is authored in
     *  choreography seconds; each tick advances them by tickSeconds * pace, so 2/3 plays the whole
     *  sequence at 66% speed (15 s of choreography over 22.5 s) without retuning anything. The
     *  simulation steps are smaller, never larger, than the 60 Hz the behaviours were tuned at. */
    float pace = 2.0F / 3.0F;
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
    std::uint32_t claimCap = 96;   ///< Climb build: slots claimed but not yet locked, per letter and depth layer.
    float stuckSeconds = 40.0F / 60.0F; ///< Choreography seconds without progress before a unit counts as stuck.
    float pushSeconds = 0.5F;           ///< Choreography seconds a push contingency's repelling magnet stays on.
    float raiseDelay = 0.15F;      ///< Raise build: pause between a letter's last lock and its rise.
    float raiseSeconds = 0.9F;     ///< Raise build: duration of the hinge from flat to upright.
    /** Hybrid build: bit per letter (C R U C I B L E = bits 0..7) to raise; the rest climb.
     *  Zero chooses automatically: letters whose build depth is above the median are raised,
     *  since depth is the climb's critical path. */
    std::uint8_t hybridRaise = 0;
    WordmarkSettings wordmark{};
    NaniteBody body{};             ///< 3D body: stacking heights here, the mesh in the renderer.
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
    std::uint32_t claims = 0;            ///< Climb build: slots claimed this tick.
    std::uint32_t queued = 0;            ///< Climb build: units waiting at letter bases.
    std::uint32_t stuck = 0;             ///< Units that became stuck this tick.
    std::uint32_t recovered = 0;         ///< Stuck units that made progress again this tick.
    std::uint32_t pushes = 0;            ///< Push contingencies started this tick.
    std::uint32_t reclaims = 0;          ///< Reclaim contingencies this tick.
    std::uint32_t stuckNow = 0;          ///< Units currently stuck.
    std::uint32_t lifted = 0;            ///< Climbers currently drawn up by lift magnetism.
    std::uint32_t climbers = 0;          ///< Climb build: units on the wall face this tick.
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
    std::uint8_t magnet;   ///< Active MagnetMode, 0 when off.
    std::uint8_t layer;    ///< Depth layer of the title tile the unit is bound for (0 otherwise).
    bool shaped;           ///< Locked into a tile: `shape` holds its morphed corners.
    std::array<float, 6> shape;   ///< Corner offsets in the letter's plane (x, up), when shaped.
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

    /** @return Elapsed choreography time in seconds (presentation time is this over pace). */
    [[nodiscard]] float time() const { return time_; }
    /** @return Choreography seconds one tick advances: tickSeconds * pace. */
    [[nodiscard]] float stepSeconds() const { return settings_.tickSeconds * settings_.pace; }
    /** @return The camera of the most recent tick. */
    [[nodiscard]] const Camera& camera() const { return camera_; }
    /** @return Diagnostics of the most recent tick. */
    [[nodiscard]] const TickMetrics& metrics() const { return metrics_; }
    /** @return Number of title slots. */
    [[nodiscard]] std::size_t slotCount() const { return slots_.size(); }
    /** @return Number of ribbon moorings. */
    [[nodiscard]] std::size_t ribbonCount() const { return ribbon_.size(); }
    /** @return Drawbridge panels: whole letters (raise, hybrid) or stems (scaffold). */
    [[nodiscard]] std::uint32_t panelCount() const { return static_cast<std::uint32_t>(panels_.size()); }
    /** @return Bit per letter (C R U C I B L E = bits 0..7) with at least one raised panel. */
    [[nodiscard]] std::uint8_t raisedLetterMask() const
    {
        std::uint8_t m = 0;
        for (const Panel& panel : panels_) m = static_cast<std::uint8_t>(m | (1u << panel.letter));
        return m;
    }
    /** @return What tile morphing did to the outline. */
    [[nodiscard]] const WordmarkStats& wordmarkStats() const { return wordmarkStats_; }
    /** @return Units given a slot by the planner. */
    [[nodiscard]] std::size_t assignedSlots() const { return assignedSlots_; }

    /** Borrows a diagnostics sink for events; it must outlive every later tick. */
    void attachDiagnostics(SwarmDiagnostics* diagnostics) { diagnostics_ = diagnostics; }
    /** Records the end-of-run summary event, when diagnostics are attached. */
    void recordSummary();

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
    /** True when any letter is climbed (climb or hybrid build). */
    [[nodiscard]] bool climbing() const { return anyClimb_; }
    /** A unit bound for a climbed letter: it claims a slot at the base and climbs. */
    [[nodiscard]] bool climber(const Mind& mind) const { return !mind.ribbon && mind.letter >= 0; }
    [[nodiscard]] bool oblique() const { return settings_.build != Build::Lying; }
    /** The sheet follows the act, never the position: climb-build title units are on the wall
     *  from the moment they step onto it until they lock. */
    [[nodiscard]] bool onWall(const Mind& mind) const
    {
        return climber(mind) && (mind.act == Act::Climb || mind.act == Act::Dock || mind.act == Act::Locked);
    }
    /** Hinge angle of a drawbridge panel, 0 flat to pi/2 upright. */
    [[nodiscard]] float panelAngle(std::size_t panel) const;
    /** World position (z up) and plane tilt of a unit. */
    void worldOf(const Pose& pose, const Mind& mind, float& wx, float& wy, float& wz, float& plane) const;
    void viewOf(const Pose& pose, const Mind& mind, float& vx, float& vy) const;
    /** Upright: next locked slot to step onto, routing over the built surface toward a slot. */
    [[nodiscard]] std::int64_t surfaceStep(const Pose& pose, std::size_t goal, std::int64_t* after = nullptr);
    /** Marks slots reachable from the ground without crossing a panel not yet upright. */
    void updateClimbable();
    void goalApproach(const Mind& mind, float& gx, float& gy) const;
    /** Flat assembly: a front-layer tile seats only on a locked tile of the layer behind. */
    [[nodiscard]] bool faceBacked(std::size_t slot) const;

    // Systems.
    void gather();
    void rebuildGrid(std::span<const float> xs, std::span<const float> ys, std::span<const std::uint8_t> include);
    void signalSystem();
    void behaviourSystems();
    void negotiationSystem();
    void claimSystem();
    void contingencySystem();
    [[nodiscard]] bool progressOf(const Pose& pose, const Mind& mind, float& value) const;
    void setMagnet(std::uint32_t unit, Mind& mind, MagnetMode mode, bool on);
    void pushAway(std::uint32_t unit, const Pose& pose, float& vx, float& vy) const;
    /** Depth of the band in front of a letter where its climbers wait: 2 units per depth layer. */
    [[nodiscard]] float stagingBand() const { return 2.0F * static_cast<float>(std::max(1, settings_.wordmark.layers)); }
    void stagingPoint(std::uint32_t unit, const Mind& mind, float& x, float& y) const;
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
    bool climbDesire(std::uint32_t unit, Pose& pose, Mind& mind, Desire& d);
    void advanceAct(std::uint32_t unit, Pose& pose, Mind& mind);
    void integrate(Pose& pose, Motion& motion, float vx, float vy, const Desire& d, float turnRate);
    [[nodiscard]] float liftOf(const Identity& id, const Pose& pose, const Mind& mind) const;
    void heroChoose(const Identity& id, const Pose& pose, float& vx, float& vy);
    [[nodiscard]] bool isMover(Act act) const;
    [[nodiscard]] bool flows(const Pose& pose, const Mind& mind) const;
    [[nodiscard]] float seatHeading(float slotHeading, float current) const;
    [[nodiscard]] bool inView(float x, float y, float margin) const;

    IntroSettings settings_;
    World world_;
    std::vector<sub0ecs::Entity> entities_;
    std::vector<Slot> slots_;
    WordmarkStats wordmarkStats_{};
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
    std::vector<std::uint32_t> slotDepth_, slotParent_, slotRoot_;
    std::vector<std::vector<std::uint32_t>> slotNeighbours_;
    std::vector<std::uint8_t> slotLocked_, gSlotLocked_, gSlotSeating_;
    std::array<std::uint32_t, 8> letterLocked_{};
    /** A drawbridge panel: tiles assembled flat behind the baseline, hinged up as one rigid
     *  interlocked body once all are locked. */
    struct Panel
    {
        std::uint8_t letter = 0;
        std::uint32_t tiles = 0;
        std::uint32_t locked = 0;
        float completeAt = -1.0F;
        bool uprightLogged = false;
    };
    std::vector<Panel> panels_;
    std::vector<std::uint8_t> slotClimbs_;   // per tile: climbed (1) or assembled flat (0)
    std::vector<std::uint8_t> slotClimbable_;   // per tile: reachable without crossing a panel not yet upright
    std::size_t climbableEpoch_ = 0;            // standing panel tiles when slotClimbable_ was computed
    std::vector<std::int16_t> slotPanel_;    // per tile: panel index, -1 when climbed or lying
    std::vector<std::uint8_t> slotStanding_; // locked and, for panel tiles, upright: wall support
    std::vector<float> slotWallY_;           // every tile's standing position on the wall sheet
    std::array<bool, 8> letterClimbs_{};     // letter has any climbed tile
    bool anyClimb_ = false;
    std::array<float, 8> letterBaseMin_{}, letterBaseMax_{};   // ground-row x span per letter
    std::array<std::uint32_t, 8> letterInFlight_{};   // claimed, not yet locked
    std::vector<std::int32_t> slotOwner_;
    std::vector<std::uint8_t> slotSupportLogged_;               // claiming unit, -1 when free
    std::vector<std::pair<std::uint32_t, std::uint32_t>> queueScratch_;   // (arrived tick, unit)
    std::vector<std::uint32_t> gStall_;
    std::vector<std::uint8_t> gPush_;
    std::vector<std::uint8_t> gLayer_;   // flows over others this tick (climb build)
    SwarmDiagnostics* diagnostics_ = nullptr;
    std::uint64_t claimsTotal_ = 0, stuckTotal_ = 0, recoveredTotal_ = 0;
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
