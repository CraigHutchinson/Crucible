#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>

#include "crucible/application/game_journey.hpp"
#include "crucible/application/profile_store.hpp"

namespace
{
using namespace crucible::application;

void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

std::string readBytes(const std::filesystem::path& path)
{
    std::ifstream input{path, std::ios::binary};
    require(static_cast<bool>(input), "Fixture read failed");
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

void writeBytes(const std::filesystem::path& path, const std::string& bytes)
{
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    output << bytes;
    output.close();
    require(static_cast<bool>(output), "Fixture write failed");
}

class Fixture
{
public:
    explicit Fixture(const std::filesystem::path& parent)
        : directory_{parent / ("profile-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))},
          path_{directory_ / "progress.txt"}, pending_{directory_ / "progress.txt.pending"}
    {
        require(std::filesystem::create_directory(directory_), "Fixture directory was not exclusive");
    }
    ~Fixture()
    {
        std::error_code error;
        std::filesystem::remove(pending_ / "unrelated.txt", error);
        std::filesystem::remove(pending_, error);
        std::filesystem::remove(path_, error);
        std::filesystem::remove(directory_, error);
    }
    std::filesystem::path directory_, path_, pending_;
};

void checkPersistence(const std::filesystem::path& parent)
{
    Fixture fixture{parent};
    ProfileStore store{fixture.path_};
    const auto absent = store.tryLoad();
    require(absent && !*absent, "An absent profile fabricated continuation");
    const ProgressProfile profile{true, MissionId::secureRelay, true,
        {true, false, TextScale::large}};
    require(store.trySave(profile).has_value(), "Valid profile did not save");
    const auto goodBytes = readBytes(fixture.path_);
    require(goodBytes == "CRUCIBLE_PROGRESS 1\n1 1 1 1 0 1\n", "Profile wire mapping changed");
    const auto loaded = store.tryLoad();
    require(loaded && *loaded && (**loaded).nextMission_ == MissionId::secureRelay &&
        (**loaded).relayUnlocked_ && (**loaded).options_.reducedMotion_ &&
        (**loaded).options_.textScale_ == TextScale::large, "Profile round trip changed consumed values");
    ProgressProfile invalid = profile;
    invalid.relayUnlocked_ = false;
    require(!store.trySave(invalid) && readBytes(fixture.path_) == goodBytes,
        "Rejected profile changed the last good file");
    require(std::filesystem::create_directory(fixture.pending_), "Busy fixture creation failed");
    writeBytes(fixture.pending_ / "unrelated.txt", "OTHER OWNER");
    const auto busy = store.trySave(ProgressProfile{});
    require(!busy && busy.error() == ProfileStore::Error::transactionBusy &&
        readBytes(fixture.path_) == goodBytes &&
        readBytes(fixture.pending_ / "unrelated.txt") == "OTHER OWNER",
        "Busy transaction replaced or cleaned another owner's data");
    std::filesystem::remove(fixture.pending_ / "unrelated.txt");
    std::filesystem::remove(fixture.pending_);
    require(store.trySave(ProgressProfile{}).has_value(), "Replacement of an existing profile failed");
    const auto replaced = store.tryLoad();
    require(replaced && *replaced && !(**replaced).hasContinuation_, "Old profile was not replaced");
    writeBytes(fixture.path_, "CRUCIBLE_PROGRESS 2\n1 1 1 1 0 1\n");
    require(!store.tryLoad() && store.tryLoad().error() == ProfileStore::Error::incompatible,
        "Unknown version was accepted");
    writeBytes(fixture.path_, "CRUCIBLE_PROGRESS 1\n1 1 0 0 0 0\n");
    require(!store.tryLoad(), "Locked relay continuation was accepted");
    writeBytes(fixture.path_, "CRUCIBLE_PROGRESS 1\n0 0 0 0 0 0\nTRAILING");
    require(!store.tryLoad(), "Trailing data was accepted");
    writeBytes(fixture.path_, "CRUCIBLE_PROGRESS -4294967295\n1 1 1 1 0 1\n");
    require(!store.tryLoad(), "Signed version alias was accepted");
    writeBytes(fixture.path_, "CRUCIBLE_PROGRESS 1\n-4294967295 1 1 1 0 1\n");
    require(!store.tryLoad(), "Signed continuation alias was accepted");
    writeBytes(fixture.path_, std::string(257, 'x'));
    require(!store.tryLoad(), "Oversized profile was accepted");
}

void checkJourneyFailures(const std::filesystem::path& parent)
{
    Fixture fixture{parent};
    ProfileStore store{fixture.path_};
    require(store.trySave({true, MissionId::secureRelay, true, {}}).has_value(), "Continuation fixture failed");
    GameJourney journey{fixture.path_};
    auto navigate = [&](NavigationAction action)
    {
        const auto view = journey.getFrontendView();
        require(journey.tryApplyIntent({view.screen_, view.transitionSerial_, action}),
            "Persistence navigation was rejected");
    };
    require(journey.getFrontendView().canContinue_ && journey.getFrontendView().cards_[1].enabled_,
        "Relaunch did not receive the saved continuation");
    navigate(NavigationAction::skipIntro);
    navigate(NavigationAction::continueRun);
    require(journey.getFrontendView().selectedMission_ == MissionId::secureRelay &&
        !journey.getSnapshot(), "Continue restored fabricated mid-mission state");
    navigate(NavigationAction::back);
    navigate(NavigationAction::showOptions);
    const auto goodBytes = readBytes(fixture.path_);
    require(std::filesystem::create_directory(fixture.pending_), "Save-failure fixture failed");
    auto view = journey.getFrontendView();
    require(journey.tryApplyIntent({view.screen_, view.transitionSerial_, SetReducedMotion{true}}),
        "Preference intent failed");
    require(journey.getFrontendView().screen_ == FrontendScreen::saveFailed &&
        !journey.getFrontendView().error_.empty() && readBytes(fixture.path_) == goodBytes &&
        !journey.tryPump(std::chrono::hours{1}), "Save failure fabricated success or advanced Runtime");
    std::filesystem::remove(fixture.pending_);
    navigate(NavigationAction::retrySave);
    require(journey.getFrontendView().screen_ == FrontendScreen::options &&
        journey.getFrontendView().error_.empty(), "Save retry did not restore the interrupted screen");
    GameJourney reloaded{fixture.path_};
    require(reloaded.getFrontendView().options_.reducedMotion_, "Preferences did not survive relaunch");
    writeBytes(fixture.path_, "CRUCIBLE_PROGRESS 9\n");
    view = journey.getFrontendView();
    require(journey.tryApplyIntent({view.screen_, view.transitionSerial_, SetReducedMotion{false}}),
        "Live-file refusal lost the correlated intent");
    require(journey.getFrontendView().screen_ == FrontendScreen::saveFailed &&
        readBytes(fixture.path_) == "CRUCIBLE_PROGRESS 9\n",
        "A live profile replacement bypassed unsupported-version refusal");
    GameJourney incompatible{fixture.path_};
    const auto startup = incompatible.getFrontendView();
    require(!startup.canContinue_ && !startup.error_.empty(), "Incompatible file fabricated continuation");
    require(incompatible.tryApplyIntent({startup.screen_, startup.transitionSerial_, NavigationAction::skipIntro}),
        "Invalid profile prevented menu navigation");
    view = incompatible.getFrontendView();
    require(incompatible.tryApplyIntent({view.screen_, view.transitionSerial_, NavigationAction::showOptions}),
        "Invalid profile prevented options navigation");
    view = incompatible.getFrontendView();
    require(incompatible.tryApplyIntent({view.screen_, view.transitionSerial_, SetReducedMotion{true}}),
        "Invalid profile preference intent failed");
    require(incompatible.getFrontendView().screen_ == FrontendScreen::saveFailed &&
        readBytes(fixture.path_) == "CRUCIBLE_PROGRESS 9\n", "Unsupported profile was silently overwritten");
    const auto failed = incompatible.getFrontendView();
    require(incompatible.tryApplyIntent({failed.screen_, failed.transitionSerial_,
        NavigationAction::continueWithoutSaving}), "Unsaved return was rejected");
}
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2) throw std::invalid_argument("Pass the owned build fixture directory");
        checkPersistence(argv[1]);
        checkJourneyFailures(argv[1]);
        std::cout << "Profile receiving: bounded format, replacement, refusal, relaunch and retry passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
