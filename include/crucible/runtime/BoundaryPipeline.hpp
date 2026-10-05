#pragma once

#include <crucible/runtime/HeadlessSession.hpp>
#include <crucible/presentation/ScenarioSnapshot.hpp>
#include <functional>
#include <memory>

namespace crucible::runtime {

/** Startup-built sequential boundary, owned capture and completion graph.
 * Session and Simulation are borrowed exclusively and must outlive this object.
 * All calls belong to the coordinator; no job escapes a call or uses a timer.
 * Completion receives the newly captured committed frame and returns terminal.
 * It must commit its own output atomically: throwing cannot undo caller state.
 * A failure retains the last good frame and closes admission permanently.
 */
class BoundaryPipeline {
public:
    using Completion = std::function<bool(const presentation::ScenarioSnapshot&)>;
    /// Captures the initial frame; invalid capacities throw before use.
    BoundaryPipeline(HeadlessSession& session, Simulation& simulation,
        std::size_t samples, std::size_t fields, std::size_t cells,
        Completion completion = {});
    ~BoundaryPipeline();
    BoundaryPipeline(const BoundaryPipeline&) = delete;
    BoundaryPipeline& operator=(const BoundaryPipeline&) = delete;
    /// Normal pause/close/backpressure skips capture and completion entirely.
    /// Exceptions close admission, retain the previous frame, then propagate.
    [[nodiscard]] HeadlessSession::StepResult TryStep();
    /// Owned frame borrow expires on next successful boundary or destruction.
    [[nodiscard]] const presentation::ScenarioSnapshot& GetFrame() const noexcept;
    [[nodiscard]] bool IsTerminal() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
