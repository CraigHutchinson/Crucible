// Runs four fixed, live-tool strategies through the production mission coordinator.
// Compares each terminal snapshot to an independent trace replay and optionally
// exports actual painter frames plus raw checkpoints; no difficulty claim follows.
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
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace {
using namespace crucible;
enum class Strategy { passive, sweep, stationary, repel };
void Require(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
std::optional<FieldEdit> Edit(Strategy strategy, std::uint64_t tick) {
    if (strategy == Strategy::sweep) return runtime::GetReferenceMissionRouteEdit(tick);
    if (strategy == Strategy::stationary && tick == 0) return FieldEdit{FieldEditKind::set, 0, {32, 16}, 8, 4};
    if (strategy == Strategy::repel) {
        if (tick == 0) return FieldEdit{FieldEditKind::set, 0, {32, 16}, 8, -4};
        if (tick == 120) return FieldEdit{FieldEditKind::set, 0, {48, 16}, 8, -4};
        if (tick == 240) return FieldEdit{FieldEditKind::remove, 0, {}, 0, 0};
    }
    return std::nullopt;
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
            x.radius == y.radius && x.strength == y.strength &&
                x.end.x == y.end.x && x.end.y == y.end.y;
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
void Run(Strategy strategy, std::string_view name, std::ostream& out, const std::filesystem::path& directory) {
    runtime::InspectorSession run{2048, {64, 4096}, ReclamationMissionSettings{}};
    out << "{\"strategy\":\"" << name << "\",\"checkpoints\":[";
    bool first = true;
    while (true) {
        const auto mission = *run.GetMission();
        const auto ledger = *run.GetSnapshot().GetInfo()->biomass;
        Require(ledger.initial_total == ledger.remaining_stock + ledger.mobile_mass + ledger.reserve,
            "strategy conservation at every observation");
        if (mission.completed_tick % 60 == 0 || mission.outcome != ReclamationMissionOutcome::active) {
            if (!first) out << ',';
            first = false;
            out << "{\"tick\":" << mission.completed_tick << ",\"reclaimed\":" << mission.reclaimed
                << ",\"stock\":" << ledger.remaining_stock << ",\"mobile\":" << ledger.mobile_mass
                << ",\"commands\":" << run.GetTrace().size() << '}';
        }
        if (!directory.empty() && strategy == Strategy::sweep && mission.completed_tick == 60)
            Capture(run, directory / "sweep-active.bmp", "Actual shared sweep / applied trace / full-state replay at outcome");
        if (mission.outcome != ReclamationMissionOutcome::active) break;
        if (const auto edit = Edit(strategy, mission.completed_tick))
            Require(run.TryAdmitFieldEdit(*edit).status == runtime::CommandIngress::AdmissionStatus::accepted,
                "strategy input admitted before next boundary");
        Require(run.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1, "strategy one completed boundary");
    }
    VerifyReplay(run);
    const auto mission = *run.GetMission();
    Require(run.GetStatus() == runtime::ClockDriver::Status::closed, "strategy terminal closed admission");
    if (strategy == Strategy::passive) Require(mission.outcome == ReclamationMissionOutcome::lost &&
        mission.completed_tick == 900 && mission.reclaimed == 1774, "passive preserved receiving outcome");
    if (strategy == Strategy::sweep) Require(mission.outcome == ReclamationMissionOutcome::won &&
        mission.completed_tick == 267 && mission.reclaimed == 1780 && run.GetTrace().size() == 5,
        "shared sweep preserved receiving outcome");
    if (!directory.empty() && (strategy == Strategy::sweep || strategy == Strategy::passive))
        Capture(run, directory / (strategy == Strategy::sweep ? "sweep-won.bmp" : "passive-lost.bmp"), name);
    out << "],\"outcome\":\"" << (mission.outcome == ReclamationMissionOutcome::won ? "WON" : "LOST")
        << "\",\"tick\":" << mission.completed_tick << ",\"reclaimed\":" << mission.reclaimed
        << ",\"replay\":\"full-state\",\"trace\":[";
    first = true;
    for (const auto& command : run.GetTrace()) {
        if (!first) out << ',';
        first = false;
        const auto& edit = command.command.field;
        out << "{\"sequence\":" << command.sequence << ",\"applied_tick\":" << command.tick
            << ",\"kind\":\"" << (edit.kind == FieldEditKind::set ? "set" :
                    edit.kind == FieldEditKind::set_flow ? "set_flow" : "remove")
            << "\",\"slot\":" << edit.slot << ",\"x\":" << edit.center.x << ",\"y\":" << edit.center.y
            << ",\"radius\":" << edit.radius << ",\"strength\":" << edit.strength
            << ",\"end_x\":" << edit.end.x << ",\"end_y\":" << edit.end.y << '}';
    }
    out << "]}";
}
}
int main(int argc, char** argv) {
    try {
        std::filesystem::path directory;
        if (argc == 3 && std::string_view{argv[1]} == "--export") {
            directory = argv[2];
            Require(std::filesystem::create_directory(directory), "export directory must be new");
        } else Require(argc == 1, "usage: crucible_mission_examples [--export NEW_DIRECTORY]");
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        Require(SDL_Init(SDL_INIT_VIDEO), "example SDL startup");
        struct Quit { ~Quit() { SDL_Quit(); } } quit;
        std::ofstream file;
        if (!directory.empty()) { file.open(directory / "results.json"); Require(file.good(), "example results file"); }
        std::ostream& out = directory.empty() ? std::cout : file;
        out << std::setprecision(17) << "{\"target\":1780,\"deadline\":900,\"samples\":2048,\"strategies\":[";
        constexpr std::array strategies{Strategy::passive, Strategy::sweep, Strategy::stationary, Strategy::repel};
        constexpr std::array names{"passive", "sweep", "stationary", "repel-reposition-erase"};
        for (std::size_t i = 0; i < strategies.size(); ++i) {
            if (i) out << ',';
            Run(strategies[i], names[i], out, directory);
        }
        out << "]}\n";
        out.flush();
        Require(out.good(), "example results persisted");
    } catch (const std::exception& error) { std::cerr << error.what() << ": " << SDL_GetError() << '\n'; return 1; }
}
