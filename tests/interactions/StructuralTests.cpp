#include <crucible/simulation.hpp>
#include <crucible/blight/Grid.hpp>
#include <crucible/interactions/Reclamation.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using namespace crucible;
#define CHECK(expression) do { if (!(expression)) { std::cerr << #expression << '\n'; return false; } } while (false)

struct Frame {
    std::vector<SampleState> samples;
    std::vector<FieldEdit> fields;
    std::vector<std::uint8_t> infection;
    std::vector<std::uint64_t> stocks;
    ScenarioStateInfo info;
};
Frame Capture(Simulation& simulation, std::size_t count, std::size_t cells) {
    Frame frame{std::vector<SampleState>(count), std::vector<FieldEdit>(4),
                std::vector<std::uint8_t>(cells), std::vector<std::uint64_t>(cells), {}};
    const auto info = simulation.TryCopyState({frame.samples, frame.fields, frame.infection, frame.stocks});
    if (!info) throw std::runtime_error("structural state copy failed");
    frame.info = *info;
    return frame;
}
bool SameRuleState(const Frame& a, const Frame& b) {
    const auto x = a.info.structural, y = b.info.structural;
    return a.info.completed_tick == b.info.completed_tick && a.info.biomass == b.info.biomass &&
        x && y && x->occupied == y->occupied && x->generation == y->generation &&
        x->hold_ticks == y->hold_ticks && x->eligible_mobile == y->eligible_mobile && x->members == y->members &&
        a.stocks == b.stocks && a.infection == b.infection &&
        std::ranges::equal(a.samples, b.samples, [](const auto& left, const auto& right) {
            return left.id == right.id && left.position.x == right.position.x && left.position.y == right.position.y &&
                left.velocity.x == right.velocity.x && left.velocity.y == right.velocity.y && left.activity == right.activity;
        });
}
bool Conserved(BiomassLedger ledger) {
    return ledger.initial_total == ledger.remaining_stock + ledger.mobile_mass + ledger.reserve +
        ledger.structure_mass + ledger.lost_mass;
}

bool LedgerAndSparseOracle() {
    const GridConfig config{16, 1, 1};
    blight::Grid infection{config};
    interactions::Reclamation reclaim{config, 80, {4, 1, 0, 3}};
    CHECK((reclaim.GetLedger() == BiomassLedger{144, 64, 80, 0, 0, 0, 0, 0}));
    CHECK(reclaim.TryAnchor(64));
    CHECK((reclaim.GetLedger() == BiomassLedger{144, 64, 16, 0, 0, 0, 64, 0}));
    auto anchored = reclaim.GetLedger();
    CHECK(!reclaim.TryAnchor(17) && !reclaim.TryRelease(64, 65));
    CHECK(reclaim.GetLedger() == anchored);
    CHECK(reclaim.TryRelease(64, 48));
    CHECK((reclaim.GetLedger() == BiomassLedger{144, 64, 64, 0, 0, 0, 0, 16}));
    std::array<SampleState, 64> mobile{};
    for (std::size_t i = 0; i < mobile.size(); ++i)
        mobile[i] = {{i < 48 ? i + 1 : i + 17}, {.5F, .5F}, {}};
    CHECK(infection.TrySeed(0, 0));
    CHECK(reclaim.TryStep(infection, mobile));
    CHECK((reclaim.GetLedger() == BiomassLedger{144, 61, 64, 3, 3, 3, 0, 16}));
    CHECK(Conserved(reclaim.GetLedger()));
    const auto ledger = reclaim.GetLedger();
    mobile[0].activity = SampleActivity::anchored;
    CHECK(!reclaim.TryStep(infection, mobile) && reclaim.GetLedger() == ledger);
    mobile[0].activity = SampleActivity::mobile;
    mobile[0].id = mobile[63].id;
    CHECK(!reclaim.TryStep(infection, mobile) && reclaim.GetLedger() == ledger);
    CHECK(reclaim.TryAnchor(64));
    CHECK(reclaim.TryStep(infection, {}));
    CHECK(reclaim.GetLedger().mobile_mass == 0 && reclaim.GetLedger().structure_mass == 64);
    return true;
}

bool MembershipAndRefusalOracle() {
    // Centers .5..79.5: center31.5/radius32 includes IDs1..64 exactly.
    StructuralSettings settings{{31.5F, .5F}, 32, 2, 120};
    Simulation::ScenarioOptions options{{80, 1, 1}, 4, {}, ResourceSettings{4, 1, 0, 0}, settings};
    Simulation simulation{80, options};
    CHECK(simulation.GetStructuralState()->eligible_mobile == 64);
    CHECK(simulation.TryShatterRelay(0) == StructuralCommandResult::empty);
    CHECK(simulation.TryFuseRelay() == StructuralCommandResult::applied);
    auto frame = Capture(simulation, 80, 80);
    CHECK(frame.info.structural->generation == 1 && frame.info.structural->hold_ticks == 0);
    CHECK(frame.info.biomass->mobile_mass == 16 && frame.info.biomass->structure_mass == 64);
    for (std::size_t i = 0; i < 64; ++i) {
        CHECK(frame.info.structural->members[i].value == i + 1);
        CHECK(frame.samples[i].activity == SampleActivity::anchored);
        CHECK(frame.samples[i].position.x == 31.5F && frame.samples[i].position.y == .5F);
        CHECK(frame.samples[i].velocity.x == 0 && frame.samples[i].velocity.y == 0);
    }
    CHECK(simulation.TryCountNeighbors(settings.relay_center, 0) == 0);
    CHECK(simulation.TryFuseRelay() == StructuralCommandResult::occupied);
    CHECK(simulation.TryShatterRelay(0) == StructuralCommandResult::stale_generation);
    CHECK(SameRuleState(frame, Capture(simulation, 80, 80)));
    simulation.tick();
    CHECK(simulation.GetStructuralState()->hold_ticks == 1);
    CHECK(simulation.TryShatterRelay(1) == StructuralCommandResult::applied);
    auto released = Capture(simulation, 80, 80);
    CHECK(released.info.biomass->mobile_mass == 64 && released.info.biomass->lost_mass == 16);
    CHECK(released.info.biomass->structure_mass == 0 && Conserved(*released.info.biomass));
    CHECK(simulation.GetStructuralState()->generation == 1 && simulation.GetStructuralState()->hold_ticks == 0);
    CHECK(simulation.TryCountNeighbors(settings.relay_center, 0) == 48);
    for (std::size_t i = 0; i < 64; ++i)
        CHECK(released.samples[i].activity == (i < 48 ? SampleActivity::mobile : SampleActivity::lost));
    CHECK(std::ranges::all_of(simulation.GetStructuralState()->members, [](SampleId id) { return id.value == 0; }));
    CHECK(simulation.TryFuseRelay() == StructuralCommandResult::insufficient_mass);
    CHECK(SameRuleState(released, Capture(simulation, 80, 80)));
    // The retained owned frame still describes anchored identities after shatter.
    CHECK(frame.samples[0].activity == SampleActivity::anchored && frame.info.structural->occupied);
    options.structural->eligibility_radius = std::nextafter(32.0F, 0.0F);
    Simulation below{80, options};
    CHECK(below.GetStructuralState()->eligible_mobile == 63);
    CHECK(below.TryFuseRelay() == StructuralCommandResult::insufficient_mass);
    return true;
}

bool EmptyMobileAndProtection() {
    StructuralSettings settings{{31.5F, .5F}, 32, 2, 120};
    Simulation simulation{64, {{64, 1, 1}, 4, SteeringSettings{}, ResourceSettings{4, 1, 0, 0}, settings}};
    CHECK(simulation.TryFuseRelay() == StructuralCommandResult::applied);
    for (int tick = 0; tick < 120; ++tick) simulation.tick();
    CHECK(simulation.GetOccupiedCellCount() == 0);
    CHECK(simulation.TryCountNeighbors(settings.relay_center, 100) == 0);
    CHECK(simulation.GetStructuralState()->hold_ticks == 120);
    const auto frame = Capture(simulation, 64, 64);
    CHECK(frame.info.biomass->reserve == 0 && frame.info.biomass->remaining_stock == 256);
    for (const auto& sample : frame.samples)
        CHECK(sample.activity == SampleActivity::anchored && sample.position.x == 31.5F && sample.velocity.x == 0);
    CHECK(simulation.TryShatterRelay(1) == StructuralCommandResult::applied);
    simulation.tick();
    CHECK(simulation.GetStructuralState()->hold_ticks == 0);
    CHECK(simulation.GetBiomassLedger()->mobile_mass == 48 && simulation.GetBiomassLedger()->lost_mass == 16);

    // Protection uses inclusive cell-center distance; clearing makes no harvest.
    const GridConfig config{5, 1, 1};
    blight::Grid infection{config};
    interactions::Reclamation reclaim{config, 0, {4, 1, 0, 0}};
    for (std::size_t cell = 0; cell < 5; ++cell) CHECK(infection.TrySeed(cell, 0));
    const auto before = reclaim.GetLedger();
    CHECK(reclaim.TryStep(infection, {}, interactions::Reclamation::ProtectedArea{{2.5F, .5F}, 1}));
    CHECK(infection.IsInfected(0, 0) && infection.IsInfected(4, 0));
    for (std::size_t cell = 1; cell <= 3; ++cell) CHECK(!infection.IsInfected(cell, 0));
    CHECK(reclaim.GetLedger() == before);
    CHECK(reclaim.TryStep(infection, {}));
    CHECK(infection.IsInfected(1, 0) && infection.IsInfected(3, 0) && !infection.IsInfected(2, 0));

    // A just-protected old source emits one final wave outside its zero-radius sanctuary.
    blight::Grid wave{config};
    interactions::Reclamation sterilize{config, 0, {4, 1, 0, 0}};
    CHECK(wave.TrySeed(2, 0));
    CHECK(sterilize.TryStep(wave, {}, interactions::Reclamation::ProtectedArea{{2.5F, .5F}, 0}));
    CHECK(wave.IsInfected(1, 0) && wave.IsInfected(3, 0) && !wave.IsInfected(2, 0));
    blight::Grid contact{{1, 1, 1}};
    interactions::Reclamation harvest{{1, 1, 1}, 1, {4, 1, 0, 1}};
    const std::array one{SampleState{{1}, {.5F, .5F}, {}}};
    CHECK(contact.TrySeed(0, 0));
    CHECK(harvest.TryStep(contact, one, interactions::Reclamation::ProtectedArea{{.5F, .5F}, 0}));
    CHECK((harvest.GetLedger() == BiomassLedger{5, 3, 1, 1, 1, 1, 0, 0}));
    CHECK(!contact.IsInfected(0, 0)); // ordinary harvest occurs before sanctuary clearing
    return true;
}

bool DefaultConcentration() {
    Simulation simulation{2048, {{64, 32, 1}, 4, SteeringSettings{}, ResourceSettings{}, StructuralSettings{}}};
    CHECK(simulation.GetStructuralState()->eligible_mobile == 49);
    CHECK(simulation.TryFuseRelay() == StructuralCommandResult::insufficient_mass);
    CHECK(simulation.TryApplyFieldEdit({FieldEditKind::set, 0, {48.5F, 16.5F}, 8, 4}));
    for (int tick = 0; tick < 60; ++tick) simulation.tick();
    auto frame = Capture(simulation, 2048, 2048);
    std::array<SampleId, 64> expected{};
    std::size_t eligible = 0;
    for (const auto& sample : frame.samples) {
        const double x = static_cast<double>(sample.position.x) - 48.5;
        const double y = static_cast<double>(sample.position.y) - 16.5;
        if (x * x + y * y <= 16) {
            if (eligible < expected.size()) expected[eligible] = sample.id;
            ++eligible;
        }
    }
    CHECK(eligible >= 64 && simulation.GetStructuralState()->eligible_mobile == eligible);
    CHECK(simulation.TryFuseRelay() == StructuralCommandResult::applied);
    CHECK(simulation.GetStructuralState()->members == expected);
    CHECK(simulation.GetBiomassLedger()->mobile_mass == 1984);
    CHECK(simulation.TryShatterRelay(1) == StructuralCommandResult::applied);
    CHECK(simulation.GetBiomassLedger()->mobile_mass == 2032 && simulation.GetBiomassLedger()->lost_mass == 16);
    for (int tick = 0; tick < 60; ++tick) simulation.tick();
    CHECK(simulation.TryFuseRelay() == StructuralCommandResult::applied);
    CHECK(simulation.GetStructuralState()->generation == 2);
    CHECK(simulation.TryShatterRelay(1) == StructuralCommandResult::stale_generation);
    return true;
}

bool StartupAndDisabled() {
    Simulation legacy{1};
    CHECK(!legacy.GetStructuralState());
    CHECK(legacy.TryFuseRelay() == StructuralCommandResult::disabled);
    CHECK(legacy.TryShatterRelay(1) == StructuralCommandResult::disabled);
    auto options = Simulation::ScenarioOptions{{64, 32, 1}, 4, {}, ResourceSettings{}, StructuralSettings{}};
    const auto rejects = [](Simulation::ScenarioOptions settings) {
        try { Simulation invalid{64, settings}; } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    auto invalid = options; invalid.resources.reset(); CHECK(rejects(invalid));
    invalid = options; invalid.resources->mass_per_sample = 2; CHECK(rejects(invalid));
    invalid = options; invalid.structural->relay_center.x = 65; CHECK(rejects(invalid));
    invalid = options; invalid.structural->eligibility_radius = std::numeric_limits<float>::infinity(); CHECK(rejects(invalid));
    invalid = options; invalid.structural->protection_radius = -1; CHECK(rejects(invalid));
    invalid = options; invalid.structural->hold_ticks = 0; CHECK(rejects(invalid));
    return true;
}
}
int main() {
    if (!LedgerAndSparseOracle() || !MembershipAndRefusalOracle() || !EmptyMobileAndProtection() ||
        !DefaultConcentration() || !StartupAndDisabled()) return 1;
    std::cout << "Structural transitions, mobile-prefix accounting and protection passed\n";
}
