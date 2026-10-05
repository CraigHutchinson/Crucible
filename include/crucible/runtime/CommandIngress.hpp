#pragma once

#include <crucible/contracts/BoundaryCommand.hpp>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

namespace crucible::runtime {

/// An owned edit tagged with its successful admission order.
struct AdmittedCommand {
    BoundaryCommand command{};
    std::uint64_t sequence{};
};

/** Bounded owned input shared by producers and one boundary coordinator.
 * All operations serialize on a mutex. Callers must join producers and the
 * coordinator before destruction; no caller storage is retained.
 */
class CommandIngress {
public:
    /// Fixed storage and the field-slot bound checked at admission.
    struct Limits { std::size_t commands{}, fields{}; };
    /// Admission failures never consume a sequence or partially enqueue a batch.
    enum class AdmissionStatus { accepted, full, closed, invalid, sequence_exhausted };
    /// Successful batches occupy the inclusive sequence range; failures use zero.
    struct Admission {
        AdmissionStatus status{};
        std::uint64_t first_sequence{}, last_sequence{};
    };
    /// Boundary snapshot; later admissions have larger sequences.
    struct Cutoff { std::uint64_t sequence{}; bool closed{}; };
    /// Rejected batches saturate at uint64's maximum; accepted counts individual commands.
    struct Statistics {
        std::size_t pending{};
        std::uint64_t accepted{}, rejected{};
    };

    /// Throws invalid_argument for zero command capacity, length_error for overflow.
    explicit CommandIngress(Limits limits);
    CommandIngress(const CommandIngress&) = delete;
    CommandIngress& operator=(const CommandIngress&) = delete;

    /// Copies an entire nonempty batch or rejects it without changing the ring.
    [[nodiscard]] Admission TryAdmit(std::span<const FieldEdit> edits);
    [[nodiscard]] Admission TryAdmitCommands(std::span<const BoundaryCommand> commands);
    [[nodiscard]] Cutoff CaptureCutoff() const;
    /** Copies and removes the prefix through cutoff into caller-owned storage.
     * Returns nullopt without removing anything when output is too small. The
     * returned span borrows output and may be empty. Closing does not discard input.
     */
    [[nodiscard]] std::optional<std::span<const AdmittedCommand>> TryDrainThrough(
        Cutoff cutoff, std::span<AdmittedCommand> output);
    /// Idempotently rejects future input even when the ring is full.
    void Close();
    [[nodiscard]] Statistics GetStatistics() const;

private:
    template<class Payload>
    [[nodiscard]] Admission TryAdmitLocked(std::span<const Payload> commands);
    [[nodiscard]] Admission RejectAdmission(AdmissionStatus status) noexcept;

    std::vector<AdmittedCommand> ring_;
    const std::size_t field_capacity_;
    mutable std::mutex mutex_;
    std::size_t head_{}, pending_{};
    std::uint64_t last_sequence_{}, rejected_{};
    bool closed_{};
};

}
