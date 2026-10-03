// Bounded timing investigation through the production coordinator; replay evidence
// describes deterministic behavior, not human comprehension or balanced difficulty.
#include <crucible/runtime/ReferenceMissionRoute.hpp>
#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/presentation/desktop/ScenePainter.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <crucible/simulation.hpp>
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace {
using namespace crucible;
void Require(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
/// Frozen study schedule; route coordinates advance once per admitted request.
struct Schedule { std::string_view name; std::uint64_t start, cadence; };
constexpr std::array Schedules{
    Schedule{"cadence30-start0", 0, 30}, Schedule{"cadence120-start0", 0, 120},
    Schedule{"cadence60-start60", 60, 60}};
constexpr std::uint64_t Deadline = 900;
std::optional<FieldEdit> Edit(Schedule schedule, std::uint64_t tick) {
    // Only active pre-deadline boundaries are study requests. This bound also
    // makes the canonical index*60 arithmetic finite without a general API.
    if (tick >= Deadline || tick < schedule.start || (tick - schedule.start) % schedule.cadence)
        return std::nullopt;
    return runtime::GetReferenceMissionRouteEdit(((tick - schedule.start) / schedule.cadence) * 60);
}
void ScheduleFixtures() {
    constexpr std::array<std::array<std::uint64_t, 3>, 3> requests{{
        {0, 30, 120}, {0, 120, 480}, {60, 120, 300}}};
    constexpr std::array<Position, 3> positions{{{8, 8}, {24, 8}, {8, 24}}};
    for (std::size_t i = 0; i < Schedules.size(); ++i) {
        for (std::size_t j = 0; j < positions.size(); ++j) {
            const auto edit = Edit(Schedules[i], requests[i][j]);
            Require(edit && edit->kind == FieldEditKind::set && edit->slot == 0 &&
                edit->center.x == positions[j].x && edit->center.y == positions[j].y &&
                edit->radius == 8 && edit->strength == 4, "independent schedule coordinate fixture");
            Require(!Edit(Schedules[i], requests[i][j] + 1), "non-request boundary absent");
        }
        Require(!Edit(Schedules[i], 900) && !Edit(Schedules[i], std::numeric_limits<std::uint64_t>::max()),
            "bounded study excludes deadline and overflow input");
    }
    Require(!Edit(Schedules[2], 0) && !Edit(Schedules[2], 59), "delayed start has no early request");
    constexpr std::array<std::array<std::uint64_t, 2>, 3> boundaries{{{1, 31}, {1, 121}, {61, 121}}};
    for (std::size_t i = 0; i < Schedules.size(); ++i) {
        runtime::InspectorSession run{4};
        for (std::uint64_t tick = 0; tick < boundaries[i][1]; ++tick) {
            const auto before = run.GetTrace().size();
            if (const auto edit = Edit(Schedules[i], tick)) {
                Require(run.TryAdmitFieldEdit(*edit).status == runtime::CommandIngress::AdmissionStatus::accepted,
                    "fixture request accepted");
                Require(run.GetTrace().size() == before, "request is not an applied boundary");
            }
            Require(run.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1,
                "fixture exact boundary");
        }
        const auto trace = run.GetTrace();
        Require(trace.size() == 2 && trace[0].tick == boundaries[i][0] && trace[1].tick == boundaries[i][1] &&
            trace[0].edit.center.x == 8 && trace[0].edit.center.y == 8 &&
            trace[1].edit.center.x == 24 && trace[1].edit.center.y == 8, "independent actual application fixture");
    }
}
void VerifyReplay(const runtime::InspectorSession& run) {
    Simulation oracle{2048, {{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}}};
    runtime::HeadlessSession replay{oracle, {64, 4096}};
    presentation::ScenarioSnapshot frame{2048, 4, 2048};
    const auto& actual = run.GetSnapshot();
    const auto a = *actual.GetInfo();
    Require(replay.TryReplay(run.GetTrace(), a.completed_tick).status == runtime::HeadlessSession::StepStatus::advanced &&
        frame.TryCapture(oracle), "strategy replay completed");
    const auto b = *frame.GetInfo();
    Require(a.completed_tick == b.completed_tick && a.biomass == b.biomass && a.samples == b.samples &&
        a.fields == b.fields && a.cells == b.cells && a.grid.columns == b.grid.columns &&
        a.grid.rows == b.grid.rows && a.grid.cell_size == b.grid.cell_size, "strategy metadata replay");
    Require(std::ranges::equal(actual.GetSamples(), frame.GetSamples(), [](const auto& x, const auto& y) {
        return x.id == y.id && x.position.x == y.position.x && x.position.y == y.position.y &&
            x.velocity.x == y.velocity.x && x.velocity.y == y.velocity.y;
    }), "strategy stable identity/kinematic replay");
    Require(std::ranges::equal(actual.GetFields(), frame.GetFields(), [](const auto& x, const auto& y) {
        return x.kind == y.kind && x.slot == y.slot && x.center.x == y.center.x && x.center.y == y.center.y &&
            x.radius == y.radius && x.strength == y.strength;
    }) && std::ranges::equal(actual.GetBlight(), frame.GetBlight()) &&
        std::ranges::equal(actual.GetStocks(), frame.GetStocks()), "strategy fields/infection/stock replay");
}
void Capture(const runtime::InspectorSession& run, const std::filesystem::path& file, std::string_view name) {
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{
        SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface};
    Require(surface != nullptr, "example surface");
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{
        SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer};
    Require(renderer != nullptr, "example renderer");
    presentation::desktop::ScenePainter painter{2048, 2048};
    presentation::Camera2D camera{{64, 32, 1}, {24, 96, 1232, 520}};
    presentation::desktop::SceneUi ui{};
    ui.mission = run.GetMission(); ui.message = name;
    Require(painter.TryDraw(*renderer, run.GetSnapshot(), camera, ui) && SDL_FlushRenderer(renderer.get()) &&
        SDL_SaveBMP(surface.get(), file.string().c_str()), "example painter capture");
}
void Run(Schedule schedule, std::ostream& out, const std::filesystem::path& directory) {
    runtime::InspectorSession run{2048, {64, 4096}, ReclamationMissionSettings{}};
    Require(run.GetMission()->settings.target_reclaimed == 1780 &&
        run.GetMission()->settings.deadline_ticks == Deadline, "frozen study mission settings");
    std::vector<std::uint64_t> requests;
    out << "{\"case\":\"" << schedule.name << "\",\"start\":" << schedule.start
        << ",\"cadence\":" << schedule.cadence << ",\"checkpoints\":[";
    bool first = true;
    while (true) {
        const auto mission = *run.GetMission();
        const auto ledger = *run.GetSnapshot().GetInfo()->biomass;
        Require(mission.completed_tick <= Deadline && ledger.initial_total ==
            ledger.remaining_stock + ledger.mobile_mass + ledger.reserve,
            "study bounded tick and conserved observation");
        if (mission.completed_tick % 60 == 0 || mission.outcome != ReclamationMissionOutcome::active) {
            if (!first) out << ',';
            first = false;
            out << "{\"tick\":" << mission.completed_tick << ",\"reclaimed\":" << mission.reclaimed
                << ",\"stock\":" << ledger.remaining_stock << ",\"mobile\":" << ledger.mobile_mass
                << ",\"reserve\":" << ledger.reserve << ",\"commands\":" << run.GetTrace().size() << '}';
        }
        if (!directory.empty() && mission.completed_tick == 60)
            Capture(run, directory / (std::string{schedule.name} + "-tick60.bmp"), schedule.name);
        if (mission.outcome != ReclamationMissionOutcome::active) break;
        if (const auto edit = Edit(schedule, mission.completed_tick)) {
            Require(requests.size() < 30, "bounded study request count");
            Require(run.TryAdmitFieldEdit(*edit).status == runtime::CommandIngress::AdmissionStatus::accepted,
                "study request admitted");
            requests.push_back(mission.completed_tick);
        }
        Require(run.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1,
            "study exact completed boundary");
    }
    VerifyReplay(run);
    const auto mission = *run.GetMission();
    Require(run.GetStatus() == runtime::ClockDriver::Status::closed && requests.size() == run.GetTrace().size(),
        "terminal closes with all requested edits applied");
    const auto trace_size = run.GetTrace().size();
    run.Resume();
    Require(run.TryPump(std::chrono::seconds{1}).advanced_ticks == 0 &&
        run.GetMission()->completed_tick == mission.completed_tick && run.GetTrace().size() == trace_size &&
        run.TryAdmitFieldEdit({FieldEditKind::remove, 0, {}, 0, 0}).status ==
            runtime::CommandIngress::AdmissionStatus::closed, "terminal cannot advance or admit");
    if (!directory.empty()) Capture(run, directory / (std::string{schedule.name} + "-terminal.bmp"), schedule.name);
    out << "],\"outcome\":\"" << (mission.outcome == ReclamationMissionOutcome::won ? "WON" : "LOST")
        << "\",\"tick\":" << mission.completed_tick << ",\"reclaimed\":" << mission.reclaimed
        << ",\"replay\":\"full-state\",\"trace\":[";
    first = true;
    for (std::size_t i = 0; i < run.GetTrace().size(); ++i) {
        const auto& command = run.GetTrace()[i];
        const auto& edit = command.edit;
        Require(command.tick == requests[i] + 1 && command.sequence == i + 1,
            "all request/application boundaries match");
        if (!first) out << ',';
        first = false;
        out << "{\"sequence\":" << command.sequence << ",\"request_tick\":" << requests[i]
            << ",\"applied_tick\":" << command.tick << ",\"kind\":\"set\",\"slot\":" << edit.slot
            << ",\"x\":" << edit.center.x << ",\"y\":" << edit.center.y
            << ",\"radius\":" << edit.radius << ",\"strength\":" << edit.strength << '}';
    }
    out << "]}";
}
}
int main(int argc, char** argv) {
    try {
        Require(argc >= 2, "usage: crucible_mission_sensitivity --schedule-only | --study [--export NEW_DIRECTORY]");
        const std::string_view mode{argv[1]};
        const bool schedule_only = mode == "--schedule-only" && argc == 2;
        const bool study = mode == "--study" && (argc == 2 ||
            (argc == 4 && std::string_view{argv[2]} == "--export" && std::string_view{argv[3]}.size() != 0));
        Require(schedule_only || study, "invalid sensitivity mode/arguments");
        ScheduleFixtures();
        if (schedule_only) {
            std::cout << "three frozen schedules and actual request/application boundaries passed\n";
            std::cout.flush(); Require(std::cout.good(), "schedule results persisted");
            return 0;
        }
        std::filesystem::path directory;
        if (argc == 4) {
            directory = argv[3];
            Require(std::filesystem::create_directory(directory), "export directory must be new");
        }
        Require(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"), "study SDL dummy selection");
        Require(SDL_Init(SDL_INIT_VIDEO), "study SDL startup");
        struct Quit { ~Quit() { SDL_Quit(); } } quit;
        std::ofstream file;
        if (!directory.empty()) { file.open(directory / "results.json"); Require(file.good(), "study results file"); }
        std::ostream& out = directory.empty() ? std::cout : file;
        out << std::setprecision(17) << "{\"target\":1780,\"deadline\":900,\"samples\":2048,\"case_count\":3,\"cases\":[";
        for (std::size_t i = 0; i < Schedules.size(); ++i) {
            if (i) out << ',';
            Run(Schedules[i], out, directory);
        }
        out << "]}\n";
        out.flush(); Require(out.good(), "study results persisted");
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
