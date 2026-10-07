#include <crucible/runtime/CommandIngress.hpp>

#include <array>
#include <atomic>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
using crucible::FieldEdit;
using crucible::FieldEditKind;
using crucible::runtime::AdmittedCommand;
using crucible::runtime::CommandIngress;
using Status = CommandIngress::AdmissionStatus;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

FieldEdit Edit(std::size_t slot) {
    return {FieldEditKind::set, slot, {1.0F, 2.0F}, 3.0F, -4.0F};
}

void CheckAtomicAdmission() {
    CommandIngress ingress{{3, 4}};
    const std::array first{Edit(0), Edit(1)};
    const auto admitted = ingress.TryAdmit(first);
    Require(admitted.status == Status::accepted && admitted.first_sequence == 1 &&
        admitted.last_sequence == 2, "first batch sequence");
    const std::array too_large{Edit(2), Edit(3)};
    Require(ingress.TryAdmit(too_large).status == Status::full, "atomic full rejection");
    auto invalid = too_large;
    invalid.back().slot = 4;
    Require(ingress.TryAdmit(invalid).status == Status::invalid, "atomic invalid rejection");
    Require(ingress.GetStatistics().pending == 2, "rejected batches left queue unchanged");
    const std::array last{Edit(2)};
    const auto accepted_last = ingress.TryAdmit(last);
    Require(accepted_last.first_sequence == 3 && accepted_last.last_sequence == 3,
        "rejection did not consume sequence");
    Require(ingress.TryAdmit(last).status == Status::full, "reject newest at capacity");
    std::array<AdmittedCommand, 3> output{};
    const auto drained = ingress.TryDrainThrough(ingress.CaptureCutoff(), output);
    Require(drained && drained->size() == 3, "accepted batch drain");
    for (std::size_t index{}; index < output.size(); ++index)
        Require(output[index].sequence == index + 1 && output[index].command.field.slot == index,
            "batch order preserved");
    const auto stats = ingress.GetStatistics();
    Require(stats.pending == 0 && stats.accepted == 3 && stats.rejected == 3,
        "admission counters distinguish commands and rejected batches");
}

void CheckCutoffAndWrap() {
    CommandIngress ingress{{3, 3}};
    const std::array first{Edit(0), Edit(1)};
    Require(ingress.TryAdmit(first).status == Status::accepted, "cutoff initial batch");
    const auto cutoff = ingress.CaptureCutoff();
    const std::array late{Edit(2)};
    bool late_accepted{};
    std::jthread producer{[&] {
        late_accepted = ingress.TryAdmit(late).status == Status::accepted;
    }};
    producer.join();
    Require(late_accepted, "late admission");
    std::array<AdmittedCommand, 1> insufficient{};
    Require(!ingress.TryDrainThrough(cutoff, insufficient), "drain capacity rejection");
    Require(ingress.GetStatistics().pending == 3, "small drain leaves whole prefix queued");
    std::array<AdmittedCommand, 3> output{};
    const auto first_drain = ingress.TryDrainThrough(cutoff, output);
    Require(first_drain && first_drain->size() == 2 && output[1].sequence == 2,
        "captured cutoff excludes late arrival");
    Require(ingress.GetStatistics().pending == 1, "late command retained");
    Require(ingress.TryAdmit(first).status == Status::accepted, "wrapped batch admitted");
    const auto second_drain = ingress.TryDrainThrough(ingress.CaptureCutoff(), output);
    Require(second_drain && second_drain->size() == 3 && output[0].sequence == 3 &&
        output[1].sequence == 4 && output[2].sequence == 5, "wrapped FIFO drain");
}

void CheckCloseAndValidation() {
    CommandIngress ingress{{1, 1}};
    Require(ingress.TryAdmit({}).status == Status::invalid, "empty batch rejected");
    auto invalid = std::array{Edit(0)};
    invalid[0].radius = std::numeric_limits<float>::infinity();
    Require(ingress.TryAdmit(invalid).status == Status::invalid, "nonfinite set rejected");
    invalid[0].radius = -1.0F;
    Require(ingress.TryAdmit(invalid).status == Status::invalid, "negative radius rejected");
    auto value = std::array{Edit(0)};
    Require(ingress.TryAdmit(value).status == Status::accepted, "close full setup");
    value[0].center.x = 999.0F;
    ingress.Close();
    ingress.Close();
    Require(ingress.CaptureCutoff().closed, "close out of band");
    Require(ingress.TryAdmit(value).status == Status::closed, "closed admission rejected");
    std::array<AdmittedCommand, 1> output{};
    const auto drained = ingress.TryDrainThrough(ingress.CaptureCutoff(), output);
    Require(drained && drained->size() == 1 && output[0].command.field.center.x == 1.0F,
        "close retains owned pre-close command");
    CommandIngress removals{{1, 1}};
    value[0] = {FieldEditKind::remove, 0, {}, std::numeric_limits<float>::quiet_NaN(), 0};
    Require(removals.TryAdmit(value).status == Status::accepted,
        "remove ignores unused numerical payload");
}

void CheckOwnedAtomicFlowAdmission() {
    CommandIngress ingress{{3, 2}};
    auto batch = std::array{FieldEdit{FieldEditKind::set_flow, 0, {2, 3}, 4, 8, {10, 3}},
                            FieldEdit{FieldEditKind::set_flow, 1, {10, 3}, 4, 8, {2, 3}}};
    auto invalid = batch;
    invalid.back().end = invalid.back().center;
    Require(ingress.TryAdmit(invalid).status == Status::invalid && ingress.GetStatistics().pending == 0 &&
        ingress.GetStatistics().accepted == 0, "invalid flow batch changes no queue prefix");
    const auto accepted = ingress.TryAdmit(batch);
    Require(accepted.status == Status::accepted && accepted.first_sequence == 1 && accepted.last_sequence == 2,
        "flow rejection consumes no sequence");
    Require(ingress.TryAdmit(batch).status == Status::full && ingress.GetStatistics().pending == 2,
        "full flow batch preserves accepted prefix");
    batch[0].center = {}; batch[0].end = {};
    batch[1].end.x = 999;
    const std::array erase{FieldEdit{FieldEditKind::remove, 0}};
    Require(ingress.TryAdmit(erase).last_sequence == 3, "flow overflow consumes no sequence");
    std::array<AdmittedCommand, 3> output{};
    const auto drained = ingress.TryDrainThrough(ingress.CaptureCutoff(), output);
    Require(drained && drained->size() == 3 && output[0].command.field.kind == FieldEditKind::set_flow &&
        output[0].command.field.center.x == 2 && output[0].command.field.end.x == 10 && output[0].command.field.end.y == 3 &&
        output[1].command.field.end.x == 2 && output[2].command.field.kind == FieldEditKind::remove,
        "flow admission owns both endpoints and preserves FIFO erase");
}

void CheckStartupFailure() {
    bool zero_failed{};
    try { CommandIngress invalid{{0, 1}}; }
    catch (const std::invalid_argument&) { zero_failed = true; }
    Require(zero_failed, "zero command capacity fails construction");
    bool overflow_failed{};
    try { CommandIngress invalid{{std::numeric_limits<std::size_t>::max(), 1}}; }
    catch (const std::length_error&) { overflow_failed = true; }
    Require(overflow_failed, "overflow command capacity fails before allocation");
}

void CheckJoinedProducerConsumerLifetime() {
    constexpr std::size_t count = 128;
    CommandIngress ingress{{count, 1}};
    bool producer_ok = true;
    std::atomic<bool> finished{};
    std::jthread producer{[&] {
        for (std::size_t index{}; index < count; ++index) {
            auto value = std::array{Edit(0)};
            value[0].center.x = static_cast<float>(index);
            if (ingress.TryAdmit(value).status != Status::accepted) producer_ok = false;
            value[0].center.x = -1.0F;
        }
        finished.store(true, std::memory_order_release);
    }};
    std::array<AdmittedCommand, count> output{};
    std::size_t consumed{};
    while (true) {
        const auto drained = ingress.TryDrainThrough(ingress.CaptureCutoff(), output);
        Require(drained.has_value(), "concurrent drain capacity");
        for (const auto& command : *drained) {
            Require(command.sequence == consumed + 1 &&
                command.command.field.center.x == static_cast<float>(consumed),
                "concurrent FIFO owns producer values");
            ++consumed;
        }
        if (finished.load(std::memory_order_acquire) && ingress.GetStatistics().pending == 0) break;
    }
    producer.join();
    Require(producer_ok && consumed == count, "joined producer completed before owner destruction");
}
}

int main() {
    try {
        CheckAtomicAdmission();
        CheckOwnedAtomicFlowAdmission();
        CheckCutoffAndWrap();
        CheckCloseAndValidation();
        CheckStartupFailure();
        CheckJoinedProducerConsumerLifetime();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
