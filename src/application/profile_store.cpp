#include <array>
#include <charconv>
#include <fstream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "crucible/application/profile_store.hpp"

namespace crucible::application
{
namespace
{
std::optional<unsigned> parseUnsigned(std::istringstream& fields)
{
    std::string token;
    if (!(fields >> token)) return std::nullopt;
    unsigned value{};
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) return std::nullopt;
    return value;
}

bool isValid(const ProgressProfile& profile) noexcept
{
    const auto mission = profile.nextMission_;
    const auto scale = profile.options_.textScale_;
    return (mission == MissionId::reclaimFront || mission == MissionId::secureRelay) &&
        (mission != MissionId::secureRelay || profile.relayUnlocked_) &&
        (scale == TextScale::standard || scale == TextScale::large || scale == TextScale::extraLarge);
}

class SaveTransaction
{
public:
    explicit SaveTransaction(const std::filesystem::path& destination)
        : directory_{destination.parent_path() / (destination.filename().native() +
            std::filesystem::path{".pending"}.native())}, temporary_{directory_ / "profile.tmp"}
    {
    }
    ~SaveTransaction()
    {
        if (!owned_) return;
        std::error_code error;
        std::filesystem::remove(temporary_, error);
        std::filesystem::remove(directory_, error);
    }
    SaveTransaction(const SaveTransaction&) = delete;
    SaveTransaction& operator=(const SaveTransaction&) = delete;

    std::filesystem::path directory_, temporary_;
    bool owned_{};
};
}

ProfileStore::ProfileStore(std::filesystem::path path) : path_{std::move(path)}
{
    if (path_.empty() || path_.filename().empty())
        throw std::invalid_argument("Profile destination must name a file");
}

std::expected<std::optional<ProgressProfile>, ProfileStore::Error> ProfileStore::tryLoad() const
{
    std::error_code error;
    const bool exists = std::filesystem::exists(path_, error);
    if (error) return std::unexpected(Error::io);
    if (!exists) return std::optional<ProgressProfile>{};
    if (!std::filesystem::is_regular_file(path_, error) || error) return std::unexpected(Error::io);
    std::ifstream input{path_, std::ios::binary};
    if (!input) return std::unexpected(Error::io);
    std::array<char, 257> bytes{};
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    const auto count = input.gcount();
    if (input.bad()) return std::unexpected(Error::io);
    if (count > 256) return std::unexpected(Error::malformed);
    std::istringstream fields{std::string{bytes.data(), static_cast<std::size_t>(count)}};
    fields.imbue(std::locale::classic());
    std::string magic;
    if (!(fields >> magic) || magic != "CRUCIBLE_PROGRESS")
        return std::unexpected(Error::malformed);
    const auto version = parseUnsigned(fields);
    if (!version) return std::unexpected(Error::malformed);
    if (*version != 1) return std::unexpected(Error::incompatible);
    std::array<unsigned, 6> values{};
    for (auto& value : values)
    {
        const auto parsed = parseUnsigned(fields);
        if (!parsed) return std::unexpected(Error::malformed);
        value = *parsed;
    }
    const auto& [continuation, mission, unlocked, motion, fullscreen, scale] = values;
    fields >> std::ws;
    if (!fields.eof() || continuation > 1 || mission > 1 || unlocked > 1 || motion > 1 ||
        fullscreen > 1 || scale > 2) return std::unexpected(Error::malformed);
    ProgressProfile profile{continuation != 0, static_cast<MissionId>(mission), unlocked != 0,
        {motion != 0, fullscreen != 0, static_cast<TextScale>(scale)}};
    if (!isValid(profile)) return std::unexpected(Error::malformed);
    return std::optional<ProgressProfile>{profile};
}

std::expected<void, ProfileStore::Error> ProfileStore::trySave(const ProgressProfile& profile) const
{
    if (!isValid(profile)) return std::unexpected(Error::malformed);
    SaveTransaction transaction{path_};
    std::error_code error;
    if (!std::filesystem::create_directory(transaction.directory_, error))
        return std::unexpected(error ? Error::io : Error::transactionBusy);
    transaction.owned_ = true;
    const auto previous = tryLoad();
    if (!previous) return std::unexpected(previous.error());
    std::ofstream output{transaction.temporary_, std::ios::binary | std::ios::trunc};
    output.imbue(std::locale::classic());
    output << "CRUCIBLE_PROGRESS 1\n" << static_cast<unsigned>(profile.hasContinuation_) << ' '
        << static_cast<unsigned>(profile.nextMission_) << ' '
        << static_cast<unsigned>(profile.relayUnlocked_) << ' '
        << static_cast<unsigned>(profile.options_.reducedMotion_) << ' '
        << static_cast<unsigned>(profile.options_.fullscreen_) << ' '
        << static_cast<unsigned>(profile.options_.textScale_) << '\n';
    output.flush();
    output.close();
    if (!output) return std::unexpected(Error::io);
#if defined(_WIN32)
    if (!MoveFileExW(transaction.temporary_.c_str(), path_.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return std::unexpected(Error::io);
#else
    std::filesystem::rename(transaction.temporary_, path_, error);
    if (error) return std::unexpected(Error::io);
#endif
    return {};
}
}
