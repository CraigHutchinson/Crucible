#include <crucible/simulation.hpp>

#include <array>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <locale>
#include <string_view>
#include <stdexcept>

#include <crucible/runtime/headless.hpp>
#include <crucible/runtime/HeadlessSession.hpp>
#include <crucible/runtime/ClockDriver.hpp>
#include <crucible/runtime/IntentDelivery.hpp>
#include <crucible/runtime/BoundaryPipeline.hpp>
#include <crucible/runtime/InspectorSession.hpp>
#include <crucible/runtime/ReferenceMissionRoute.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>

#include "crucible/application/game_journey.hpp"

namespace {
enum class MissionInput { none, sweeping_attractor };
void runJourney(MissionInput input, const char* profilePath = nullptr)
{
    using namespace crucible;
    using namespace application;
    GameJourney journey{profilePath ? std::optional<std::filesystem::path>{profilePath} : std::nullopt};
    auto navigate = [&](NavigationAction action) {
        const auto view = journey.getFrontendView();
        if (!journey.tryApplyIntent({view.screen_, view.transitionSerial_, action}))
            throw std::runtime_error("Journey navigation was rejected");
    };
    if (journey.tryPump(std::chrono::seconds{1}) || journey.getSnapshot())
        throw std::runtime_error("Journey advanced before play");
    navigate(NavigationAction::skipIntro);
    navigate(NavigationAction::newRun);
    navigate(NavigationAction::beginMission);
    navigate(NavigationAction::pause);
    navigate(NavigationAction::showOptions);
    if (journey.tryPump(std::chrono::hours{1}))
        throw std::runtime_error("Journey advanced behind its options screen");
    navigate(NavigationAction::back);
    navigate(NavigationAction::resume);
    while (journey.getFrontendView().screen_ == FrontendScreen::playing) {
        const auto view = journey.getFrontendView();
        if (!view.progress_) throw std::runtime_error("Journey lost its mission progress");
        if (input == MissionInput::sweeping_attractor) {
            if (const auto edit = runtime::GetReferenceMissionRouteEdit(view.progress_->completed_tick)) {
                const auto admission = journey.tryAdmitFieldEdit(*edit);
                if (!admission || admission->status != runtime::CommandIngress::AdmissionStatus::accepted)
                    throw std::runtime_error("Journey reference route admission failed");
            }
        }
        const auto pump = journey.tryPump(std::chrono::nanoseconds{16'666'667});
        if (!pump || pump->advanced_ticks != 1)
            throw std::runtime_error("Journey mission boundary failed");
    }
    const auto completed = journey.getFrontendView();
    if (completed.screen_ != FrontendScreen::results || !completed.progress_ ||
        journey.tryPump(std::chrono::hours{1}))
        throw std::runtime_error("Journey terminal result was not latched");
    const auto previousAttempt = journey.getAttemptId();
    navigate(NavigationAction::retryMission);
    if (journey.getAttemptId() != previousAttempt + 1 ||
        journey.getFrontendView().progress_->completed_tick != 0)
        throw std::runtime_error("Journey retry did not replace the attempt");
    navigate(NavigationAction::requestQuit);
    navigate(NavigationAction::confirmQuit);
    if (!journey.isQuitRequested()) throw std::runtime_error("Journey exit was not confirmed");
    if (profilePath)
    {
        GameJourney reloaded{std::filesystem::path{profilePath}};
        if (!reloaded.getFrontendView().canContinue_ || reloaded.getSnapshot())
            throw std::runtime_error("Journey continuation did not survive relaunch");
    }
    std::cout << "Crucible headless journey: intro/menu/briefing/play/pause/options/result/retry/quit"
              << " outcome=" << (completed.progress_->outcome == ReclamationMissionOutcome::won ? "WON" : "LOST")
              << " tick=" << completed.progress_->completed_tick
              << " attempts=" << journey.getAttemptId()
              << " persistent_progress=" << (profilePath ? "received" : "in-memory")
              << " native_visuals=not-yet-received\n";
}

void RunMission(MissionInput input) {
    using namespace crucible;
    runtime::InspectorSession run{2048, {64, 4096}, ReclamationMissionSettings{}};
    while (run.GetMission()->outcome == ReclamationMissionOutcome::active) {
        const auto tick = run.GetMission()->completed_tick;
        if (input == MissionInput::sweeping_attractor) {
            if (const auto edit = runtime::GetReferenceMissionRouteEdit(tick))
                if (run.TryAdmitFieldEdit(*edit).status != runtime::CommandIngress::AdmissionStatus::accepted)
                    throw std::runtime_error("Reference route admission failed");
        }
        if (run.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks != 1)
            throw std::runtime_error("Reference challenge boundary failed");
    }
    const auto mission = *run.GetMission();
    const auto ledger = *run.GetSnapshot().GetInfo()->biomass;
    if (run.GetStatus() != runtime::ClockDriver::Status::closed ||
        ledger.initial_total != ledger.remaining_stock + ledger.mobile_mass + ledger.reserve)
        throw std::runtime_error("Reference challenge terminal conservation failed");
    Simulation oracle{2048, {{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}}};
    runtime::HeadlessSession replay{oracle, {64, 4096}};
    presentation::ScenarioSnapshot expected{2048, 4, 2048};
    if (replay.TryReplay(run.GetTrace(), mission.completed_tick).status != runtime::HeadlessSession::StepStatus::advanced ||
        !expected.TryCapture(oracle) || !run.GetSnapshot().HasEqualState(expected))
        throw std::runtime_error("Reference challenge full-state replay failed");
    std::cout << "Crucible reference challenge ("
        << (input == MissionInput::none ? "no field input" : "swept attractor") << "): "
        << (mission.outcome == ReclamationMissionOutcome::won ? "WON" : "LOST")
        << " tick=" << mission.completed_tick << " recovered=" << mission.reclaimed
        << " target=" << mission.settings.target_reclaimed << " deadline=" << mission.settings.deadline_ticks
        << " commands=" << run.GetTrace().size() << " conserved=1 replay=full-state\n";
}

void RunStructuralMission() {
    using namespace crucible;
    runtime::InspectorSession run{2048, {64, 4096}, ReclamationMissionSettings{}, true};
    bool gathering{}, fused{};
    while (run.GetMission()->outcome == ReclamationMissionOutcome::active) {
        const auto tick = run.GetMission()->completed_tick;
        if (!gathering && run.GetMission()->reclaimed < 1780) {
            if (const auto edit = runtime::GetReferenceMissionRouteEdit(tick))
                if (run.TryAdmitFieldEdit(*edit).status != runtime::CommandIngress::AdmissionStatus::accepted)
                    throw std::runtime_error("Structural harvest route admission failed");
        } else if (!gathering) {
            const FieldEdit gather{FieldEditKind::set, 0, {48.5F,16.5F}, 8, 4};
            if (run.TryAdmitFieldEdit(gather).status != runtime::CommandIngress::AdmissionStatus::accepted)
                throw std::runtime_error("Structural gathering admission failed");
            gathering = true;
        }
        const auto structure = run.GetSnapshot().GetInfo()->structural;
        if (gathering && !fused && structure->eligible_mobile >= StructuralSettings::cost) {
            if (run.TryFuseRelay().status != runtime::CommandIngress::AdmissionStatus::accepted)
                throw std::runtime_error("Structural fuse admission failed");
            fused = true;
        }
        if (run.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks != 1)
            throw std::runtime_error("Structural mission boundary failed");
    }
    const auto mission = *run.GetMission();
    const auto info = *run.GetSnapshot().GetInfo();
    const auto ledger = *info.biomass;
    if (run.GetStatus() != runtime::ClockDriver::Status::closed ||
        ledger.initial_total != ledger.remaining_stock + ledger.mobile_mass + ledger.reserve +
            ledger.structure_mass + ledger.lost_mass)
        throw std::runtime_error("Structural terminal conservation failed");
    Simulation oracle{2048, {{64,32,1},4,SteeringSettings{},ResourceSettings{},StructuralSettings{}}};
    runtime::HeadlessSession replay{oracle,{64,4096}};
    presentation::ScenarioSnapshot expected{2048,4,2048};
    if (replay.TryReplay(run.GetTrace(),mission.completed_tick).status != runtime::HeadlessSession::StepStatus::advanced ||
        !expected.TryCapture(oracle) || !run.GetSnapshot().HasEqualState(expected))
        throw std::runtime_error("Structural full-state replay failed");
    std::cout << "Crucible structural challenge: "
        << (mission.outcome == ReclamationMissionOutcome::won ? "WON" : "LOST")
        << " tick=" << mission.completed_tick << " harvested=" << mission.reclaimed
        << " hold=" << info.structural->hold_ticks << " mobile=" << ledger.mobile_mass
        << " structure=" << ledger.structure_mass << " lost=" << ledger.lost_mass
        << " commands=" << run.GetTrace().size() << " conserved=1 replay=full-state\n";
}

void ExportSvg(const crucible::presentation::ScenarioSnapshot& snapshot, const char* path) {
    const auto info = snapshot.GetInfo();
    if (!info) throw std::runtime_error("Cannot export an uncaptured snapshot");
    std::ofstream output{path, std::ios::binary};
    if (!output) throw std::runtime_error("Cannot open SVG destination");
    output.imbue(std::locale::classic());
    output << std::setprecision(17);
    const double width = static_cast<double>(info->grid.columns) * info->grid.cell_size;
    const double height = static_cast<double>(info->grid.rows) * info->grid.cell_size;
    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 880 520\">\n"
        << "<rect width=\"880\" height=\"520\" fill=\"#0b1320\"/>\n"
        << "<text x=\"24\" y=\"30\" fill=\"#edf6ff\" font-family=\"sans-serif\" font-size=\"20\">"
        << "CRUCIBLE / actual simulation state / tick " << info->completed_tick << "</text>\n"
        << "<defs><clipPath id=\"world\"><rect width=\"" << width << "\" height=\"" << height
        << "\"/></clipPath><marker id=\"flow-arrow\" viewBox=\"0 0 10 10\" refX=\"10\" refY=\"5\" markerWidth=\"6\" markerHeight=\"6\" orient=\"auto\"><path d=\"M 0 0 L 10 5 L 0 10 z\" fill=\"#b49aff\"/></marker></defs>\n<g transform=\"translate(24 52) scale("
        << 832.0 / width << ' ' << 416.0 / height << ")\" clip-path=\"url(#world)\">\n"
        << "<rect width=\"" << width << "\" height=\"" << height << "\" fill=\"#162637\"/>\n";
    for (std::size_t i = 0; i < info->cells; ++i) {
        if (!snapshot.GetBlight()[i]) continue;
        output << "<rect x=\"" << (i % info->grid.columns) * static_cast<double>(info->grid.cell_size)
            << "\" y=\"" << (i / info->grid.columns) * static_cast<double>(info->grid.cell_size)
            << "\" width=\"" << info->grid.cell_size << "\" height=\"" << info->grid.cell_size
            << "\" fill=\"#994552\"/>\n";
    }
    for (const auto& field : snapshot.GetFields()) {
        if (field.kind == crucible::FieldEditKind::set_flow) {
            if (field.radius == 0 || field.strength == 0) continue;
            output << "<line x1=\"" << field.center.x << "\" y1=\"" << field.center.y
                << "\" x2=\"" << field.end.x << "\" y2=\"" << field.end.y
                << "\" stroke=\"#b49aff\" stroke-opacity=\"0.2\" stroke-linecap=\"round\" stroke-width=\""
                << static_cast<double>(field.radius) * 2 << "\"/>\n"
                << "<line x1=\"" << field.center.x << "\" y1=\"" << field.center.y
                << "\" x2=\"" << field.end.x << "\" y2=\"" << field.end.y
                << "\" stroke=\"#b49aff\" stroke-width=\"0.14\" marker-end=\"url(#flow-arrow)\"/>\n";
            continue;
        }
        if (field.kind != crucible::FieldEditKind::set) continue;
        output << "<circle cx=\"" << field.center.x << "\" cy=\"" << field.center.y
            << "\" r=\"" << field.radius << "\" fill=\"none\" stroke=\""
            << (field.strength >= 0 ? "#73e3ad" : "#ffc76b")
            << "\" stroke-width=\"0.14\"/>\n";
    }
    for (const auto& sample : snapshot.GetSamples()) {
        output << "<circle cx=\"" << sample.position.x << "\" cy=\"" << sample.position.y
            << "\" r=\"" << info->grid.cell_size * 0.10 << "\" fill=\"#77ddff\"/>\n";
    }
    output << "</g>\n<text x=\"24\" y=\"493\" fill=\"#c6d9ea\" font-family=\"sans-serif\" font-size=\"15\">"
        << "Cyan: " << info->samples << " nanite samples | red: Blight | ring: radial field | arrow: straight flow"
        << "</text>\n<text x=\"24\" y=\"513\" fill=\"#8199ae\" font-family=\"sans-serif\" font-size=\"12\">"
        << "Planar prototype / downwards +Y / ";
    if (info->biomass) {
        const auto ledger = *info->biomass;
        output << "stock " << ledger.remaining_stock << " + mobile " << ledger.mobile_mass
            << " + reserve " << ledger.reserve << " = initial " << ledger.initial_total;
    } else {
        output << "no resource consumption, terrain or mission logic yet";
    }
    output << "</text>\n</svg>\n";
    output.flush();
    if (!output) throw std::runtime_error("SVG write failed");
}

void RunScenario(const char* export_path, std::optional<crucible::ResourceSettings> resources) {
    using namespace crucible;
    using runtime::HeadlessSession;
    const Simulation::ScenarioOptions options{{64, 32, 1.0F}, 2, SteeringSettings{}, resources};
    Simulation scenario{2048, options};
    HeadlessSession session{scenario, {8, 8}};
    runtime::IntentDelivery delivery{session.GetIngress(), 1};
    runtime::BoundaryPipeline graph{session, scenario, 2048, 2, 2048};
    runtime::ClockDriver clock{session, {}, [&] { return graph.TryStep(); }};
    const auto initial_ledger = graph.GetFrame().GetInfo()->biomass;
    const std::array paint{
        BoundaryCommand{FieldEdit{FieldEditKind::set, 0, {24, 16}, 20, 4}},
        BoundaryCommand{FieldEdit{FieldEditKind::set, 1, {48, 8}, 8, -2}}};
    if (delivery.TryAdmitCommands(paint).admission.status != runtime::CommandIngress::AdmissionStatus::accepted)
        throw std::runtime_error("Scenario paint admission failed");
    clock.Pause();
    if (clock.TryPump(std::chrono::milliseconds{100}).status != runtime::ClockDriver::Status::paused)
        throw std::runtime_error("Paused scenario advanced");
    clock.Resume();
    for (int tick = 0; tick < 20; ++tick) {
        if (tick == 10) {
            const std::array remove{BoundaryCommand{FieldEdit{FieldEditKind::remove, 0}}};
            if (delivery.TryAdmitCommands(remove).admission.status != runtime::CommandIngress::AdmissionStatus::accepted)
                throw std::runtime_error("Scenario removal admission failed");
        }
        if (clock.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks != 1)
            throw std::runtime_error("Scenario tick did not complete");
    }

    Simulation replayed{2048, options};
    HeadlessSession replay{replayed, {8, 8}};
    presentation::ScenarioSnapshot replay_snapshot{2048, 2, 2048};
    const auto& snapshot = graph.GetFrame();
    if (replay.TryReplay(session.GetTrace(), 20).status != HeadlessSession::StepStatus::advanced ||
        !replay_snapshot.TryCapture(replayed) ||
        !snapshot.HasEqualState(replay_snapshot))
        throw std::runtime_error("Scenario replay diverged");
    const auto neighbors = scenario.TryCountNeighbors({32, 16}, 3);
    if (!neighbors) throw std::runtime_error("Scenario neighbor query failed");
    if (export_path) ExportSvg(snapshot, export_path);
    const auto summary = clock.GetSummary();
    clock.Close();
    std::cout << "Crucible " << (resources ? "reclamation" : "phase2")
              << " scenario: entities=2048 ticks=20 commands=" << session.GetTrace().size()
              << " occupied_cells=" << scenario.GetOccupiedCellCount()
              << " nearby=" << *neighbors << " blight=" << scenario.GetBlightInfectedCount()
              << " captured_samples=" << snapshot.GetSamples().size()
              << " completed_tick=" << summary.completed_tick
              << " discarded_ns=" << summary.discarded_scaled_nanoseconds / 60
              << " replay=full-state\n";
    if (const auto ledger = snapshot.GetInfo()->biomass) {
        std::cout << "Biomass: initial=" << initial_ledger->initial_total << " stock=" << ledger->remaining_stock
                  << " mobile=" << ledger->mobile_mass << " reserve=" << ledger->reserve
                  << " harvested=" << ledger->harvested << " work_actions=" << ledger->work_actions
                  << " conserved=" << (ledger->initial_total == ledger->remaining_stock + ledger->mobile_mass + ledger->reserve)
                  << '\n';
    }
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 5 && std::string_view{argv[1]} == "--journey" &&
            std::string_view{argv[2]} == "--route" && std::string_view{argv[3]} == "--profile") {
            runJourney(MissionInput::sweeping_attractor, argv[4]);
            return 0;
        }
        if ((argc == 2 || (argc == 3 && std::string_view{argv[2]} == "--route")) &&
                std::string_view{argv[1]} == "--journey") {
            runJourney(argc == 2 ? MissionInput::none : MissionInput::sweeping_attractor);
            return 0;
        }
        if (argc == 3 && std::string_view{argv[1]} == "--structural" && std::string_view{argv[2]} == "--route") {
            RunStructuralMission();
            return 0;
        }
        if ((argc == 2 || (argc == 3 && std::string_view{argv[2]} == "--route")) &&
                std::string_view{argv[1]} == "--mission") {
            RunMission(argc == 2 ? MissionInput::none : MissionInput::sweeping_attractor);
            return 0;
        }
        const char* export_path = nullptr;
        std::optional<crucible::ResourceSettings> resources;
        int first_option = 1;
        if (argc > 1 && std::string_view{argv[1]} == "--reclamation") {
            resources = crucible::ResourceSettings{};
            first_option = 2;
        }
        if (argc == first_option + 2 && std::string_view{argv[first_option]} == "--export-svg")
            export_path = argv[first_option + 1];
        else if (argc != first_option)
            throw std::invalid_argument("Usage: crucible --journey [--route [--profile path]] | --mission [--route] | [--reclamation] [--export-svg path.svg]");
        crucible::Simulation simulation{150'000};
        crucible::runtime::run_ticks(simulation, 60);
        std::cout << "Crucible headless ECS foundation: 150000 entities, 60 ticks, checksum="
                  << simulation.checksum() << '\n';
        RunScenario(export_path, resources);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
