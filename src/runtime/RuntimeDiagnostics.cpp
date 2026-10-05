#include <crucible/runtime/RuntimeDiagnostics.hpp>

#include <array>
#include <ostream>
#include <crucible/contracts/ReclamationMission.hpp>
#include <crucible/contracts/StructuralState.hpp>
#include <sub0log/log.hpp>
#include <sub0log/reader.hpp>

namespace crucible::runtime {

struct RuntimeDiagnostics::Impl {
    // Compact header plus fifteen 4096-byte chunks; unused tail remains bounded.
    alignas(64) std::array<std::byte, 65536> storage{};
    sub0log::Logger logger;

    Impl() : logger{sub0log::Logger::createInMemory(storage,
        {.segment_ = {.chunkBytes_ = 4096}})} {}
};

RuntimeDiagnostics::RuntimeDiagnostics() : impl_{std::make_unique<Impl>()} {}
RuntimeDiagnostics::~RuntimeDiagnostics() = default;

void RuntimeDiagnostics::RecordAdmissionRefusal(const Context context,
    const BoundaryCommand& command, const CommandIngress::AdmissionStatus status) noexcept {
    if (status == CommandIngress::AdmissionStatus::accepted || !impl_->logger.valid()) return;
    const sub0log::Logger::ScopedBind bind{impl_->logger};
    sub0log_warning(sub0log::SubsystemId{1},
        "schema=1 event=1 run={} tick={} status={} action={} generation={} kind={} slot={} x={} y={} radius={} strength={} end_x={} end_y={}",
        context.run_id, context.completed_tick, static_cast<std::uint64_t>(status),
        static_cast<std::uint64_t>(command.action), command.generation,
        static_cast<std::uint64_t>(command.field.kind), static_cast<std::uint64_t>(command.field.slot),
        command.field.center.x, command.field.center.y, command.field.radius, command.field.strength,
        command.field.end.x, command.field.end.y);
}

void RuntimeDiagnostics::RecordStructuralRefusal(const Context context,
    const BoundaryCommand& command, const StructuralCommandResult result) noexcept {
    if (result == StructuralCommandResult::applied || !impl_->logger.valid()) return;
    const sub0log::Logger::ScopedBind bind{impl_->logger};
    sub0log_warning(sub0log::SubsystemId{1},
        "schema=1 event=2 run={} tick={} status={} action={} generation={}",
        context.run_id, context.completed_tick, static_cast<std::uint64_t>(result),
        static_cast<std::uint64_t>(command.action), command.generation);
}

void RuntimeDiagnostics::RecordMissionSummary(const std::uint64_t run_id,
    const ReclamationMissionProgress& progress) noexcept {
    if (progress.outcome == ReclamationMissionOutcome::active || !impl_->logger.valid()) return;
    const sub0log::Logger::ScopedBind bind{impl_->logger};
    sub0log_info(sub0log::SubsystemId{1},
        "schema=1 event=3 run={} tick={} status={} reclaimed={} target={} deadline={}",
        run_id, progress.completed_tick, static_cast<std::uint64_t>(progress.outcome),
        progress.reclaimed, progress.settings.target_reclaimed, progress.settings.deadline_ticks);
}

RuntimeDiagnostics::Statistics RuntimeDiagnostics::GetStatistics() const noexcept {
    const auto stats = impl_->logger.stats();
    return {impl_->logger.valid(), stats.droppedRecords_, stats.truncatedRecords_};
}

std::span<const std::byte> RuntimeDiagnostics::GetImage() const noexcept {
    return impl_->logger.valid() ? std::span<const std::byte>{impl_->storage}
        : std::span<const std::byte>{};
}

bool RuntimeDiagnostics::WriteDecoded(std::ostream& output) const {
    const auto stats = GetStatistics();
    output << "diagnostics valid=" << stats.valid << " dropped=" << stats.dropped_records
        << " truncated=" << stats.truncated_records << '\n';
    auto reader = sub0log::SegmentReader::open(GetImage());
    if (!reader.valid()) return false;
    sub0log::Decoder decoder;
    const auto records = decoder.decodeAll(reader);
    for (const auto& record : records) output << sub0log::Decoder::format(record) << '\n';
    output << "diagnostics decoded=" << records.size() << " unreadable=" << reader.unreadableBytes()
        << " undecodable=" << decoder.undecodableRecords() << " skipped=" << decoder.skippedRecords() << '\n';
    return static_cast<bool>(output) && reader.unreadableBytes() == 0
        && decoder.undecodableRecords() == 0;
}

}
