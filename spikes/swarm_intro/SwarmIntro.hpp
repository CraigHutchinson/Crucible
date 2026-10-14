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
/** Hero tier, with the unit's System 2 plan: the last deliberated choice, held as a turn and
 *  a speed scale relative to the desired velocity, so System 1 can keep applying it to a
 *  desire that moves on between deliberations. */
struct HeroTier
{
    std::uint32_t since = 0;
    float planTurn = 0.0F;
    float planScale = 1.0F;
    bool planned = false;
};
struct NearTier { std::uint32_t since = 0; };
/** A cheap tier's System 2 for calm acts: the last goal steering (desired velocity and
 *  facing), re-planned every LodLevers::routePeriod ticks; System 1 integrates it every tick. */
struct RouteCache
{
    float vx = 0.0F;
    float vy = 0.0F;
    float face = 0.0F;
    bool hasFace = false;
    bool valid = false;
};
struct FarTier { std::uint32_t since = 0; RouteCache route; };
struct HiddenTier { std::uint32_t since = 0; RouteCache route; };

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
/** Decision LOD: who earns which behaviour tier. Blend: a score weighing camera focus (actor
 *  fidelity of motion and collision on screen) against activity (interaction complexity),
 *  set by LodLevers. Camera: by on-screen position and size only. Activity: by what a unit
 *  is doing only. All*: one tier for every unit, the isolation gates. */
enum class LodMode : std::uint8_t { Camera, AllHero, AllNear, AllFar, AllHidden, Activity, Blend };

/** The decision-LOD levers. Adjustable between ticks (SwarmIntro::setLevers): the intro, or
 *  later a game, can shift fidelity toward the camera or toward activity, or tighten the
 *  budget, while it runs. */
struct LodLevers
{
    float viewWeight = 0.5F;       ///< Blend: weight of camera focus (on-screen size and centrality).
    float activityWeight = 1.0F;   ///< Blend: weight of activity (contention and interaction).
    float heroMin = 0.55F;         ///< Blend: least score for a hero candidate.
    float nearMin = 0.30F;         ///< Blend: least score for the near tier.
    std::size_t heroCap = 128;     ///< Upper bound on full per-unit decision makers.
    /** System 2 cadence: a hero deliberates (sampled velocity obstacles) every this many ticks,
     *  staggered by identity; between deliberations System 1 holds the last decision and
     *  separates reactively every tick. 1 deliberates every tick. */
    std::uint32_t heroPeriod = 2;
    /** System 2 cadence of goal steering for calm units (cruising a lane) in the far and
     *  hidden tiers: they re-plan every this many ticks, staggered by identity, and integrate
     *  the last plan between. Goal logic is most of every tier's cost (S0n). 1 re-plans every
     *  tick; the budget controller stretches it up to 4 as fidelity falls. */
    std::uint32_t routePeriod = 1;
    /** Decision budget per presented tick in cost units: thousands of instructions of the
     *  behaviour and resolve passes, from the per-tier costs below (deterministic and
     *  host-independent; a host's instructions per microsecond turns it into time). 0 is
     *  unbounded. A deterministic controller scales fidelity (the hero cap, the near threshold
     *  and, below half fidelity, the route cadence) to keep the predicted cost inside it. */
    float budget = 0.0F;
    // Per-mover costs, thousands of instructions per tick (S0n callgrind, scaffold, release):
    // behaviour plus resolve. Deliberation dominates a hero; resolve dominates the rest.
    float costHero = 14.2F;        ///< Per hero deliberation (System 2).
    float costHeroHold = 6.4F;     ///< Per hero holding its plan (System 1).
    float costNear = 6.3F;         ///< Per near unit.
    float costFar = 5.5F;          ///< Per far unit.
    float costFarUnseen = 3.0F;    ///< Per far unit off screen below half fidelity (no resolve).
    float costHidden = 2.9F;       ///< Per hidden unit (no resolve).
};

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
    /** Pace while the camera tours the construction close up (cameraAt): slower, so a viewer
     *  can follow units seating. Eased in and out; equal to `pace` keeps a constant pace. */
    float closePace = 0.5F;
    float diameter = 1.0F;
    float cruiseSpeed = 30.0F;     ///< Lane speed, diameters per second.
    float wakeAt = 0.6F;           ///< First core pulse (shot01).
    float answerAt = 1.8F;         ///< Answer wave begins (shot02).
    float answerWindow = 1.9F;     ///< Wave reaches the last slotted unit by answerAt+window.
    float ribbonAt = 5.6F;         ///< Surplus units leave the bed for the ribbon (shot05 lead-in).
    LodLevers levers;              ///< Decision-LOD levers (Blend), the hero cap and System 2 cadence.
    LodMode lod = LodMode::Blend;  ///< Decision LOD (S0n: camera focus and activity blended).
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
    float yaw = 0.0F;   ///< Oblique view: degrees of turn about the vertical (presentation only; the LOD ignores it).
};

/** One tick's diagnostics; costs are host wall time and advisory only. */
struct TickMetrics
{
    float time = 0.0F;
    std::array<std::uint32_t, 4> tierCount{};   ///< Movers per decision tier.
    std::uint32_t migrations = 0;
    std::uint32_t visibleMovers = 0;
    std::uint32_t visibleOverlaps = 0;   ///< Mover pairs closer than 0.8 diameter, in view.
    float visibleMinGap = -1.0F;         ///< Smallest mover-mover centre distance in view; -1 if no pair.
    std::uint32_t locked = 0;
    std::uint32_t moored = 0;
    std::uint32_t dockClicks = 0;
    std::uint32_t heroYields = 0;
    std::uint32_t deliberations = 0;     ///< Hero System 2 deliberations this tick.
    std::uint32_t routes = 0;            ///< Far and hidden goal re-plans this tick.
    float predictedCost = 0.0F;          ///< Decision cost the LOD assignment predicts (cost units).
    float fidelity = 1.0F;               ///< Budget controller's fidelity factor in [kMinFidelity, 1].
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
    float speed;                  ///< Diameters per choreography second (the renderer stretches moving shells).
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

    /** Updates the viewport at a coordinator boundary.
     * @param aspect Finite drawable width/height in [1/32, 32].
     * @param pixelsWide Finite drawable width in [1, 4096].
     * @return False without mutation for unsupported dimensions. */
    [[nodiscard]] bool trySetViewport(float aspect, float pixelsWide) noexcept;

    /** Replaces the decision-LOD levers; takes effect from the next tick's LOD pass. */
    void setLevers(const LodLevers& levers) { settings_.levers = levers; }
    /** @return The current decision-LOD levers. */
    [[nodiscard]] const LodLevers& levers() const { return settings_.levers; }

    /** @return Elapsed choreography time in seconds (presentation time: presentAt). */
    [[nodiscard]] float time() const { return time_; }
    /** @return Choreography seconds the next tick advances: tickSeconds * paceAt(time). */
    [[nodiscard]] float stepSeconds() const { return settings_.tickSeconds * paceAt(time_); }
    /** @return Choreography seconds per presentation second at choreography time t: `pace`,
     *  easing to `closePace` while the camera tours the construction close up. */
    [[nodiscard]] float paceAt(float t) const;
    /** @return Presentation (on-screen) seconds at which choreography time t is shown. */
    [[nodiscard]] float presentAt(float t) const;
    /** @return Choreography length: the build's sequence (12 s, or 15 s when climbing) and the
     *  closing pull-out to the whole title. */
    [[nodiscard]] float sequenceSeconds() const;
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
    /** This tick's camera, a camera operator in two parts (S0o). System 2, the operator,
     *  plans a shot four times a presented second: the scripted path (cameraAt) outside the
     *  close-up tour; inside it, the title section with the most building now and predicted
     *  (units heading for slots there), held for a minimum time, framed to the activity's
     *  spread, creeping in while held, widening on long moves (an arc through the title) and
     *  orbiting as it travels. System 1, the rig, moves the camera every tick toward that shot
     *  with critically damped springs under speed and acceleration limits, in presented time.
     *  Deterministic: it reads the previous tick's state. */
    void updateCamera();
    /** System 2 of updateCamera: the shot the operator wants now. */
    void planShot(float presented);
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
    /** System 1 reactive separation from nearby ground movers (near tier; heroes between
     *  deliberations). */
    void separate(std::uint32_t unit, const Pose& pose, const Mind& mind, float& vx, float& vy) const;
    /** Blend LOD: what a unit's act asks of its decisions, 0 (resting) to 1 (stuck). */
    [[nodiscard]] static float activityOf(const Mind& mind);
    /** Goal steering for a far or hidden unit, re-planned at the route cadence for lane
     *  cruising and every tick otherwise. */
    Desire plannedDesire(const Identity& id, Pose& pose, Mind& mind, RouteCache& route);
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
    float fidelity_ = 1.0F;   ///< Budget controller state (LodLevers::budget); 1 when unbounded.
    std::uint32_t routePeriod_ = 1;   ///< Effective route cadence: the lever, stretched by the budget.
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
    std::uint32_t gPushers_ = 0;   ///< Units pushing this tick (gather); pushAway is free when none.
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
    /** The title in view coordinates (x, and screen height in the oblique view or y when
     *  lying), for the camera's close-up tour. */
    float titleMinX_ = 0.0F, titleMaxX_ = 0.0F, titleMidY_ = 0.0F;
    /** Camera rig state (System 1): position, log zoom and yaw with their velocities. */
    struct CameraRig
    {
        float x = 0.0F, y = 0.0F, logW = 0.0F, yaw = 0.0F;
        float vx = 0.0F, vy = 0.0F, vw = 0.0F, vyaw = 0.0F;
        bool ready = false;
    };
    CameraRig rig_;
    float viewportAspect_ = 16.0F / 9.0F;
    float viewportPixelsWide_ = 1920.0F;
    /** The operator's shot (System 2): where it wants the camera, and since when. */
    struct Shot
    {
        float x = 0.0F, y = 0.0F, logW = 0.0F, yaw = 0.0F;
        int section = -1;         ///< Title section held (-1: stream head or scripted).
        float since = 0.0F;       ///< Presented seconds the section has been held.
        float planned = -1.0F;    ///< Presented time of the last plan.
    };
    Shot shot_;
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
