#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string_view>

#include "crucible/application/progress_profile.hpp"

namespace crucible::application
{
/** Receives a bounded versioned profile and replaces it only after a complete write.
 * An exclusive sibling transaction directory prevents cooperating writers from
 * sharing temporary bytes. The destination's current version/format is validated
 * under that lease before replacement; rejected writes preserve its bytes.
 * Uncoordinated external edits during the transaction are outside this contract.
 * @note Cold coordinator work. Crash/power-loss durability is not established.
 */
class ProfileStore
{
public:
    /// Load/save failures retain their cause without presenting a false continuation.
    enum class Error { io, malformed, incompatible, transactionBusy };

    /** Retains one explicit profile destination; its parent must already exist.
     * @param path Nonempty profile-file path supplied by the application owner.
     * @throws std::invalid_argument Empty path or missing filename.
     */
    explicit ProfileStore(std::filesystem::path path);
    /** Reads at most 256 bytes and validates all fields and version before returning.
     * @return No profile for an absent file, a complete value, or a typed refusal.
     * @throws std::bad_alloc Cold path/string allocation failure.
     */
    [[nodiscard]] std::expected<std::optional<ProgressProfile>, Error> tryLoad() const;
    /** Writes complete validated bytes, then performs a same-parent replacement.
     * @param profile Mission-boundary continuation and consumed preferences.
     * @return Success only after replacement; refusals preserve the previous file.
     * @throws std::bad_alloc Cold path/string allocation failure.
     */
    [[nodiscard]] std::expected<void, Error> trySave(const ProgressProfile& profile) const;
    /** Supplies a bounded player-facing explanation for a profile refusal.
     * @param error Load/save disposition.
     * @return Static UTF-8 label; no caller borrow or allocation.
     */
    [[nodiscard]] static constexpr std::string_view getErrorText(Error error) noexcept
    {
        switch (error)
        {
        case Error::io: return "Progress could not be read or written. The previous file was retained.";
        case Error::malformed: return "The progress file is invalid. It has not been overwritten.";
        case Error::incompatible: return "This progress version is unsupported. The file was retained.";
        case Error::transactionBusy: return "Another progress transaction is present. The previous file was retained.";
        }
        return "Progress was refused.";
    }

private:
    std::filesystem::path path_;
};
}
