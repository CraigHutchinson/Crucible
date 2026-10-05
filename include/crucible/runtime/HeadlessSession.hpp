#pragma once

#include <crucible/runtime/CommandIngress.hpp>
#include <crucible/contracts/StructuralState.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace crucible { class Simulation; }

namespace crucible::runtime {

/// An admitted command attempt at a completed tick, including normal domain refusal.
struct AppliedCommand {
    BoundaryCommand command{};
    std::uint64_t sequence{}, tick{};
    StructuralCommandResult result{StructuralCommandResult::applied};
};

/** Sequential headless boundaries with bounded input and an owned replay trace.
 * Simulation is borrowed exclusively and must outlive this session. Only ingress
 * operations may run on producer threads; all session operations belong to one
 * coordinator. Join producers before destroying the session.
 */
class HeadlessSession {
public:
    /// Commands bounds pending input; trace bounds all recorded edits for this run.
    struct Limits { std::size_t commands{}, trace{}; };
    /// A failed application stops this session; trace_full preserves the boundary.
    enum class StepStatus {
        advanced, paused, closed, trace_full, application_failed, tick_exhausted, replay_invalid
    };
    /// Tick counts completed ticks; applied_commands counts processed attempts, including normal refusals.
    struct StepResult {
        StepStatus status{};
        std::uint64_t tick{};
        std::size_t applied_commands{};
    };

    /// Allocates all runtime buffers once; invalid capacities throw before use.
    HeadlessSession(Simulation& simulation, Limits limits);
    HeadlessSession(const HeadlessSession&) = delete;
    HeadlessSession& operator=(const HeadlessSession&) = delete;

    [[nodiscard]] CommandIngress& GetIngress() noexcept;
    [[nodiscard]] std::uint64_t GetCompletedTick() const noexcept;
    /// Suppresses boundaries while leaving bounded admission open.
    void Pause() noexcept;
    /// Queued edits become eligible at the next boundary.
    void Resume() noexcept;
    /** Advances one tick after applying only its captured admission prefix.
     * Trace exhaustion leaves input and simulation untouched. Normal structure refusals are traced and do not stop ticking. Unexpected field
     * rejection closes admission and stops without ticking or recording a completed boundary.
     * A throwing simulation tick also stops the session, then propagates the error.
     */
    [[nodiscard]] StepResult TryStep();
    /// Borrowed trace is valid until session destruction; only its used prefix is exposed.
    [[nodiscard]] std::span<const AppliedCommand> GetTrace() const noexcept;
    /** Replays a capacity-bounded trace on a fresh matching Simulation.
     * Requires a pristine, unpaused session and quiescent producers. Validates the
     * entire input (consecutive sequences, ordered ticks, valid payloads and possible results) before any
     * mutation, then closes admission and runs exactly target_ticks boundaries.
     * Simulation's initial state and configuration must match the original run.
     */
    [[nodiscard]] StepResult TryReplay(
        std::span<const AppliedCommand> trace, std::uint64_t target_ticks);

private:
    /// Construction token guarantees every buffer's arithmetic was checked first.
    struct ValidatedLimits {
        explicit ValidatedLimits(Limits limits) noexcept : value(limits) {}
        Limits value;
    };
    [[nodiscard]] static ValidatedLimits ValidateLimits(Limits limits);
    HeadlessSession(Simulation& simulation, ValidatedLimits limits);
    void CompleteTick();

    Simulation& simulation_; // non-owning; caller retains exclusive ownership
    CommandIngress ingress_;
    std::vector<AdmittedCommand> boundary_;
    std::vector<AppliedCommand> trace_;
    std::vector<StructuralCommandResult> results_;
    std::size_t trace_size_{};
    std::uint64_t tick_{};
    bool paused_{}, failed_{};
};

}
