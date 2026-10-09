#pragma once
#include <crucible/runtime/ClockDriver.hpp>
#include <crucible/contracts/ReclamationMission.hpp>
#include "crucible/contracts/scenario_settings.hpp"
#include "crucible/contracts/tick_statistics.hpp"
#include "crucible/contracts/execution_settings.hpp"
#include "crucible/contracts/row_execution_storage.hpp"
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
    /** Startup-bounded receiving observations, independent of outcome diagnostics.
     * A positive capacity requires integrated execution. Stage clocks are an explicit
     * attribution arm, excluded from ordinary baseline/candidate timing comparisons.
     */
    struct ObservationSettings { std::size_t capacity{}; bool simulationStages{}; };
    /** One successful integrated boundary, including command application, simulation,
     * snapshot and mission publication. Immutable values never retain worker/ECS borrows.
     */
    struct TickObservation {
        std::uint64_t runId{}, completedTick{};
        std::chrono::nanoseconds boundary{};
        std::size_t appliedCommands{};
        std::optional<TickStatistics> simulation{};
    };
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
     * @param observations Startup receiving capacity; positive requires integrated execution.
     * Failed boundaries are not observations. Stage attribution requires positive capacity.
     * @param rowExecution Startup workers (1..32) and partitions (1..128, no larger
     * than max(population,1)). Partitions0 resolves to1 or twice the worker count.
     * @throws std::invalid_argument Invalid geometry, capacities or mission settings.
     * @note Coordinator-only; restart constructs the same scenario before replacement.
     */
    explicit InspectorSession(ScenarioSettings scenario,
        HeadlessSession::Limits limits = {64, 4096},
        std::optional<ReclamationMissionSettings> mission = std::nullopt,
        ExecutionPath execution = ExecutionPath::integrated,
        Diagnostics diagnostics = Diagnostics::disabled,
        ObservationSettings observations = {0, false}, ExecutionSettings rowExecution = {});
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
    /** Borrows the immutable completed prefix of startup-bounded tick observations.
     * @return Empty when disabled; records append without overwriting. Failed/paused
     * boundaries add nothing. Borrow expires on successful restart or destruction.
     * @note Coordinator-only; copy before restart, never retain on a renderer/worker.
     */
    [[nodiscard]] std::span<const TickObservation> getTickObservations() const noexcept;
    /// Saturating count of successful boundaries omitted after startup storage fills.
    [[nodiscard]] std::uint64_t getDroppedTickObservations() const noexcept;
    /// Optional sink borrow expires on destruction; decode only while the coordinator is idle.
    [[nodiscard]] const RuntimeDiagnostics* GetDiagnostics() const noexcept;
    /** Identifies the current run independently of completed tick and trace indices.
     * @return Nonzero session-local identity; increments only after a successful restart.
     * @note Coordinator-only observation; no storage borrow is returned.
     */
    [[nodiscard]] constexpr std::uint64_t getRunId() const noexcept { return run_id_; }
    /** Copies the resolved startup row execution policy retained by restart.
     * @return Positive worker/partition bounds, independent of per-tick FP fallback.
     * @note Coordinator-only; no scheduler or mutable simulation borrow escapes.
     */
    [[nodiscard]] constexpr ExecutionSettings getRowExecution() const noexcept { return rowExecution_; }
    /** Copies actual startup row storage bounds from the owned Simulation.
     * @return Retained scratch, graph and pool queue capacities, including paused runs.
     * @note Coordinator-only; no scheduler or mutable storage borrow escapes.
     */
    [[nodiscard]] RowExecutionStorage getRowExecutionStorage() const noexcept;
    /** Copies completed Simulation ticks recomputed after unsupported FP in this run.
     * @return Cumulative Simulation count, reset by successful restart.
     * @note Coordinator-only; this reports sequential recomputation, not worker utilization.
     */
    [[nodiscard]] std::uint64_t getRowFallbackCount() const noexcept;
private:
    struct Run;
    [[nodiscard]] CommandIngress::Admission AdmitCommand(const BoundaryCommand& command);
    const ScenarioSettings scenarioSettings_;
    const HeadlessSession::Limits limits_;
    const std::optional<ReclamationMissionSettings> mission_;
    const ExecutionPath execution_;
    const ObservationSettings observationSettings_{};
    const ExecutionSettings rowExecution_{};
    std::uint64_t run_id_{1};
    std::unique_ptr<Run> run_;
#if CRUCIBLE_ENABLE_DIAGNOSTICS
    std::unique_ptr<RuntimeDiagnostics> diagnostics_;
#endif
    std::size_t diagnostic_trace_size_{};
    bool mission_summary_recorded_{};
};
}
