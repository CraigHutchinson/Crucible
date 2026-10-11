#pragma once

#include <crucible/runtime/CommandIngress.hpp>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <span>

namespace crucible {
enum class StructuralCommandResult;
struct ReclamationMissionProgress;
}

namespace crucible::runtime {

/** Captures runtime refusals and completed mission results without growing storage.
 * One coordinator owns all calls; scoped logger bindings must not overlap calls
 * from another thread. Diagnostic losses never alter simulation outcomes.
 */
class RuntimeDiagnostics {
public:
    /// Owned identity of the last published boundary, including fresh-run identity.
    struct Context { std::uint64_t run_id{}, completed_tick{}; };
    /// Producer health; a valid exhausted segment retains its committed prefix.
    struct Statistics {
        bool valid{};
        std::uint64_t dropped_records{}, truncated_records{};
    };

    /// Allocates fixed storage at startup; allocation failures propagate.
    RuntimeDiagnostics();
    ~RuntimeDiagnostics();
    RuntimeDiagnostics(const RuntimeDiagnostics&) = delete;
    RuntimeDiagnostics& operator=(const RuntimeDiagnostics&) = delete;

    /** Copies refusal fields into the segment; accepted admissions are ignored.
     * @param[in] context Last published run and tick, not an application promise.
     * @param[in] command Rejected owned intent; no source storage is retained.
     * @param[in] status Actual admission disposition.
     */
    void RecordAdmissionRefusal(Context context, const BoundaryCommand& command,
        CommandIngress::AdmissionStatus status) noexcept;
    /** Copies a domain refusal observed after a completed boundary.
     * @param[in] context Published run and tick at which application was attempted.
     * @param[in] command Attempted intent, including requested relay generation.
     * @param[in] result Actual structural disposition; applied is ignored.
     */
    void RecordStructuralRefusal(Context context, const BoundaryCommand& command,
        StructuralCommandResult result) noexcept;
    /** Copies a terminal mission result; the session emits once per run.
     * @param[in] run_id Fresh-run identity supplied by the owning session.
     * @param[in] progress Published mission value; active missions are ignored.
     */
    void RecordMissionSummary(std::uint64_t run_id,
        const ReclamationMissionProgress& progress) noexcept;
    /** Returns producer health without allocation.
     * @return Initialization validity and counted dropped/truncated records.
     */
    [[nodiscard]] Statistics GetStatistics() const noexcept;
    /** Borrows binary segment storage for shutdown copy/export.
     * @return Empty on initialization failure; otherwise valid until destruction.
     * Stop emissions before reading or copying this span and keep it alive for
     * every reader/decoder borrow; restart does not invalidate session storage.
     */
    [[nodiscard]] std::span<const std::byte> GetImage() const noexcept;
    /** Decodes the committed prefix and writes text plus producer/decoder health.
     * @param[in,out] output Shutdown destination; emissions must have stopped.
     * @return False for an invalid segment, decoder damage or failed output.
     * Allocation and stream exceptions propagate to the shutdown caller.
     */
    [[nodiscard]] bool WriteDecoded(std::ostream& output) const;

    /** Writes upstream schema-1 JSONL messages and one Crucible health object.
     * @param[in,out] output Message destination; stop all emissions before calling.
     * @param[in,out] health Separate health destination, including actual producer losses.
     * @return False for invalid/damaged input or failed writes/flushes on either stream.
     * @note Cold coordinator-only export; all borrows remain within this call.
     * Counted producer losses do not make the recovered prefix invalid. Allocation
     * and stream exceptions propagate; no complete health receipt is then promised.
     */
    [[nodiscard]] bool writeJson(std::ostream& output, std::ostream& health) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
