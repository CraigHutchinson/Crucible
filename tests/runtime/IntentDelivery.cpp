// Receive synchronous Pub admission: isolated live runs, owned queued batches,
// normal refusals and restart teardown. Delivery never applies world commands.
#include <crucible/runtime/IntentDelivery.hpp>

#include <array>
#include <iostream>
#include <stdexcept>

namespace {
using crucible::BoundaryCommand;
using crucible::FieldEdit;
using crucible::FieldEditKind;
using crucible::runtime::AdmittedCommand;
using crucible::runtime::CommandIngress;
using crucible::runtime::IntentDelivery;
using Status = CommandIngress::AdmissionStatus;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

BoundaryCommand Edit(std::size_t slot) {
    return FieldEdit{FieldEditKind::set_flow, slot, {2, 3}, 4, 8, {10, 3}};
}

void CheckIsolationAndOwnership() {
    CommandIngress first{{4, 2}}, second{{4, 2}};
    IntentDelivery one{first, 11}, two{second, 12};
    auto commands = std::array{Edit(0), BoundaryCommand::Fuse(), BoundaryCommand::Shatter(7)};
    const auto receipt = one.TryAdmitCommands(commands);
    Require(receipt.run_id == 11 && receipt.request_id == 1 &&
        receipt.admission.status == Status::accepted &&
        receipt.admission.first_sequence == 1 && receipt.admission.last_sequence == 3,
        "owned correlated batch receipt");
    Require(second.GetStatistics().pending == 0, "live run cannot cross-deliver");
    commands[0].field.end = {};
    commands[2].generation = 99;
    const std::array other{Edit(1)};
    const auto second_receipt = two.TryAdmitCommands(other);
    Require(second_receipt.run_id == 12 && second_receipt.request_id == 1 &&
        second_receipt.admission.first_sequence == 1 && first.GetStatistics().pending == 3,
        "independent run sequence and request IDs");
    std::array<AdmittedCommand, 4> output{};
    const auto drained = first.TryDrainThrough(first.CaptureCutoff(), output);
    Require(drained && drained->size() == 3 && output[0].command.field.end.x == 10 &&
        output[1].command.action == crucible::BoundaryAction::fuse &&
        output[2].command.generation == 7, "Pub accepted batch owns source values");
    Require(receipt.admission.last_sequence == 3 && receipt.request_id == 1,
        "earlier receipt remains owned after later deliveries");
}

void CheckRefusalsAgainstDirect() {
    CommandIngress routed{{2, 1}}, direct{{2, 1}};
    IntentDelivery delivery{routed, 21};
    std::uint64_t request{};
    auto compare = [&](std::span<const BoundaryCommand> commands, Status expected) {
        const auto received = delivery.TryAdmitCommands(commands);
        const auto baseline = direct.TryAdmitCommands(commands);
        Require(received.request_id == ++request && received.run_id == 21 &&
            received.admission.status == expected && received.admission.status == baseline.status &&
            received.admission.first_sequence == baseline.first_sequence &&
            received.admission.last_sequence == baseline.last_sequence, "direct admission parity");
        const auto pub_stats = routed.GetStatistics(), direct_stats = direct.GetStatistics();
        Require(pub_stats.pending == direct_stats.pending && pub_stats.accepted == direct_stats.accepted &&
            pub_stats.rejected == direct_stats.rejected, "refusals preserve exact ingress statistics");
    };
    compare({}, Status::invalid);
    auto oversized = std::array{Edit(0), BoundaryCommand::Fuse(), BoundaryCommand::Shatter(0)};
    compare(oversized, Status::full);
    oversized.back() = Edit(1);
    compare(oversized, Status::invalid);
    const std::array accepted{Edit(0), BoundaryCommand::Fuse()};
    compare(accepted, Status::accepted);
    compare(accepted, Status::full);
    routed.Close(); direct.Close();
    compare({}, Status::closed);
    compare(accepted, Status::closed);
    std::array<AdmittedCommand, 2> output{};
    Require(routed.TryDrainThrough(routed.CaptureCutoff(), output)->size() == 2,
        "closed route retains already admitted batch");
}

void CheckCutoffAndRestart() {
    CommandIngress ingress{{4, 1}};
    const std::array batch{Edit(0)};
    const auto retained = [&] {
        IntentDelivery old{ingress, 31};
        return old.TryAdmitCommands(batch);
    }();
    const auto cutoff = ingress.CaptureCutoff();
    IntentDelivery replacement{ingress, 32};
    const auto next = replacement.TryAdmitCommands(batch);
    Require(retained.run_id == 31 && next.run_id == 32 && next.request_id == 1 &&
        next.admission.first_sequence == 2 && ingress.GetStatistics().pending == 2,
        "destroyed most-derived sink neither survives nor duplicates new delivery");
    std::array<AdmittedCommand, 4> output{};
    const auto prefix = ingress.TryDrainThrough(cutoff, output);
    Require(prefix && prefix->size() == 1 && ingress.GetStatistics().pending == 1,
        "Pub leaves captured cutoff unchanged");
    for (std::uint64_t run = 33; run != 97; ++run) {
        CommandIngress queue{{1, 1}};
        IntentDelivery route{queue, run};
        Require(route.TryAdmitCommands(batch).admission.status == Status::accepted,
            "repeated per-domain registration and teardown");
    }
    bool failed{};
    try { IntentDelivery invalid{ingress, 0}; }
    catch (const std::invalid_argument&) { failed = true; }
    Require(failed, "zero correlation run ID rejected at startup");
}
}

int main() {
    try {
        CheckIsolationAndOwnership();
        CheckRefusalsAgainstDirect();
        CheckCutoffAndRestart();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
