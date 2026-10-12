#include "SwarmDiagnostics.hpp"

#include <array>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sub0log/json.hpp>
#include <sub0log/log.hpp>
#include <sub0log/merge.hpp>

namespace crucible::spike::swarm
{
namespace
{
constexpr sub0log::SubsystemId kSwarm{ 2 };
constexpr std::uint64_t kSchema = 1;
constexpr std::array<std::pair<sub0log::SubsystemId, std::string_view>, 1> kNames{ { { kSwarm, "swarm_intro" } } };

constexpr std::uint64_t id(SwarmEvent event)
{
    return static_cast<std::uint64_t>(event);
}
} // namespace

struct SwarmDiagnostics::Impl
{
    std::vector<std::byte> storage;   // heap storage is 8-aligned, as the segment requires
    sub0log::Logger logger;

    explicit Impl(std::size_t bytes)
        : storage(bytes)
        , logger{ sub0log::Logger::createInMemory(storage, { .segment_ = { .chunkBytes_ = 4096 }, .subsystemNames_ = kNames }) }
    {
    }
};

SwarmDiagnostics::SwarmDiagnostics(std::size_t bytes)
    : impl_{ std::make_unique<Impl>(bytes) }
{
}

SwarmDiagnostics::~SwarmDiagnostics() = default;

bool SwarmDiagnostics::valid() const noexcept
{
    return impl_->logger.valid();
}

void SwarmDiagnostics::claim(std::uint64_t tick, std::uint64_t unit, std::uint64_t slot, std::uint64_t letter, std::uint64_t depth,
                             std::uint64_t arrivalRank, std::uint64_t queuedTicks) noexcept
{
    const sub0log::Logger::ScopedBind bind{ impl_->logger };
    sub0log_info(kSwarm, "schema={} event={} tick={} unit={} slot={} letter={} depth={} rank={} queued={}", kSchema,
                 id(SwarmEvent::Claim), tick, unit, slot, letter, depth, arrivalRank, queuedTicks);
}

void SwarmDiagnostics::lock(std::uint64_t tick, std::uint64_t unit, std::uint64_t slot, std::uint64_t letter, std::uint64_t depth,
                            std::uint64_t mode) noexcept
{
    const sub0log::Logger::ScopedBind bind{ impl_->logger };
    sub0log_info(kSwarm, "schema={} event={} tick={} unit={} slot={} letter={} depth={} mode={}", kSchema, id(SwarmEvent::Lock),
                 tick, unit, slot, letter, depth, mode);
}

void SwarmDiagnostics::stuck(std::uint64_t tick, std::uint64_t unit, std::uint64_t act, std::uint64_t stalledTicks,
                             Contingency contingency) noexcept
{
    const sub0log::Logger::ScopedBind bind{ impl_->logger };
    sub0log_warning(kSwarm, "schema={} event={} tick={} unit={} act={} stalled={} contingency={}", kSchema,
                    id(SwarmEvent::Stuck), tick, unit, act, stalledTicks, static_cast<std::uint64_t>(contingency));
}

void SwarmDiagnostics::recovered(std::uint64_t tick, std::uint64_t unit, std::uint64_t act, std::uint64_t stuckTicks) noexcept
{
    const sub0log::Logger::ScopedBind bind{ impl_->logger };
    sub0log_info(kSwarm, "schema={} event={} tick={} unit={} act={} stuck_ticks={}", kSchema, id(SwarmEvent::Recovered), tick,
                 unit, act, stuckTicks);
}

void SwarmDiagnostics::magnet(std::uint64_t tick, std::uint64_t unit, MagnetMode mode, bool on) noexcept
{
    const sub0log::Logger::ScopedBind bind{ impl_->logger };
    sub0log_debug(kSwarm, "schema={} event={} tick={} unit={} mode={} on={}", kSchema, id(SwarmEvent::Magnet), tick, unit,
                  static_cast<std::uint64_t>(mode), std::uint64_t{ on ? 1u : 0u });
}

void SwarmDiagnostics::letter(std::uint64_t tick, std::uint64_t letter, std::uint64_t kind) noexcept
{
    const sub0log::Logger::ScopedBind bind{ impl_->logger };
    sub0log_info(kSwarm, "schema={} event={} tick={} letter={} kind={}", kSchema, id(SwarmEvent::Letter), tick, letter, kind);
}

void SwarmDiagnostics::supported(std::uint64_t tick, std::uint64_t slot, std::uint64_t letter, std::uint64_t depth,
                                 std::uint64_t owner, std::uint64_t ownerAct, std::uint64_t ownerDistanceMilli) noexcept
{
    const sub0log::Logger::ScopedBind bind{ impl_->logger };
    sub0log_debug(kSwarm, "schema={} event={} tick={} slot={} letter={} depth={} owner={} owner_act={} owner_distance_milli={}",
                  kSchema, id(SwarmEvent::Supported), tick, slot, letter, depth, owner, ownerAct, ownerDistanceMilli);
}

void SwarmDiagnostics::summary(std::uint64_t tick, std::uint64_t slots, std::uint64_t locked, std::uint64_t moored,
                               std::uint64_t claims, std::uint64_t stuck, std::uint64_t recovered) noexcept
{
    const sub0log::Logger::ScopedBind bind{ impl_->logger };
    sub0log_info(kSwarm, "schema={} event={} tick={} slots={} locked={} moored={} claims={} stuck={} recovered={}", kSchema,
                 id(SwarmEvent::Summary), tick, slots, locked, moored, claims, stuck, recovered);
}

bool SwarmDiagnostics::writeJson(std::ostream& output, std::ostream& health) const
{
    const auto stats = impl_->logger.stats();
    sub0log::Merger merger;
    const bool valid = impl_->logger.valid() && merger.addSegment(impl_->storage) == sub0log::SegmentError::Ok;
    const auto records = merger.merged();
    for (const auto& record : records)
    {
        output << sub0log::formatJson(record, merger.subsystemName(record.record_.site_->subsystem_)) << '\n';
    }
    output.flush();
    const auto totals = merger.totals();
    // Decimal strings keep the upstream JSONL integer contract.
    auto count = [](std::uint64_t value) { return "\"" + std::to_string(value) + "\""; };
    health << "{\"schema\":1,\"kind\":\"swarm_health\",\"valid\":" << (valid ? "true" : "false")
           << ",\"producer_dropped_records\":" << count(stats.droppedRecords_)
           << ",\"producer_truncated_records\":" << count(stats.truncatedRecords_) << ",\"records\":" << count(records.size())
           << ",\"unreadable_bytes\":" << count(totals.unreadableBytes_) << ",\"unwritten_bytes\":" << count(totals.unwrittenBytes_)
           << ",\"undecodable_records\":" << count(totals.undecodableRecords_)
           << ",\"skipped_records\":" << count(totals.skippedRecords_) << ",\"output_ok\":" << (output ? "true" : "false")
           << "}\n";
    health.flush();
    return valid && output && health && stats.droppedRecords_ == 0 && stats.truncatedRecords_ == 0 &&
           totals.unreadableBytes_ == 0 && totals.undecodableRecords_ == 0;
}
} // namespace crucible::spike::swarm
