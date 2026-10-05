// Receives gather/fuse/shatter/redirect through the production InspectorSession,
// painter and GPU packet. Optional exports are actual software frames. Checks
// fixed identities, retained snapshots and full replay; no human-playtest claim.
#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <crucible/presentation/Camera2D.hpp>
#include <crucible/presentation/desktop/ScenePainter.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <crucible/presentation/gpu/InstancePacket.hpp>
#include <crucible/simulation.hpp>
#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace crucible;
void Require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
void Step(runtime::InspectorSession& run) {
    Require(run.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks == 1, "one completed structural boundary");
}
void Capture(const runtime::InspectorSession& run, const std::filesystem::path& directory, const char* name) {
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface{
        SDL_CreateSurface(1280, 720, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface};
    Require(surface != nullptr, "structural software surface");
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{
        SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer};
    Require(renderer != nullptr, "structural software renderer");
    presentation::desktop::ScenePainter painter{2048, 2048};
    presentation::Camera2D camera{{64, 32, 1}, {24, 96, 1232, 520}};
    presentation::desktop::SceneUi ui{}; ui.mission = run.GetMission(); ui.message = name;
    Require(painter.TryDraw(*renderer, run.GetSnapshot(), camera, ui) && SDL_FlushRenderer(renderer.get()), "actual structural painter frame");
    if (!directory.empty()) Require(SDL_SaveBMP(surface.get(), (directory / (std::string{name} + ".bmp")).string().c_str()), "actual structural BMP");
    const auto info = *run.GetSnapshot().GetInfo();
    const auto mass = *info.biomass;
    Require(mass.initial_total == mass.remaining_stock + mass.mobile_mass + mass.reserve + mass.structure_mass + mass.lost_mass,
        "structural conserved frame");
    std::cout << "{\"frame\":\"" << name << "\",\"tick\":" << info.completed_tick
        << ",\"mobile\":" << mass.mobile_mass << ",\"structure\":" << mass.structure_mass
        << ",\"lost\":" << mass.lost_mass << ",\"stock\":" << mass.remaining_stock << ",\"reserve\":" << mass.reserve
        << ",\"hold\":" << info.structural->hold_ticks << ",\"eligible\":" << info.structural->eligible_mobile
        << ",\"generation\":" << info.structural->generation << ",\"commands\":" << run.GetTrace().size() << "}\n";
}
void Run(const std::filesystem::path& directory) {
    if (!directory.empty()) std::filesystem::create_directories(directory);
    runtime::InspectorSession run{2048, {64, 4096}, ReclamationMissionSettings{}, true};
    Require(run.TryAdmitFieldEdit({FieldEditKind::set, 0, {48.5F, 16.5F}, 8, 4}).status ==
        runtime::CommandIngress::AdmissionStatus::accepted, "gather admitted");
    for (int i = 0; i < 60; ++i) Step(run);
    Require(run.GetSnapshot().GetInfo()->structural->eligible_mobile >= 64, "actual radial gather unlocks cost64");
    Capture(run, directory, "gather-tick60");
    Require(run.TryFuseRelay().status == runtime::CommandIngress::AdmissionStatus::accepted, "fuse admitted");
    Step(run);
    Require(run.GetLastStructuralResult() == StructuralCommandResult::applied, "fuse applied");
    const auto fused_info = *run.GetSnapshot().GetInfo();
    Require(fused_info.biomass->mobile_mass == 1984 && fused_info.biomass->structure_mass == 64 &&
        fused_info.structural->occupied && fused_info.structural->hold_ticks == 1, "actual anchored mobile tradeoff");
    Require(std::ranges::count(run.GetSnapshot().GetSamples(), SampleActivity::anchored, &SampleState::activity) == 64,
        "owned fixed identity membership");
    presentation::gpu::InstancePacket packet{2048, 2048};
    Require(packet.TryCapture(run.GetSnapshot()) && packet.GetMarkers().size() == 1984, "GPU omits anchored particles");
    Capture(run, directory, "fuse-tick61");
    Simulation replay_simulation{2048, {{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}, StructuralSettings{}}};
    runtime::HeadlessSession replay{replay_simulation, {64, 4096}};
    presentation::ScenarioSnapshot retained{2048, 4, 2048};
    Require(replay.TryReplay(run.GetTrace(), 61).status == runtime::HeadlessSession::StepStatus::advanced &&
        retained.TryCapture(replay_simulation) && retained.HasEqualState(run.GetSnapshot()), "complete fused snapshot replay");
    Require(run.TryShatterRelay().status == runtime::CommandIngress::AdmissionStatus::accepted, "shatter admitted");
    Step(run);
    Require(run.GetLastStructuralResult() == StructuralCommandResult::applied, "shatter applied");
    const auto shattered = *run.GetSnapshot().GetInfo();
    Require(shattered.biomass->mobile_mass == 2032 && shattered.biomass->structure_mass == 0 && shattered.biomass->lost_mass == 16 &&
        !shattered.structural->occupied && shattered.structural->hold_ticks == 0, "actual48 refund and16 irreversible loss");
    Require(retained.GetInfo()->structural->occupied && retained.GetInfo()->biomass->structure_mass == 64 &&
        !retained.HasEqualState(run.GetSnapshot()), "retained fuse frame survives later shatter");
    Require(packet.TryCapture(run.GetSnapshot()) && packet.GetMarkers().size() == 2032, "GPU omits lost identities");
    Capture(run, directory, "shatter-tick62");
    Require(run.TryAdmitFieldEdit({FieldEditKind::set_flow, 0, {48.5F, 16.5F}, 8, 4, {32, 16.5F}}).status ==
        runtime::CommandIngress::AdmissionStatus::accepted, "redirect admitted");
    Step(run); Capture(run, directory, "redirect-tick63");
    Simulation final_simulation{2048, {{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}, StructuralSettings{}}};
    runtime::HeadlessSession final_replay{final_simulation, {64, 4096}};
    presentation::ScenarioSnapshot final_frame{2048, 4, 2048};
    Require(final_replay.TryReplay(run.GetTrace(), 63).status == runtime::HeadlessSession::StepStatus::advanced &&
        final_frame.TryCapture(final_simulation) && final_frame.HasEqualState(run.GetSnapshot()), "complete redirect structural replay");
    run.Restart();
    Require(!run.GetSnapshot().GetInfo()->structural->occupied && run.GetSnapshot().GetInfo()->biomass->lost_mass == 0 &&
        retained.GetInfo()->structural->occupied, "restart replaces world without altering owned retained frame");
}
}
int main(int argc, char** argv) {
    try { Run(argc == 2 ? std::filesystem::path{argv[1]} : std::filesystem::path{}); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
