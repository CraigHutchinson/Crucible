#pragma once
#include <crucible/runtime/ClockDriver.hpp>
#include <crucible/contracts/ReclamationMission.hpp>
#include "crucible/contracts/scenario_settings.hpp"
#include <memory>
namespace crucible::runtime { class RuntimeDiagnostics; }
namespace crucible::presentation { class ScenarioSnapshot; }
namespace crucible::runtime {
/** Owns the interactive reference scenario, clock, bounded commands and display frame.
 * Every operation requires one coordinator. No ECS or renderer borrow escapes.
 */
class InspectorSession {
public:
    /// Direct is an explicit receiving/measurement comparator, sharing all game rules.
    enum class ExecutionPath { integrated, direct };
    /// Bounded opts into a fixed session-lifetime log; requires the CMake feature.
    enum class Diagnostics { disabled, bounded };
    /** Allocates the fixed 64x32, four-field finite-resource scenario and initial frame.
     * @param[in] samples Fixed population; no subsequent structural growth.
     * @param[in] limits Startup bounds for pending commands and completed trace.
     * @param[in] mission Optional positive quota/deadline challenge; quota must not
     * exceed startup substrate stock. Omission retains ordinary inspector behavior.
     * @param[in] structural Enables the fixed relay mission and structural commands.
     * @param[in] execution Production backbone or direct receiving comparator.
     * @param[in] diagnostics Disabled by default; bounded requires the CMake feature.
     * Diagnostic allocation failure disables the optional sink; GetDiagnostics()
     * returns null and the desktop reports the failure without stopping the run.
     * @throws std::invalid_argument Invalid count/limits; allocation errors propagate.
     */
    explicit InspectorSession(std::size_t samples = 2048,
        HeadlessSession::Limits limits = {64, 4096},
        std::optional<ReclamationMissionSettings> mission = std::nullopt, bool structural = false,
        ExecutionPath execution = ExecutionPath::integrated,
        Diagnostics diagnostics = Diagnostics::disabled);
    /** Allocates a complete owned scenario from one authoritative startup value.
     * @param scenario Fixed geometry, population, fields and resource/steering rules.
     * @param limits Pending command and completed trace capacities.
     * @param mission Optional quota policy; omission leaves scale scenarios evolving.
     * @param execution Integrated production path or direct receiving comparator.
     * @param diagnostics Optional bounded outcome sink, with the existing failure policy.
     * @throws std::invalid_argument Invalid geometry, capacities or mission settings.
     * @note Coordinator-only; restart constructs the same scenario before replacement.
     */
    explicit InspectorSession(ScenarioSettings scenario,
        HeadlessSession::Limits limits = {64, 4096},
        std::optional<ReclamationMissionSettings> mission = std::nullopt,
        ExecutionPath execution = ExecutionPath::integrated,
        Diagnostics diagnostics = Diagnostics::disabled);
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
    /// Optional sink borrow expires on destruction; decode only while the coordinator is idle.
    [[nodiscard]] const RuntimeDiagnostics* GetDiagnostics() const noexcept;
    /** Identifies the current run independently of completed tick and trace indices.
     * @return Nonzero session-local identity; increments only after a successful restart.
     * @note Coordinator-only observation; no storage borrow is returned.
     */
    [[nodiscard]] constexpr std::uint64_t getRunId() const noexcept { return run_id_; }
private:
    struct Run;
    [[nodiscard]] CommandIngress::Admission AdmitCommand(const BoundaryCommand& command);
    const ScenarioSettings scenarioSettings_;
    const HeadlessSession::Limits limits_;
    const std::optional<ReclamationMissionSettings> mission_;
    const ExecutionPath execution_;
    std::uint64_t run_id_{1};
    std::unique_ptr<Run> run_;
#if CRUCIBLE_ENABLE_DIAGNOSTICS
    std::unique_ptr<RuntimeDiagnostics> diagnostics_;
#endif
    std::size_t diagnostic_trace_size_{};
    bool mission_summary_recorded_{};
};
}
