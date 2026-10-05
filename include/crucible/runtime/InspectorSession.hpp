#pragma once
#include <crucible/runtime/ClockDriver.hpp>
#include <crucible/contracts/ReclamationMission.hpp>
#include <memory>
namespace crucible::presentation { class ScenarioSnapshot; }
namespace crucible::runtime {
/** Owns the interactive reference scenario, clock, bounded commands and display frame.
 * Every operation requires one coordinator. No ECS or renderer borrow escapes.
 */
class InspectorSession {
public:
    /** Allocates the fixed 64x32, four-field finite-resource scenario and initial frame.
     * @param[in] samples Fixed population; no subsequent structural growth.
     * @param[in] limits Startup bounds for pending commands and completed trace.
     * @param[in] mission Optional positive quota/deadline challenge; quota must not
     * exceed startup substrate stock. Omission retains ordinary inspector behavior.
     * @throws std::invalid_argument Invalid count/limits; allocation errors propagate.
     */
    explicit InspectorSession(std::size_t samples = 2048,
        HeadlessSession::Limits limits = {64, 4096},
        std::optional<ReclamationMissionSettings> mission = std::nullopt, bool structural = false);
    ~InspectorSession();
    InspectorSession(const InspectorSession&) = delete;
    InspectorSession& operator=(const InspectorSession&) = delete;
    /** Publishes a frame only after successful boundaries.
     * @param[in] elapsed Time since the last call/resume; never includes suspended time.
     * @return Bounded pump outcome with completed-boundary counters.
     * @throws std::logic_error Frame capacity invariant fails; simulation errors propagate.
     */
    [[nodiscard]] ClockDriver::PumpResult TryPump(std::chrono::nanoseconds elapsed);
    /** Copies one edit into the bounded ingress; acceptance is not application.
     * @param[in] edit Owned one-slot intent; no caller storage is retained.
     * @return Accepted sequence or all-or-nothing rejection from CommandIngress.
     */
    [[nodiscard]] CommandIngress::Admission TryAdmitFieldEdit(const FieldEdit& edit);
    [[nodiscard]] CommandIngress::Admission TryFuseRelay();
    [[nodiscard]] CommandIngress::Admission TryShatterRelay();
    [[nodiscard]] std::optional<StructuralCommandResult> GetLastStructuralResult() const noexcept;
    void Pause() noexcept;
    void Resume() noexcept;
    void Close();
    /// Constructs a complete fresh run first; construction failure preserves the old run.
    void Restart();
    [[nodiscard]] ClockDriver::Status GetStatus() const noexcept;
    [[nodiscard]] ClockDriver::Summary GetSummary() const;
    /// Returns owned completed-boundary mission progress, absent for ordinary inspectors.
    [[nodiscard]] std::optional<ReclamationMissionProgress> GetMission() const noexcept;
    /// Borrow expires on next successful pump, restart or destruction.
    [[nodiscard]] const presentation::ScenarioSnapshot& GetSnapshot() const noexcept;
    /// Borrow expires on restart/destruction; completed trace prefix never changes.
    [[nodiscard]] std::span<const AppliedCommand> GetTrace() const noexcept;
private:
    struct Run;
    const std::size_t samples_;
    const HeadlessSession::Limits limits_;
    const std::optional<ReclamationMissionSettings> mission_;
    const bool structural_;
    std::unique_ptr<Run> run_;
};
}
