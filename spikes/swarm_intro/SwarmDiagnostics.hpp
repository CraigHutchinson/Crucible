#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>

namespace crucible::spike::swarm
{
/** Swarm intro event schema 1. Every event's Sub0Log arguments start with unsigned64
 *  `[app_schema, event, tick, ...]`; the rest are listed per event. Consumers key on the
 *  numeric arguments, never on message text. */
enum class SwarmEvent : std::uint64_t
{
    Claim = 1,       ///< unit, slot, letter, depth, arrival_rank, queued_ticks
    Lock = 2,        ///< unit, slot, letter, depth
    Stuck = 3,       ///< unit, act, stalled_ticks, contingency
    Recovered = 4,   ///< unit, act, stuck_ticks
    Magnet = 5,      ///< unit, mode, on
    Letter = 6,      ///< letter, kind (1 complete, 2 upright)
    Summary = 7,     ///< slots, locked, moored, claims, stuck, recovered
    Supported = 8,   ///< slot, letter, depth, owner (or max u64), owner_act, owner_distance_milli
};

/** What a stuck unit tries; recorded in Stuck events. */
enum class Contingency : std::uint64_t
{
    Wait = 1,      ///< Blocked by a senior unit that is still moving: queue behind it.
    Push = 2,      ///< Blocked by juniors or idle units: repel them for a short time.
    Reclaim = 3,   ///< Still stuck after pushing: release the slot and rejoin the queue.
};

/** Selective magnetism modes; recorded in Magnet events. */
enum class MagnetMode : std::uint64_t
{
    Lift = 1,   ///< Attached to the built face; the structure draws the unit up.
    Push = 2,   ///< Repelling nearby movers to clear a path.
    Seat = 3,   ///< Pulled into its slot once supported.
};

/** Bounded in-memory Sub0Log sink for the swarm intro. Storage is allocated once at
 *  construction; recording never allocates, formats or locks. Not thread-safe: the intro
 *  ticks on one thread. Export after the last tick (cold path). */
class SwarmDiagnostics
{
public:
    /** @param bytes Segment size; records beyond it are dropped and counted. */
    explicit SwarmDiagnostics(std::size_t bytes = std::size_t{ 4 } << 20);
    ~SwarmDiagnostics();
    SwarmDiagnostics(const SwarmDiagnostics&) = delete;
    SwarmDiagnostics& operator=(const SwarmDiagnostics&) = delete;

    /** Records one event. Unused trailing arguments are omitted per event schema. */
    void claim(std::uint64_t tick, std::uint64_t unit, std::uint64_t slot, std::uint64_t letter, std::uint64_t depth,
               std::uint64_t arrivalRank, std::uint64_t queuedTicks) noexcept;
    void lock(std::uint64_t tick, std::uint64_t unit, std::uint64_t slot, std::uint64_t letter, std::uint64_t depth) noexcept;
    void stuck(std::uint64_t tick, std::uint64_t unit, std::uint64_t act, std::uint64_t stalledTicks, Contingency contingency) noexcept;
    void recovered(std::uint64_t tick, std::uint64_t unit, std::uint64_t act, std::uint64_t stuckTicks) noexcept;
    void magnet(std::uint64_t tick, std::uint64_t unit, MagnetMode mode, bool on) noexcept;
    void letter(std::uint64_t tick, std::uint64_t letter, std::uint64_t kind) noexcept;
    void supported(std::uint64_t tick, std::uint64_t slot, std::uint64_t letter, std::uint64_t depth, std::uint64_t owner,
                   std::uint64_t ownerAct, std::uint64_t ownerDistanceMilli) noexcept;
    void summary(std::uint64_t tick, std::uint64_t slots, std::uint64_t locked, std::uint64_t moored, std::uint64_t claims,
                 std::uint64_t stuck, std::uint64_t recovered) noexcept;

    /** @return False when the logger could not be created (records are then not kept). */
    [[nodiscard]] bool valid() const noexcept;

    /** Writes upstream schema-1 JSONL messages, then one `swarm_health` object to `health`.
     *  @return False for an invalid or damaged segment, producer drops, or failed output. */
    [[nodiscard]] bool writeJson(std::ostream& output, std::ostream& health) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace crucible::spike::swarm
