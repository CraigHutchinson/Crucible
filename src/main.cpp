#include <crucible/simulation.hpp>

#include <array>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <locale>
#include <string_view>
#include <stdexcept>

#include <crucible/runtime/headless.hpp>
#include <crucible/runtime/HeadlessSession.hpp>
#include <crucible/runtime/ClockDriver.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>

namespace {
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
        << "\"/></clipPath></defs>\n<g transform=\"translate(24 52) scale("
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
        << "Cyan: " << info->samples << " nanite samples | red: Blight | ring: active radial field"
        << "</text>\n<text x=\"24\" y=\"513\" fill=\"#8199ae\" font-family=\"sans-serif\" font-size=\"12\">"
        << "Planar prototype / downwards +Y / no resource consumption, terrain or mission logic yet</text>\n</svg>\n";
    output.flush();
    if (!output) throw std::runtime_error("SVG write failed");
}

void RunScenario(const char* export_path) {
    using namespace crucible;
    using runtime::HeadlessSession;
    const Simulation::ScenarioOptions options{{64, 32, 1.0F}, 2, SteeringSettings{}};
    Simulation scenario{2048, options};
    HeadlessSession session{scenario, {8, 8}};
    runtime::ClockDriver clock{session};
    presentation::ScenarioSnapshot snapshot{2048, 2, 2048};
    const std::array paint{
        FieldEdit{FieldEditKind::set, 0, {24, 16}, 20, 4},
        FieldEdit{FieldEditKind::set, 1, {48, 8}, 8, -2}};
    if (session.GetIngress().TryAdmit(paint).status != runtime::CommandIngress::AdmissionStatus::accepted)
        throw std::runtime_error("Scenario paint admission failed");
    clock.Pause();
    if (clock.TryPump(std::chrono::milliseconds{100}).status != runtime::ClockDriver::Status::paused)
        throw std::runtime_error("Paused scenario advanced");
    clock.Resume();
    for (int tick = 0; tick < 20; ++tick) {
        if (tick == 10) {
            const std::array remove{FieldEdit{FieldEditKind::remove, 0}};
            if (session.GetIngress().TryAdmit(remove).status != runtime::CommandIngress::AdmissionStatus::accepted)
                throw std::runtime_error("Scenario removal admission failed");
        }
        if (clock.TryPump(std::chrono::nanoseconds{16'666'667}).advanced_ticks != 1)
            throw std::runtime_error("Scenario tick did not complete");
    }

    Simulation replayed{2048, options};
    HeadlessSession replay{replayed, {8, 8}};
    if (replay.TryReplay(session.GetTrace(), 20).status != HeadlessSession::StepStatus::advanced ||
        replayed.checksum() != scenario.checksum() ||
        replayed.GetBlightInfectedCount() != scenario.GetBlightInfectedCount() ||
        replayed.GetOccupiedCellCount() != scenario.GetOccupiedCellCount())
        throw std::runtime_error("Scenario replay diverged");
    const auto neighbors = scenario.TryCountNeighbors({32, 16}, 3);
    if (!neighbors) throw std::runtime_error("Scenario neighbor query failed");
    if (!snapshot.TryCapture(scenario)) throw std::runtime_error("Snapshot capture failed");
    if (export_path) ExportSvg(snapshot, export_path);
    const auto summary = clock.GetSummary();
    clock.Close();
    std::cout << "Crucible phase2 scenario: entities=2048 ticks=20 commands=" << session.GetTrace().size()
              << " occupied_cells=" << scenario.GetOccupiedCellCount()
              << " nearby=" << *neighbors << " blight=" << scenario.GetBlightInfectedCount()
              << " captured_samples=" << snapshot.GetSamples().size()
              << " completed_tick=" << summary.completed_tick
              << " discarded_ns=" << summary.discarded_scaled_nanoseconds / 60
              << " replay=checksum-and-counts\n";
}
}

int main(int argc, char** argv) {
    try {
        const char* export_path = nullptr;
        if (argc == 3 && std::string_view{argv[1]} == "--export-svg") export_path = argv[2];
        else if (argc != 1) throw std::invalid_argument("Usage: crucible [--export-svg path.svg]");
        crucible::Simulation simulation{150'000};
        crucible::runtime::run_ticks(simulation, 60);
        std::cout << "Crucible headless ECS foundation: 150000 entities, 60 ticks, checksum="
                  << simulation.checksum() << '\n';
        RunScenario(export_path);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
