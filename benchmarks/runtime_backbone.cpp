#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/runtime/IntentDelivery.hpp>
#include <crucible/runtime/ReferenceMissionRoute.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#if CRUCIBLE_ENABLE_DIAGNOSTICS
#include <crucible/runtime/RuntimeDiagnostics.hpp>
#endif
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <string_view>
#ifdef _WIN32
#include <malloc.h>
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
        Require((argc==3 || argc==4) && std::string_view{argv[1]}=="--arm",
            "usage: crucible_runtime_bench --arm integrated|direct [--skip-missions]");
        const std::string_view name{argv[2]};
        Require(name=="integrated" || name=="direct","unknown execution arm");
        Require(argc==3 || std::string_view{argv[3]}=="--skip-missions","unknown benchmark option");
        const auto arm=name=="integrated" ? Arm::integrated : Arm::direct;
        std::cout << std::setprecision(17);
        AdmissionBenchmark(arm,name);
        for (const auto population : {64U,2048U}) OrdinaryBenchmark(arm,name,population);
#if CRUCIBLE_ENABLE_DIAGNOSTICS
        DiagnosticsBenchmark(arm,name);
#endif
        if (argc==3) { MissionBenchmark(arm,name,false); MissionBenchmark(arm,name,true); }
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
