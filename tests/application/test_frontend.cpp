#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include "crucible/application/frontend.hpp"

namespace
{
using namespace crucible::application;

void require(bool condition, const char* reason)
{
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}

struct ContextDelete
{
    void operator()(ImGuiContext* context) const noexcept
    {
        ImGui::DestroyContext(context);
    }
};

class Fixture
{
public:
    std::unique_ptr<ImGuiContext, ContextDelete> context_{ImGui::CreateContext()};
    Frontend frontend_;
    std::size_t maximumVertices_{};

    explicit Fixture(const std::filesystem::path& fonts) : frontend_(*context_, fonts)
    {
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        io.DisplaySize = {1280, 864};
    }

    std::optional<FrontendIntent> frame(const FrontendView& view)
    {
        ImGui::GetIO().DeltaTime = 1.0F / 60;
        ImGui::NewFrame();
        auto intent = frontend_.draw(view);
        ImGui::Render();
        const auto* data = ImGui::GetDrawData();
        require(data && data->Valid, "real ImGui CPU draw data");
        maximumVertices_ = std::max(maximumVertices_, static_cast<std::size_t>(data->TotalVtxCount));
        for (const auto* list : data->CmdLists)
        {
            for (const auto& command : list->CmdBuffer)
            {
                require(command.UserCallback == nullptr, "no custom draw callbacks");
                require(std::isfinite(command.ClipRect.x) && std::isfinite(command.ClipRect.y)
                    && std::isfinite(command.ClipRect.z) && std::isfinite(command.ClipRect.w), "finite clipping");
                // ImGui's panel background uses viewport clipping, while controls use inner clipping.
                // Receive the effective triangle extent rather than rejecting a valid background command.
                for (unsigned element = 0; element < command.ElemCount; element += 3)
                {
                    ImVec2 low{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
                    ImVec2 high{-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
                    for (unsigned corner = 0; corner < 3; ++corner)
                    {
                        const auto index = command.VtxOffset + list->IdxBuffer[command.IdxOffset + element + corner];
                        require(index < static_cast<unsigned>(list->VtxBuffer.Size), "actual UI index range");
                        const auto position = list->VtxBuffer[static_cast<int>(index)].pos;
                        require(std::isfinite(position.x) && std::isfinite(position.y), "finite UI vertices");
                        low.x = std::min(low.x, position.x);
                        low.y = std::min(low.y, position.y);
                        high.x = std::max(high.x, position.x);
                        high.y = std::max(high.y, position.y);
                    }
                    low.x = std::max(low.x, command.ClipRect.x);
                    low.y = std::max(low.y, command.ClipRect.y);
                    high.x = std::min(high.x, command.ClipRect.z);
                    high.y = std::min(high.y, command.ClipRect.w);
                    if (low.x < high.x && low.y < high.y)
                    {
                        require(low.x >= view.safeContent_.x_ - 0.5F && low.y >= view.safeContent_.y_ - 0.5F
                            && high.x <= view.safeContent_.x_ + view.safeContent_.width_ + 0.5F
                            && high.y <= view.safeContent_.y_ + view.safeContent_.height_ + 0.5F,
                            "all visible UI triangles stay inside the actual safe rectangle");
                    }
                }
            }
        }
        // CPU-only texture protocol acknowledgement; never uploads or creates a GPU texture.
        if (data->Textures)
        {
            for (auto* texture : *data->Textures)
            {
                if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates)
                {
                    texture->SetTexID(1);
                    texture->SetStatus(ImTextureStatus_OK);
                }
                else if (texture->Status == ImTextureStatus_WantDestroy)
                {
                    texture->SetTexID(ImTextureID_Invalid);
                    texture->SetStatus(ImTextureStatus_Destroyed);
                }
            }
        }
        return intent;
    }

    void settle(const FrontendView& view)
    {
        for (int index = 0; index < 3; ++index)
        {
            require(!frame(view), "idle redraw produces no intention");
        }
    }

    FrontendIntent activate(const FrontendView& view)
    {
        settle(view);
        require(context_->NavId != 0, "default keyboard focus is usable without hover");
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, true);
        auto intent = frame(view);
        if (!intent)
        {
            std::fprintf(stderr, "activation diagnostic: screen=%u serial=%llu navId=%u navVisible=%d navActive=%d activeId=%u\n",
                static_cast<unsigned>(view.screen_), static_cast<unsigned long long>(view.transitionSerial_),
                context_->NavId, context_->NavCursorVisible, ImGui::GetIO().NavActive, context_->ActiveId);
        }
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, false);
        require(!frame(view), "released activation is not repeated");
        require(intent.has_value(), "keyboard activates the real focused control");
        require(intent->originScreen_ == view.screen_ && intent->transitionSerial_ == view.transitionSerial_,
            "intention is correlated to the exact view");
        return *intent;
    }

    FrontendIntent clickFocused(const FrontendView& view)
    {
        settle(view);
        const auto* window = context_->NavWindow;
        require(window != nullptr, "focused production panel");
        const auto rect = window->NavRectRel[0];
        require(rect.GetHeight() >= 47.5F, "focused target is at least48 logical pixels high");
        const ImVec2 center{window->Pos.x + (rect.Min.x + rect.Max.x) * 0.5F,
            window->Pos.y + (rect.Min.y + rect.Max.y) * 0.5F};
        ImGui::GetIO().AddMousePosEvent(center.x, center.y);
        ImGui::GetIO().AddMouseButtonEvent(0, true);
        require(!frame(view), "mouse press does not discard or prematurely apply release activation");
        ImGui::GetIO().AddMouseButtonEvent(0, false);
        const auto intent = frame(view);
        require(intent.has_value(), "pointer release activates the real visible control");
        return *intent;
    }

    void tab(const FrontendView& view)
    {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Tab, true);
        require(!frame(view), "Tab moves focus without activating a control");
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Tab, false);
        require(!frame(view), "Tab release does not activate a control");
    }

    void reject(FrontendView view)
    {
        ImGui::NewFrame();
        const auto old = ImGui::GetStyle().FramePadding;
        bool rejected{};
        try
        {
            (void)frontend_.draw(view);
        }
        catch (const std::invalid_argument&)
        {
            rejected = true;
        }
        require(rejected && ImGui::GetStyle().FramePadding.x == old.x
            && ImGui::GetStyle().FramePadding.y == old.y, "invalid view rejects before theme/widget mutation");
        ImGui::Render();
    }
};

void navigation(const FrontendIntent& intent, NavigationAction expected)
{
    const auto* action = std::get_if<NavigationAction>(&intent.action_);
    require(action && *action == expected, "expected typed navigation from actual input");
}
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "pass the bundled font directory");
        Fixture fixture{std::filesystem::path{argv[1]}};
        std::array cards{
            MissionCard{MissionId::reclaimFront, "Reclaim the front", "Recover biomass before the tick deadline.", true, {}},
            MissionCard{MissionId::secureRelay, "Secure the relay", "Recover biomass and assemble the relay.", false,
                "Complete the first mission to unlock this objective."}};
        FrontendView view;
        view.cards_ = cards;
        navigation(fixture.activate(view), NavigationAction::skipIntro);
        view.screen_ = FrontendScreen::menu;
        view.transitionSerial_ = 9;
        view.canContinue_ = true;
        navigation(fixture.activate(view), NavigationAction::continueRun);
        // Re-entering the same screen receives a new default focus; disabled Continue is skipped.
        view.transitionSerial_ = 10;
        view.canContinue_ = false;
        view.continueReason_ = "No committed mission-boundary progress.";
        navigation(fixture.activate(view), NavigationAction::newRun);
        view.screen_ = FrontendScreen::missions;
        view.transitionSerial_ = 11;
        const auto selected = fixture.activate(view);
        const auto* mission = std::get_if<SelectMission>(&selected.action_);
        require(mission && mission->id_ == MissionId::reclaimFront, "mission card returns stable typed ID");
        view.screen_ = FrontendScreen::briefing;
        navigation(fixture.activate(view), NavigationAction::beginMission);
        view.selectedMission_ = MissionId::secureRelay;
        ++view.transitionSerial_;
        navigation(fixture.activate(view), NavigationAction::back);
        view.selectedMission_ = MissionId::reclaimFront;
        view.screen_ = FrontendScreen::playing;
        navigation(fixture.activate(view), NavigationAction::pause);
        view.screen_ = FrontendScreen::paused;
        navigation(fixture.activate(view), NavigationAction::resume);
        view.screen_ = FrontendScreen::saveFailed;
        view.error_ = "Storage is busy. The last committed profile is preserved.";
        navigation(fixture.activate(view), NavigationAction::retrySave);
        fixture.tab(view);
        navigation(fixture.activate(view), NavigationAction::continueWithoutSaving);
        view.screen_ = FrontendScreen::quitConfirmation;
        navigation(fixture.activate(view), NavigationAction::cancelQuit);
        fixture.tab(view);
        navigation(fixture.activate(view), NavigationAction::confirmQuit);
        view.error_ = {};
        view.screen_ = FrontendScreen::results;
        view.progress_ = crucible::ReclamationMissionProgress{};
        view.progress_->outcome = crucible::ReclamationMissionOutcome::lost;
        navigation(fixture.activate(view), NavigationAction::retryMission);
        cards[1].enabled_ = true;
        view.progress_->outcome = crucible::ReclamationMissionOutcome::won;
        ++view.transitionSerial_;
        navigation(fixture.activate(view), NavigationAction::nextMission);
        view.screen_ = FrontendScreen::options;
        const auto option = fixture.activate(view);
        const auto* reduced = std::get_if<SetReducedMotion>(&option.action_);
        require(reduced && reduced->value_ && !view.options_.reducedMotion_, "preference intent never mutates the borrowed view");
        const auto focused = fixture.context_->NavId;
        view.options_.reducedMotion_ = true;
        ++view.transitionSerial_;
        fixture.settle(view);
        require(fixture.context_->NavId == focused, "options focus remains stable across applied preferences");
        fixture.tab(view);
        const auto fullscreen = fixture.activate(view);
        const auto* windowMode = std::get_if<SetFullscreen>(&fullscreen.action_);
        require(windowMode && windowMode->value_ && !view.options_.fullscreen_, "fullscreen has its own typed intent");
        fixture.tab(view);
        fixture.tab(view);
        const auto textSize = fixture.activate(view);
        const auto* scaleChoice = std::get_if<SetTextScale>(&textSize.action_);
        require(scaleChoice && scaleChoice->value_ == TextScale::large, "text size uses its exact finite variant");

        // Real logical dimensions, density and text-size matrix; CPU geometry is not native input/rendering proof.
        for (const auto display : {ImVec2{320, 480}, ImVec2{480, 320}, ImVec2{1280, 864}})
        {
            ImGui::GetIO().DisplaySize = display;
            view.safeContent_ = {0, 0, display.x, display.y};
            for (const float density : {0.5F, 1.0F, 2.0F, 4.0F})
            {
                ImGui::GetIO().DisplayFramebufferScale = {density, density};
                for (const auto scale : {TextScale::standard, TextScale::large, TextScale::extraLarge})
                {
                    view.options_.textScale_ = scale;
                    for (int screen = 0; screen <= static_cast<int>(FrontendScreen::quitConfirmation); ++screen)
                    {
                        view.screen_ = static_cast<FrontendScreen>(screen);
                        ++view.transitionSerial_;
                        fixture.settle(view);
                    }
                }
            }
        }
        ImGui::GetIO().DisplaySize = {1280, 864};
        ImGui::GetIO().DisplayFramebufferScale = {1, 1};
        view.safeContent_ = {20, 30, 1240, 800};
        view.options_.textScale_ = TextScale::standard;
        view.screen_ = FrontendScreen::menu;
        view.canContinue_ = true;
        ++view.transitionSerial_;
        navigation(fixture.clickFocused(view), NavigationAction::continueRun);
        // Longest admitted labels receive actual wrapped/clipped controls, including disabled reasons.
        const std::string title(96, 'W'), description(256, 'x'), message(512, 'M');
        for (auto& card : cards)
        {
            card.title_ = title;
            card.objective_ = description;
            card.disabledReason_ = description;
            card.enabled_ = false;
        }
        view.status_ = message;
        view.error_ = message;
        view.continueReason_ = description;
        view.canContinue_ = false;
        view.screen_ = FrontendScreen::missions;
        ++view.transitionSerial_;
        fixture.settle(view);
        navigation(fixture.activate(view), NavigationAction::back);
        view.screen_ = FrontendScreen::playing;
        ++view.transitionSerial_;
        view.status_ = {};
        view.error_ = {};
        ImGui::GetIO().AddMousePosEvent(1100, 700);
        fixture.settle(view);
        require(!ImGui::GetIO().WantCaptureMouse, "compact HUD leaves gameplay pointer space available");
        auto bad = view;
        bad.status_ = std::string_view{"bad\0label", 9};
        fixture.reject(bad);
        bad.status_ = "\xF0\x80\x80\x80";
        fixture.reject(bad);
        const std::string tooLong(513, 'x');
        bad.status_ = tooLong;
        fixture.reject(bad);
        bad = view;
        bad.safeContent_.width_ = std::numeric_limits<float>::infinity();
        fixture.reject(bad);
        bad = view;
        bad.transitionSerial_ = 0;
        fixture.reject(bad);
        cards[1].id_ = cards[0].id_;
        fixture.reject(view);
        std::printf("frontend CPU behavior PASS: all10 screens, typed keyboard/pointer actions, safe-area/text/DPI matrix; observed max vertices=%zu (no GPU/native latency claim)\n", fixture.maximumVertices_);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
