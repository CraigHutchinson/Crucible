/** Receives headless production boundaries and the existing reference missions.
 * Scale mode freezes geometry, execution and evolving field input, timing complete
 * InspectorSession pumps while reporting replay state outside the measured region.
 */
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>
#ifdef _WIN32
#include <malloc.h>
#endif

#include "crucible/presentation/FieldTool.hpp"
#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/runtime/InspectorSession.hpp"
#include "crucible/runtime/IntentDelivery.hpp"
#include "crucible/runtime/ReferenceMissionRoute.hpp"
#if CRUCIBLE_ENABLE_DIAGNOSTICS
#include "crucible/runtime/RuntimeDiagnostics.hpp"
#endif

namespace allocation_probe {
/// Only successful replaceable C++ new calls on the coordinator thread are counted.
struct Counts { std::uint64_t calls{}, bytes{}; };
// [R12-MEASURE] Write-only diagnostic state; no production behavior depends on it.
thread_local Counts counts;
thread_local bool enabled{};
void Record(std::size_t size) noexcept {
    if (enabled) { ++counts.calls; counts.bytes += size; }
}
[[nodiscard]] void* Allocate(std::size_t size, std::size_t alignment = 0) {
    for (;;) {
        void* memory = nullptr;
        const auto physical_size = std::max(size, std::size_t{1});
        if (!alignment) memory = std::malloc(physical_size);
        else {
#ifdef _WIN32
            memory = _aligned_malloc(physical_size, alignment);
#else
            if (posix_memalign(&memory, alignment, physical_size) != 0) memory = nullptr;
#endif
        }
        if (memory) { Record(size); return memory; }
        if (const auto handler = std::get_new_handler()) handler();
        else throw std::bad_alloc{};
    }
}
void FreeAligned(void* memory) noexcept {
#ifdef _WIN32
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}
}
void* operator new(std::size_t n) { return allocation_probe::Allocate(n); }
void* operator new[](std::size_t n) { return allocation_probe::Allocate(n); }
void* operator new(std::size_t n, std::align_val_t a) { return allocation_probe::Allocate(n, static_cast<std::size_t>(a)); }
void* operator new[](std::size_t n, std::align_val_t a) { return allocation_probe::Allocate(n, static_cast<std::size_t>(a)); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { try { return ::operator new(n); } catch (...) { return nullptr; } }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { try { return ::operator new[](n); } catch (...) { return nullptr; } }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { try { return ::operator new(n,a); } catch (...) { return nullptr; } }
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { try { return ::operator new[](n,a); } catch (...) { return nullptr; } }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { allocation_probe::FreeAligned(p); }
void operator delete[](void* p, std::align_val_t) noexcept { allocation_probe::FreeAligned(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { allocation_probe::FreeAligned(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { allocation_probe::FreeAligned(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { allocation_probe::FreeAligned(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { allocation_probe::FreeAligned(p); }

namespace {
using namespace crucible;
using namespace crucible::runtime;
using Clock = std::chrono::steady_clock;
using Arm = InspectorSession::ExecutionPath;
constexpr auto tick_time = std::chrono::nanoseconds{16'666'667};
/// Elapsed operation time plus requested C++ allocation volume, not live memory.
struct Measurement { double us{}; allocation_probe::Counts allocations{}; };
/// Restores the diagnostic switch on exception and permits nested counting scopes.
struct ProbeScope {
    const bool previous{allocation_probe::enabled};
    ProbeScope() noexcept { allocation_probe::enabled = true; }
    ~ProbeScope() { allocation_probe::enabled = previous; }
};
template<class Operation>
[[nodiscard]] Measurement Measure(Operation&& operation) {
    ProbeScope scope;
    const auto before = allocation_probe::counts;
    const auto begin = Clock::now();
    operation();
    const auto end = Clock::now();
    const auto after = allocation_probe::counts;
    return {std::chrono::duration<double,std::micro>(end-begin).count(),
        {after.calls-before.calls, after.bytes-before.bytes}};
}
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void Accepted(CommandIngress::Admission admission) {
    Require(admission.status == CommandIngress::AdmissionStatus::accepted, "benchmark admission failed");
}
void Emit(std::string_view arm, std::string_view workload, std::size_t population,
        std::span<const Measurement> measurements) {
    std::array<double, 10'000> ordered{};
    Require(!measurements.empty() && measurements.size() <= ordered.size(), "measurement capacity");
    double total{};
    allocation_probe::Counts counts{};
    for (std::size_t i=0; i<measurements.size(); ++i) {
        ordered[i] = measurements[i].us; total += measurements[i].us;
        counts.calls += measurements[i].allocations.calls;
        counts.bytes += measurements[i].allocations.bytes;
    }
    const auto used = std::span{ordered}.first(measurements.size());
    std::ranges::sort(used);
    const auto n = used.size();
    std::cout << "{\"arm\":\"" << arm << "\",\"workload\":\"" << workload
        << "\",\"entities\":" << population << ",\"operations\":" << n
        << ",\"total_us\":" << total << ",\"median_us\":" << (used[(n-1)/2]+used[n/2])/2
        << ",\"p95_us\":" << used[(95*n+99)/100-1] << ",\"p99_us\":" << used[(99*n+99)/100-1]
        << ",\"min_us\":" << used.front() << ",\"max_us\":" << used.back()
        << ",\"new_calls\":" << counts.calls << ",\"new_bytes\":" << counts.bytes << "}\n";
}
void EmitOne(std::string_view arm, std::string_view workload, std::size_t population, Measurement result) {
    Emit(arm,workload,population,std::span{&result,1});
}
void EmitState(std::string_view arm, std::string_view workload, std::size_t population,
        const InspectorSession& run) {
    double checksum{};
    for (const auto& sample : run.GetSnapshot().GetSamples())
        checksum += sample.position.x + sample.position.y + sample.velocity.x + sample.velocity.y;
    const auto mission = run.GetMission();
    const auto info = *run.GetSnapshot().GetInfo();
    std::cout << "{\"arm\":\"" << arm << "\",\"workload\":\"" << workload
        << "\",\"entities\":" << population << ",\"completed_tick\":" << info.completed_tick
        << ",\"snapshot_samples\":" << run.GetSnapshot().GetSamples().size()
        << ",\"checksum\":" << checksum << ",\"closed\":"
        << (run.GetStatus()==ClockDriver::Status::closed ? "true" : "false")
        << ",\"outcome\":\"" << (!mission ? "none" : mission->outcome==ReclamationMissionOutcome::won ? "won" :
            mission->outcome==ReclamationMissionOutcome::lost ? "lost" : "active")
        << "\",\"reclaimed\":" << (mission ? mission->reclaimed : 0) << ",\"trace\":[";
    bool first=true;
    for (const auto& item : run.GetTrace()) {
        if (!first) std::cout << ',';
        first=false;
        const auto& edit=item.command.field;
        std::cout << "{\"tick\":" << item.tick << ",\"sequence\":" << item.sequence
            << ",\"action\":" << static_cast<int>(item.command.action)
            << ",\"generation\":" << item.command.generation << ",\"result\":" << static_cast<int>(item.result)
            << ",\"kind\":" << static_cast<int>(edit.kind) << ",\"slot\":" << edit.slot
            << ",\"x\":" << edit.center.x << ",\"y\":" << edit.center.y
            << ",\"end_x\":" << edit.end.x << ",\"end_y\":" << edit.end.y
            << ",\"radius\":" << edit.radius << ",\"strength\":" << edit.strength << '}';
    }
    std::cout << "]}\n";
}
/// Explicit opt-in headless workload; ordinary measurements disable stage clocks.
struct ScaleSettings
{
    std::size_t population{}, workers{1}, partitions{}, warmup{120}, ticks{300};
    std::string_view route{"gather"};
    bool profileStages{};
};

/** Hashes semantic words in fixed low-byte-first order, never object padding.
 * Float payloads retain every bit including signed zero. This replay digest is
 * a compact drift detector, not a collision-free proof or cryptographic hash.
 */
class StateDigest
{
public:
    constexpr void append(std::uint64_t word) noexcept
    {
        for (unsigned byte = 0; byte != 8; ++byte) {
            value_ ^= word & 255U;
            value_ *= 1099511628211ULL;
            word >>= 8;
        }
    }
    constexpr void appendFloat(float value) noexcept
    {
        append(std::bit_cast<std::uint32_t>(value));
    }
    void appendField(const FieldEdit& field) noexcept
    {
        append(static_cast<std::uint64_t>(field.kind)); append(field.slot);
        appendFloat(field.center.x); appendFloat(field.center.y);
        appendFloat(field.radius); appendFloat(field.strength);
        appendFloat(field.end.x); appendFloat(field.end.y);
    }
    [[nodiscard]] constexpr std::uint64_t value() const noexcept { return value_; }
private:
    std::uint64_t value_{14695981039346656037ULL};
};

[[nodiscard]] std::uint64_t digestState(const presentation::ScenarioSnapshot& frame)
{
    StateDigest digest;
    const auto info = frame.GetInfo().value();
    digest.append(info.grid.columns); digest.append(info.grid.rows); digest.appendFloat(info.grid.cell_size);
    digest.append(info.completed_tick); digest.append(info.samples); digest.append(info.fields); digest.append(info.cells);
    for (const auto& sample : frame.GetSamples()) {
        digest.append(sample.id.value); digest.append(static_cast<std::uint64_t>(sample.activity));
        digest.appendFloat(sample.position.x); digest.appendFloat(sample.position.y);
        digest.appendFloat(sample.velocity.x); digest.appendFloat(sample.velocity.y);
    }
    for (const auto& field : frame.GetFields()) digest.appendField(field);
    for (const auto cell : frame.GetBlight()) digest.append(cell);
    for (const auto stock : frame.GetStocks()) digest.append(stock);
    digest.append(info.biomass.has_value());
    if (info.biomass) {
        const auto& ledger = *info.biomass;
        for (const auto word : {ledger.initial_total, ledger.remaining_stock, ledger.mobile_mass,
                ledger.reserve, ledger.harvested, ledger.work_actions, ledger.structure_mass, ledger.lost_mass})
            digest.append(word);
    }
    digest.append(info.structural.has_value());
    if (info.structural) {
        const auto& state = *info.structural;
        digest.appendFloat(state.settings.relay_center.x); digest.appendFloat(state.settings.relay_center.y);
        digest.appendFloat(state.settings.eligibility_radius); digest.appendFloat(state.settings.protection_radius);
        digest.append(state.settings.hold_ticks); digest.append(state.occupied); digest.append(state.generation);
        digest.append(state.hold_ticks); digest.append(state.eligible_mobile);
        for (const auto id : state.members) digest.append(id.value);
    }
    return digest.value();
}

void scaleBenchmark(Arm arm, std::string_view name, const ScaleSettings& settings)
{
    ScenarioSettings scenario;
    scenario.population = settings.population;
    if (settings.population == 100000) scenario.grid = {400, 250, 1};
    else if (settings.population == 150000) scenario.grid = {500, 300, 1};
    const auto observationCapacity = settings.profileStages ? settings.warmup + settings.ticks : 0;
    std::unique_ptr<InspectorSession> run;
    const auto startup = Measure([&] {
        run = std::make_unique<InspectorSession>(scenario, HeadlessSession::Limits{64, 4096},
            std::nullopt, arm, InspectorSession::Diagnostics::disabled,
            InspectorSession::ObservationSettings{observationCapacity, settings.profileStages},
            ExecutionSettings{settings.workers, settings.partitions});
    });
    const auto execution = run->getRowExecution();
    const auto storage = run->getRowExecutionStorage();
    std::cout << "{\"arm\":\"" << name << "\",\"workload\":\"scale_settings\",\"entities\":" << settings.population
        << ",\"schema\":1,\"columns\":" << scenario.grid.columns << ",\"rows\":" << scenario.grid.rows
        << ",\"cell_size\":" << scenario.grid.cell_size << ",\"field_capacity\":" << scenario.fieldCapacity
        << ",\"neighbor_radius\":" << scenario.steering.neighbor_radius
        << ",\"separation_strength\":" << scenario.steering.separation_strength
        << ",\"maximum_acceleration\":" << scenario.steering.maximum_acceleration
        << ",\"maximum_speed\":" << scenario.steering.maximum_speed
        << ",\"initial_stock_per_cell\":" << scenario.resources.initial_stock_per_cell
        << ",\"mass_per_sample\":" << scenario.resources.mass_per_sample
        << ",\"initial_reserve\":" << scenario.resources.initial_reserve
        << ",\"actions_per_tick\":" << scenario.resources.actions_per_tick
        << ",\"mission\":false,\"structural\":false,\"diagnostics\":false"
        << ",\"requested_workers\":" << settings.workers << ",\"requested_partitions\":" << settings.partitions
        << ",\"resolved_workers\":" << execution.workers << ",\"resolved_partitions\":" << execution.partitions
        << ",\"query_scratch_capacity\":" << storage.queryScratchCapacity
        << ",\"task_capacity\":" << storage.taskCapacity << ",\"queue_capacity\":" << storage.queueCapacity
        << ",\"warmup_ticks\":" << settings.warmup << ",\"measured_ticks\":" << settings.ticks
        << ",\"tick_ns\":" << tick_time.count() << ",\"route\":\"" << settings.route
        << "\",\"mutation_interval\":6,\"tool_radius\":16,\"tool_magnitude\":4"
        << ",\"command_capacity\":64,\"trace_capacity\":4096,\"observation_capacity\":" << observationCapacity
        << ",\"profile_stages\":" << (settings.profileStages ? "true" : "false") << "}\n";
    EmitOne(name, "scale_startup", settings.population, startup);
    const auto pump = [&] {
        const auto result = run->TryPump(tick_time);
        Require(result.status == ClockDriver::Status::running && result.advanced_ticks == 1,
            "scale pump did not complete one evolving boundary");
    };
    for (std::size_t tick = 0; tick < settings.warmup; ++tick) pump();
    const auto initialDigest = digestState(run->GetSnapshot());
    const auto fallbackStart = run->getRowFallbackCount();
    std::vector<Measurement> measurements(settings.ticks);
    const float width = static_cast<float>(scenario.grid.columns) * scenario.grid.cell_size;
    const float height = static_cast<float>(scenario.grid.rows) * scenario.grid.cell_size;
    std::size_t mutations{};
    for (std::size_t tick = 0; tick < settings.ticks; ++tick) {
        if (tick % 6 == 0) {
            // Match Phase14's world-space route through the shared field intent builder.
            const auto kind = settings.route == "gather" ? mutations % 3 : 0;
            const auto tool = kind == 0 ? presentation::FieldTool::flow :
                kind == 1 ? presentation::FieldTool::repel : presentation::FieldTool::attract;
            const float y = (mutations % 2 == 0 ? 0.49F : 0.51F) * height;
            const Position center = kind == 0 ? Position{0.02F * width, y} :
                Position{(kind == 1 ? 0.25F : 0.85F) * width, 0.5F * height};
            const auto edit = presentation::TryBuildFieldEdit({tool, kind, 16, 4}, center,
                scenario.fieldCapacity, {0.98F * width, y});
            Require(edit.has_value(), "scale route could not build field intent");
            Accepted(run->TryAdmitFieldEdit(*edit));
            ++mutations;
        }
        measurements[tick] = Measure(pump);
    }
    Emit(name, settings.profileStages ? "scale_tick_capture_diagnostic" : "scale_tick_capture_steady",
        settings.population, measurements);
    EmitState(name, "scale_state", settings.population, *run);
    const auto info = run->GetSnapshot().GetInfo().value();
    const auto summary = run->GetSummary();
    Require(info.completed_tick == settings.warmup + settings.ticks && summary.applied_commands == mutations &&
        summary.ingress_accepted == mutations && summary.ingress_pending == 0 && summary.ingress_rejected == 0,
        "scale completion or command accounting changed");
    StateDigest traceDigest;
    for (const auto& item : run->GetTrace()) {
        traceDigest.append(item.tick); traceDigest.append(item.sequence);
        traceDigest.append(static_cast<std::uint64_t>(item.command.action)); traceDigest.append(item.command.generation);
        traceDigest.append(static_cast<std::uint64_t>(item.result)); traceDigest.appendField(item.command.field);
    }
    std::cout << "{\"arm\":\"" << name << "\",\"workload\":\"scale_receipt\",\"entities\":" << settings.population
        << ",\"digest_schema\":1,\"state_digest\":\"" << std::hex << digestState(run->GetSnapshot())
        << "\",\"initial_digest\":\"" << initialDigest << "\",\"trace_digest\":\"" << traceDigest.value() << std::dec
        << "\",\"completed_tick\":" << info.completed_tick << ",\"applied_commands\":" << summary.applied_commands
        << ",\"accepted_commands\":" << summary.ingress_accepted << ",\"pending_commands\":" << summary.ingress_pending
        << ",\"rejected_commands\":" << summary.ingress_rejected
        << ",\"discarded_scaled_nanoseconds\":" << summary.discarded_scaled_nanoseconds
        << ",\"work_actions\":" << info.biomass->work_actions << ",\"harvested\":" << info.biomass->harvested
        << ",\"row_fallbacks_start\":" << fallbackStart << ",\"row_fallbacks_end\":" << run->getRowFallbackCount()
        << ",\"tick_observation_count\":" << run->getTickObservations().size()
        << ",\"dropped_tick_observations\":" << run->getDroppedTickObservations() << "}\n";
    if (settings.profileStages) {
        Require(run->getTickObservations().size() == observationCapacity && run->getDroppedTickObservations() == 0,
            "scale stage observations incomplete");
        for (const auto& observation : run->getTickObservations().subspan(settings.warmup)) {
            Require(observation.simulation.has_value(), "scale stage statistics missing");
            const auto& stages = *observation.simulation;
            std::cout << "{\"arm\":\"" << name << "\",\"workload\":\"scale_stages\",\"entities\":" << settings.population
                << ",\"tick\":" << observation.completedTick << ",\"boundary_ns\":" << observation.boundary.count()
                << ",\"gather_ns\":" << stages.gather.count() << ",\"index_ns\":" << stages.index.count()
                << ",\"propose_ns\":" << stages.propose.count() << ",\"commit_ns\":" << stages.commit.count()
                << ",\"resources_ns\":" << stages.resources.count() << ",\"rebuild_ns\":" << stages.rebuild.count()
                << ",\"input_rows\":" << stages.inputRows << ",\"query_rows\":" << stages.queryRows
                << ",\"occupied_cells\":" << stages.occupiedCells << ",\"query_scratch_capacity\":" << stages.queryScratchCapacity
                << ",\"workers\":" << stages.workers << ",\"partitions\":" << stages.partitions
                << ",\"task_capacity\":" << stages.taskCapacity << "}\n";
        }
    }
}

void AdmissionBenchmark(Arm arm, std::string_view name) {
    CommandIngress ingress{{1,4}};
    std::unique_ptr<IntentDelivery> delivery;
    const auto startup=Measure([&] { if (arm==Arm::integrated) delivery=std::make_unique<IntentDelivery>(ingress,1); });
    EmitOne(name,"pub_route_startup",0,startup);
    const BoundaryCommand command{FieldEdit{FieldEditKind::set,0,{8,8},8,4}};
    std::array<AdmittedCommand,1> drained{};
    const auto admit=[&] { Accepted(delivery ? delivery->TryAdmitCommands(std::span{&command,1}).admission
        : ingress.TryAdmitCommands(std::span{&command,1})); };
    EmitOne(name,"pub_admission_first",0,Measure(admit));
    Require(ingress.TryDrainThrough(ingress.CaptureCutoff(),drained).has_value(),"benchmark first ingress drain");
    std::array<Measurement,10'000> samples{};
    for (auto& sample : samples) {
        sample=Measure(admit);
        Require(ingress.TryDrainThrough(ingress.CaptureCutoff(),drained).has_value(),"benchmark ingress drain");
    }
    Emit(name,"pub_admission_steady",0,samples);
    std::cout << "{\"arm\":\"" << name << "\",\"workload\":\"pub_admission_state\",\"entities\":0,\"accepted\":"
        << ingress.GetStatistics().accepted << ",\"last_sequence\":" << drained[0].sequence << "}\n";
}
void Pump(InspectorSession& run) {
    Require(run.TryPump(tick_time).advanced_ticks==1,"benchmark pump did not complete one boundary");
}
void OrdinaryBenchmark(Arm arm,std::string_view name,std::size_t population) {
    std::unique_ptr<InspectorSession> run;
    EmitOne(name,"inspector_startup",population,Measure([&] {
        run=std::make_unique<InspectorSession>(population,HeadlessSession::Limits{64,4096},std::nullopt,false,arm);
    }));
    EmitOne(name,"tick_capture_first",population,Measure([&] { Pump(*run); }));
    for (int i=0;i<15;++i) Pump(*run);
    std::array<Measurement,120> pumps{};
    std::array<Measurement,2> admissions{};
    std::size_t admitted{};
    for (auto& result : pumps) {
        const auto tick=run->GetSnapshot().GetInfo()->completed_tick;
        if (const auto edit=GetReferenceMissionRouteEdit(tick))
            admissions.at(admitted++)=Measure([&] { Accepted(run->TryAdmitFieldEdit(*edit)); });
        result=Measure([&] { Pump(*run); });
    }
    Emit(name,"production_admission_steady",population,std::span{admissions}.first(admitted));
    Emit(name,"tick_capture_steady",population,pumps);
    EmitState(name,"inspector_state",population,*run);
}
void MissionBenchmark(Arm arm,std::string_view name,bool structural) {
    constexpr std::size_t population=2048;
    const std::string_view label=structural ? "structural" : "reference";
    std::unique_ptr<InspectorSession> run;
    const auto startup=Measure([&] { run=std::make_unique<InspectorSession>(population,
        HeadlessSession::Limits{64,4096},ReclamationMissionSettings{},structural,arm); });
    EmitOne(name,structural ? "structural_startup" : "reference_startup",population,startup);
    std::array<Measurement,900> pumps{};
    std::array<Measurement,32> admissions{};
    std::size_t ticks{}, admitted{};
    bool gathering{}, fused{};
    const auto mission=Measure([&] {
        while (run->GetMission()->outcome==ReclamationMissionOutcome::active) {
            const auto tick=run->GetMission()->completed_tick;
            if (!structural || (!gathering && run->GetMission()->reclaimed<1780)) {
                if (const auto edit=GetReferenceMissionRouteEdit(tick))
                    admissions.at(admitted++)=Measure([&] { Accepted(run->TryAdmitFieldEdit(*edit)); });
            } else if (!gathering) {
                admissions.at(admitted++)=Measure([&] { Accepted(run->TryAdmitFieldEdit(
                    FieldEdit{FieldEditKind::set,0,{48.5F,16.5F},8,4})); });
                gathering=true;
            }
            if (structural && gathering && !fused &&
                run->GetSnapshot().GetInfo()->structural->eligible_mobile>=StructuralSettings::cost) {
                admissions.at(admitted++)=Measure([&] { Accepted(run->TryFuseRelay()); }); fused=true;
            }
            pumps.at(ticks++)=Measure([&] { Pump(*run); });
        }
    });
    Require(run->GetMission()->outcome==ReclamationMissionOutcome::won &&
        run->GetMission()->completed_tick==(structural ? 448U : 267U) &&
        run->GetStatus()==ClockDriver::Status::closed,"receiving mission terminal changed");
    EmitOne(name,structural ? "structural_complete_mission" : "reference_complete_mission",population,mission);
    Emit(name,structural ? "structural_tick_capture_mission" : "reference_tick_capture_mission",
        population,std::span{pumps}.first(ticks));
    Emit(name,structural ? "structural_admission" : "reference_admission",population,std::span{admissions}.first(admitted));
    EmitState(name,label,population,*run);
}
#if CRUCIBLE_ENABLE_DIAGNOSTICS
void DiagnosticsBenchmark(Arm arm, std::string_view name) {
    std::unique_ptr<InspectorSession> run;
    EmitOne(name, "diagnostic_inspector_startup", 64, Measure([&] {
        run = std::make_unique<InspectorSession>(64, HeadlessSession::Limits{1,16},
            std::nullopt, true, arm, InspectorSession::Diagnostics::bounded);
    }));
    Require(run->GetDiagnostics() && run->GetDiagnostics()->GetStatistics().valid,
        "bounded diagnostic benchmark initialized");
    run->Pause();
    Accepted(run->TryFuseRelay());
    const auto refuse = [&] {
        Require(run->TryFuseRelay().status == CommandIngress::AdmissionStatus::full,
            "diagnostic benchmark preserves full refusal");
    };
    EmitOne(name, "diagnostic_refusal_first", 64, Measure(refuse));
    std::array<Measurement,2000> refusals{};
    for (auto& result : refusals) result = Measure(refuse);
    Emit(name, "diagnostic_refusal_to_exhaustion", 64, refusals);
    const auto statistics = run->GetDiagnostics()->GetStatistics();
    Require(statistics.dropped_records > 0 && statistics.truncated_records == 0,
        "diagnostic benchmark reaches counted exhaustion");
    std::cout << "{\"arm\":\"" << name
        << "\",\"workload\":\"diagnostic_state\",\"entities\":64,\"dropped\":"
        << statistics.dropped_records << ",\"completed_tick\":"
        << run->GetSnapshot().GetInfo()->completed_tick << "}\n";
}
#endif
}
int main(int argc,char** argv) {
    try {
        Require(argc >= 3 && std::string_view{argv[1]} == "--arm",
            "usage: crucible_runtime_bench --arm integrated|direct [--skip-missions] "
            "[--scale 2048|100000|150000 --workers N --partitions N --warmup N --ticks N "
            "--route flow|gather --profile-stages]");
        const std::string_view name{argv[2]};
        Require(name == "integrated" || name == "direct", "unknown execution arm");
        const auto arm = name == "integrated" ? Arm::integrated : Arm::direct;
        bool skipMissions{}, scaleOptions{};
        ScaleSettings settings;
        for (int index = 3; index < argc; ++index) {
            const std::string_view option{argv[index]};
            if (option == "--skip-missions") { skipMissions = true; continue; }
            scaleOptions = true;
            if (option == "--profile-stages") { settings.profileStages = true; continue; }
            Require(index + 1 < argc, "benchmark option requires a value");
            const std::string_view value{argv[++index]};
            if (option == "--route") {
                Require(value == "flow" || value == "gather", "unknown scale route");
                settings.route = value;
                continue;
            }
            std::size_t number{};
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
            Require(parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size(), "invalid benchmark integer");
            if (option == "--scale") settings.population = number;
            else if (option == "--workers") settings.workers = number;
            else if (option == "--partitions") settings.partitions = number;
            else if (option == "--warmup") settings.warmup = number;
            else if (option == "--ticks") settings.ticks = number;
            else throw std::invalid_argument("unknown benchmark option");
        }
        std::cout << std::setprecision(17);
        if (scaleOptions) {
            Require(settings.population == 2048 || settings.population == 100000 || settings.population == 150000,
                "scale must be 2048, 100000 or 150000");
            Require(settings.workers >= 1 && settings.workers <= 32 && settings.partitions <= 128,
                "scale worker or partition bound exceeded");
            Require(settings.warmup <= 10000 && settings.ticks >= 1 && settings.ticks <= 10000,
                "scale warmup/tick bound exceeded");
            Require(!settings.profileStages || arm == Arm::integrated, "stage diagnostics require integrated execution");
            Require(!skipMissions, "scale is a separate workload; --skip-missions is not a scale option");
            scaleBenchmark(arm, name, settings);
            return 0;
        }
        AdmissionBenchmark(arm,name);
        for (const auto population : {64U,2048U}) OrdinaryBenchmark(arm,name,population);
#if CRUCIBLE_ENABLE_DIAGNOSTICS
        DiagnosticsBenchmark(arm,name);
#endif
        if (!skipMissions) { MissionBenchmark(arm,name,false); MissionBenchmark(arm,name,true); }
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
