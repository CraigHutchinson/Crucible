#include <limits>
#include <stdexcept>
#include <type_traits>

#include "crucible/application/game_journey.hpp"
#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "crucible/runtime/InspectorSession.hpp"

namespace crucible::application
{
GameJourney::GameJourney(std::optional<std::filesystem::path> profilePath)
    : cards_{{
        {MissionId::reclaimFront, "Reclaim the Front",
            "Reclaim 1780 biomass within 900 completed ticks.", true, {}},
        {MissionId::secureRelay, "Secure the Relay",
            "Reclaim 1780 biomass and hold a relay for 120 ticks.", false,
            "Complete Reclaim the Front to unlock this mission."}}}
{
    if (!profilePath) return;
    profileStore_.emplace(std::move(*profilePath));
    const auto loaded = profileStore_->tryLoad();
    if (!loaded)
    {
        profileError_ = loaded.error();
        loadRefused_ = true;
        return;
    }
    if (!*loaded) return;
    const auto& profile = **loaded;
    if (profile.hasContinuation_) continuation_ = profile.nextMission_;
    options_ = profile.options_;
    cards_[1].enabled_ = profile.relayUnlocked_;
    if (profile.relayUnlocked_) cards_[1].disabledReason_ = {};
}

GameJourney::~GameJourney() = default;

void GameJourney::transitionTo(FrontendScreen screen) noexcept
{
    screen_ = screen;
    ++transitionSerial_;
}

void GameJourney::saveProfileFor(FrontendScreen returnScreen)
{
    saveReturnScreen_ = returnScreen;
    if (!profileStore_)
    {
        transitionTo(returnScreen);
        return;
    }
    if (loadRefused_)
    {
        const auto loaded = profileStore_->tryLoad();
        if (!loaded)
        {
            profileError_ = loaded.error();
            unsavedProfile_ = true;
            transitionTo(FrontendScreen::saveFailed);
            return;
        }
        if (*loaded)
        {
            const auto& previous = **loaded;
            cards_[1].enabled_ = cards_[1].enabled_ || previous.relayUnlocked_;
            if (cards_[1].enabled_) cards_[1].disabledReason_ = {};
            if (!continuation_ && previous.hasContinuation_) continuation_ = previous.nextMission_;
        }
        loadRefused_ = false;
    }
    const ProgressProfile profile{continuation_.has_value(),
        continuation_.value_or(MissionId::reclaimFront), cards_[1].enabled_, options_};
    const auto saved = profileStore_->trySave(profile);
    unsavedProfile_ = !saved;
    profileError_ = saved ? std::nullopt : std::optional{saved.error()};
    if (!saved && (saved.error() == ProfileStore::Error::malformed ||
        saved.error() == ProfileStore::Error::incompatible)) loadRefused_ = true;
    transitionTo(saved ? returnScreen : FrontendScreen::saveFailed);
}

bool GameJourney::tryBeginMission()
{
    if (attemptId_ == std::numeric_limits<std::uint64_t>::max()) return false;
    const bool relay = selectedMission_ == MissionId::secureRelay;
    if (relay && !cards_[1].enabled_) return false;
    auto replacement = std::make_unique<runtime::InspectorSession>(2048,
        runtime::HeadlessSession::Limits{64, 4096}, ReclamationMissionSettings{}, relay);
    if (session_) session_->Close();
    session_.swap(replacement);
    ++attemptId_;
    transitionTo(FrontendScreen::playing);
    return true;
}

bool GameJourney::tryNavigate(NavigationAction action)
{
    switch (action)
    {
    case NavigationAction::skipIntro:
        if (screen_ != FrontendScreen::intro) return false;
        transitionTo(FrontendScreen::menu);
        return true;
    case NavigationAction::newRun:
        if (screen_ != FrontendScreen::menu) return false;
        selectedMission_ = MissionId::reclaimFront;
        transitionTo(FrontendScreen::briefing);
        return true;
    case NavigationAction::continueRun:
        if (screen_ != FrontendScreen::menu || !continuation_) return false;
        selectedMission_ = *continuation_;
        transitionTo(FrontendScreen::briefing);
        return true;
    case NavigationAction::showMissions:
        if (screen_ != FrontendScreen::menu) return false;
        transitionTo(FrontendScreen::missions);
        return true;
    case NavigationAction::showOptions:
        if (screen_ != FrontendScreen::menu && screen_ != FrontendScreen::paused) return false;
        returnScreen_ = screen_;
        transitionTo(FrontendScreen::options);
        return true;
    case NavigationAction::back:
        if (screen_ == FrontendScreen::paused)
        {
            session_->Close();
            session_.reset();
            transitionTo(FrontendScreen::menu);
            return true;
        }
        if (screen_ == FrontendScreen::options)
        {
            transitionTo(returnScreen_);
            return true;
        }
        if (screen_ == FrontendScreen::missions || screen_ == FrontendScreen::briefing ||
            screen_ == FrontendScreen::results)
        {
            transitionTo(FrontendScreen::menu);
            return true;
        }
        return false;
    case NavigationAction::beginMission:
        return screen_ == FrontendScreen::briefing && tryBeginMission();
    case NavigationAction::retryMission:
        return (screen_ == FrontendScreen::results || screen_ == FrontendScreen::paused) &&
            tryBeginMission();
    case NavigationAction::pause:
        if (screen_ != FrontendScreen::playing) return false;
        session_->Pause();
        transitionTo(FrontendScreen::paused);
        return true;
    case NavigationAction::resume:
        if (screen_ != FrontendScreen::paused) return false;
        session_->Resume();
        transitionTo(FrontendScreen::playing);
        return true;
    case NavigationAction::nextMission:
        if (screen_ != FrontendScreen::results || selectedMission_ != MissionId::reclaimFront ||
            !session_->GetMission() ||
            session_->GetMission()->outcome != ReclamationMissionOutcome::won) return false;
        selectedMission_ = MissionId::secureRelay;
        transitionTo(FrontendScreen::briefing);
        return true;
    case NavigationAction::replayIntro:
        if (screen_ != FrontendScreen::menu) return false;
        transitionTo(FrontendScreen::intro);
        return true;
    case NavigationAction::requestQuit:
        if (screen_ == FrontendScreen::quitConfirmation) return false;
        quitReturnScreen_ = screen_;
        if (screen_ == FrontendScreen::playing) session_->Pause();
        transitionTo(FrontendScreen::quitConfirmation);
        return true;
    case NavigationAction::cancelQuit:
        if (screen_ != FrontendScreen::quitConfirmation) return false;
        if (quitReturnScreen_ == FrontendScreen::playing) session_->Resume();
        transitionTo(quitReturnScreen_);
        return true;
    case NavigationAction::confirmQuit:
        if (screen_ != FrontendScreen::quitConfirmation) return false;
        if (session_) session_->Close();
        quitRequested_ = true;
        ++transitionSerial_;
        return true;
    case NavigationAction::retrySave:
        if (screen_ != FrontendScreen::saveFailed) return false;
        saveProfileFor(saveReturnScreen_);
        return true;
    case NavigationAction::continueWithoutSaving:
        if (screen_ != FrontendScreen::saveFailed) return false;
        transitionTo(saveReturnScreen_);
        return true;
    }
    return false;
}

bool GameJourney::tryApplyIntent(const FrontendIntent& intent)
{
    if (quitRequested_ || intent.originScreen_ != screen_ ||
        intent.transitionSerial_ != transitionSerial_ ||
        transitionSerial_ == std::numeric_limits<std::uint64_t>::max()) return false;
    return std::visit([this](const auto& action)
    {
        using Action = std::decay_t<decltype(action)>;
        if constexpr (std::is_same_v<Action, NavigationAction>) return tryNavigate(action);
        else if constexpr (std::is_same_v<Action, SelectMission>)
        {
            if (screen_ != FrontendScreen::missions && screen_ != FrontendScreen::briefing) return false;
            if (action.id_ != MissionId::reclaimFront && action.id_ != MissionId::secureRelay) return false;
            if (action.id_ == MissionId::secureRelay && !cards_[1].enabled_) return false;
            selectedMission_ = action.id_;
            transitionTo(FrontendScreen::briefing);
            return true;
        }
        else
        {
            static_assert(std::is_same_v<Action, SetReducedMotion> ||
                std::is_same_v<Action, SetFullscreen> || std::is_same_v<Action, SetTextScale>);
            if (screen_ != FrontendScreen::options) return false;
            if constexpr (std::is_same_v<Action, SetReducedMotion>) options_.reducedMotion_ = action.value_;
            else if constexpr (std::is_same_v<Action, SetFullscreen>) options_.fullscreen_ = action.value_;
            else if constexpr (std::is_same_v<Action, SetTextScale>)
            {
                if (action.value_ != TextScale::standard && action.value_ != TextScale::large &&
                    action.value_ != TextScale::extraLarge) return false;
                options_.textScale_ = action.value_;
            }
            saveProfileFor(FrontendScreen::options);
            return true;
        }
    }, intent.action_);
}

std::optional<runtime::ClockDriver::PumpResult> GameJourney::tryPump(std::chrono::nanoseconds elapsed)
{
    if (screen_ != FrontendScreen::playing || quitRequested_) return std::nullopt;
    auto result = session_->TryPump(elapsed);
    const auto progress = session_->GetMission();
    if (progress && progress->outcome != ReclamationMissionOutcome::active)
    {
        if (transitionSerial_ == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("Journey transition identity exhausted");
        if (selectedMission_ == MissionId::reclaimFront &&
            progress->outcome == ReclamationMissionOutcome::won)
        {
            cards_[1].enabled_ = true;
            cards_[1].disabledReason_ = {};
            continuation_ = MissionId::secureRelay;
        }
        else continuation_ = selectedMission_;
        saveProfileFor(FrontendScreen::results);
    }
    return result;
}

std::optional<runtime::CommandIngress::Admission> GameJourney::tryAdmitFieldEdit(const FieldEdit& edit)
{
    if (screen_ != FrontendScreen::playing || quitRequested_) return std::nullopt;
    return session_->TryAdmitFieldEdit(edit);
}

FrontendView GameJourney::getFrontendView(SafeContentRect safeContent) const noexcept
{
    FrontendView view;
    view.screen_ = screen_;
    view.transitionSerial_ = transitionSerial_;
    view.selectedMission_ = selectedMission_;
    view.cards_ = cards_;
    view.canContinue_ = continuation_.has_value();
    view.continueReason_ = continuation_ ? "Start the continuation mission from its briefing." :
        "Complete a mission to establish a continuation point.";
    view.options_ = options_;
    view.progress_ = session_ ? session_->GetMission() : std::nullopt;
    view.error_ = profileError_ ? ProfileStore::getErrorText(*profileError_) : std::string_view{};
    view.status_ = !profileStore_ ? "Mission-boundary progress is retained in memory only." :
        unsavedProfile_ ? "Current progress has not been saved. The previous file was retained." :
        "Continue opens a fresh mission from saved mission-boundary progress.";
    if (screen_ == FrontendScreen::paused)
        view.status_ = "Paused. Returning to the menu ends this attempt; only mission-boundary progress is saved.";
    view.safeContent_ = safeContent;
    return view;
}

const presentation::ScenarioSnapshot* GameJourney::getSnapshot() const noexcept
{
    return session_ ? &session_->GetSnapshot() : nullptr;
}
}
