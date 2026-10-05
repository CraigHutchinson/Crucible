#include <crucible/runtime/RuntimeDiagnostics.hpp>

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <vector>
#include <crucible/contracts/ReclamationMission.hpp>
#include <crucible/contracts/StructuralState.hpp>
#include <sub0log/log.hpp>
#include <sub0log/reader.hpp>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
using namespace crucible;
using namespace crucible::runtime;

void CheckFieldsAndBindings() {
    RuntimeDiagnostics diagnostics, other;
    Require(diagnostics.GetStatistics().valid && diagnostics.GetImage().size() == 65536,
        "startup fixed segment");
    Require(reinterpret_cast<std::uintptr_t>(diagnostics.GetImage().data()) % 64 == 0,
        "segment alignment");
    alignas(64) std::array<std::byte, 8192> external_storage{};
    auto external = sub0log::Logger::createInMemory(external_storage,
        {.segment_ = {.chunkBytes_ = 4096}});
    BoundaryCommand command{FieldEdit{FieldEditKind::set_flow, 3, {4, 5}, 6, 7, {8, 9}}};
    ReclamationMissionProgress progress{{99, 100}, 101, 12, ReclamationMissionOutcome::won};
    {
        const sub0log::Logger::ScopedBind binding{external};
        diagnostics.RecordAdmissionRefusal({7, 11}, command, CommandIngress::AdmissionStatus::full);
        Require(sub0log::Logger::active() == &external, "emission restores previous binding");
        command.field = {};
        auto shatter = BoundaryCommand::Shatter(44);
        other.RecordStructuralRefusal({8, 13}, shatter, StructuralCommandResult::empty);
        Require(sub0log::Logger::active() == &external, "interleaved sink restores binding");
        diagnostics.RecordStructuralRefusal({7, 12}, shatter, StructuralCommandResult::stale_generation);
        shatter.generation = 999;
        diagnostics.RecordMissionSummary(7, progress);
        progress = {};
        diagnostics.RecordMissionSummary(7, progress);
        diagnostics.RecordAdmissionRefusal({7, 12}, command, CommandIngress::AdmissionStatus::accepted);
        diagnostics.RecordStructuralRefusal({7, 12}, command, StructuralCommandResult::applied);
    }
    Require(sub0log::Logger::active() == nullptr, "no persistent logger binding");
    auto reader = sub0log::SegmentReader::open(diagnostics.GetImage());
    sub0log::Decoder decoder;
    const auto records = decoder.decodeAll(reader);
    Require(reader.valid() && records.size() == 3 && reader.unreadableBytes() == 0
        && decoder.undecodableRecords() == 0, "actual pinned decoder parity");
    const auto& admission = records[0];
    Require(admission.args_.size() == 13 && admission.site_->severity_ == sub0log::Severity::Warning
        && admission.site_->subsystem_ == sub0log::SubsystemId{1}, "admission schema");
    Require(std::get<std::uint64_t>(admission.args_[0]) == 7
        && std::get<std::uint64_t>(admission.args_[1]) == 11
        && std::get<std::uint64_t>(admission.args_[2]) == static_cast<std::uint64_t>(CommandIngress::AdmissionStatus::full)
        && std::get<std::uint64_t>(admission.args_[5]) == static_cast<std::uint64_t>(FieldEditKind::set_flow)
        && std::get<std::uint64_t>(admission.args_[6]) == 3,
        "owned refusal identity and operation");
    Require(std::get<double>(admission.args_[7]) == 4 && std::get<double>(admission.args_[8]) == 5
        && std::get<double>(admission.args_[9]) == 6 && std::get<double>(admission.args_[10]) == 7
        && std::get<double>(admission.args_[11]) == 8 && std::get<double>(admission.args_[12]) == 9,
        "copied field source survives mutation");
    Require(records[1].args_.size() == 5
        && std::get<std::uint64_t>(records[1].args_[2]) == static_cast<std::uint64_t>(StructuralCommandResult::stale_generation)
        && std::get<std::uint64_t>(records[1].args_[4]) == 44,
        "copied structural refusal");
    const auto& mission = records[2];
    Require(mission.args_.size() == 6 && mission.site_->severity_ == sub0log::Severity::Info
        && std::get<std::uint64_t>(mission.args_[0]) == 7
        && std::get<std::uint64_t>(mission.args_[1]) == 12
        && std::get<std::uint64_t>(mission.args_[2]) == static_cast<std::uint64_t>(ReclamationMissionOutcome::won)
        && std::get<std::uint64_t>(mission.args_[3]) == 101
        && std::get<std::uint64_t>(mission.args_[4]) == 99
        && std::get<std::uint64_t>(mission.args_[5]) == 100, "copied mission summary");
    auto other_reader = sub0log::SegmentReader::open(other.GetImage());
    sub0log::Decoder other_decoder;
    const auto other_records = other_decoder.decodeAll(other_reader);
    Require(other_records.size() == 1 && std::get<std::uint64_t>(other_records[0].args_[0]) == 8,
        "sink images do not cross-contaminate");
    std::ostringstream output;
    Require(diagnostics.WriteDecoded(output) && output.str().find("dropped=0 truncated=0") != std::string::npos
        && output.str().find("decoded=3 unreadable=0 undecodable=0") != std::string::npos,
        "shutdown decoded export includes health");
    std::ostringstream failed;
    failed.setstate(std::ios::badbit);
    Require(!diagnostics.WriteDecoded(failed), "output failure does not succeed");
}

void CheckExhaustionAndTeardown() {
    std::vector<std::byte> owned_image;
    {
        RuntimeDiagnostics diagnostics;
        for (std::uint64_t tick = 0; tick < 10000; ++tick)
            diagnostics.RecordStructuralRefusal({1, tick}, BoundaryCommand::Fuse(), StructuralCommandResult::occupied);
        const auto stats = diagnostics.GetStatistics();
        Require(stats.valid && stats.dropped_records > 0 && stats.truncated_records == 0,
            "bounded exhaustion counts drops without growth");
        Require(diagnostics.GetImage().size() == 65536, "exhaustion cannot grow image");
        std::ostringstream output;
        Require(diagnostics.WriteDecoded(output), "exhausted committed prefix remains decodable");
        const auto image = diagnostics.GetImage();
        owned_image.assign(image.begin(), image.end());
    }
    Require(sub0log::Logger::active() == nullptr, "destroyed diagnostics leave no binding");
    auto reader = sub0log::SegmentReader::open(owned_image);
    sub0log::Decoder decoder;
    const auto records = decoder.decodeAll(reader);
    Require(!records.empty() && reader.unreadableBytes() == 0 && decoder.undecodableRecords() == 0,
        "owned segment copy survives owner teardown");
    RuntimeDiagnostics replacement;
    replacement.RecordStructuralRefusal({2, 1}, BoundaryCommand::Fuse(), StructuralCommandResult::disabled);
    auto replacement_reader = sub0log::SegmentReader::open(replacement.GetImage());
    sub0log::Decoder replacement_decoder;
    Require(replacement_decoder.decodeAll(replacement_reader).size() == 1,
        "writer cache and sites use fresh logger generation after teardown");
}
}

int main() {
    try { CheckFieldsAndBindings(); CheckExhaustionAndTeardown(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
