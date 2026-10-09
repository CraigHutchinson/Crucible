#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <imgui.h>
#include <stdexcept>
#include <vector>

#include "crucible/application/frontend.hpp"

namespace crucible::application
{
namespace
{
constexpr std::size_t maxFontBytes = 2 * 1024 * 1024;
constexpr ImVec4 silver{0.79F, 0.86F, 0.91F, 1};
constexpr ImVec4 cyan{0.22F, 0.85F, 0.94F, 1};
constexpr ImVec4 muted{0.52F, 0.64F, 0.72F, 1};
constexpr ImVec4 amber{0.98F, 0.69F, 0.32F, 1};
constexpr ImVec4 failure{0.98F, 0.39F, 0.58F, 1};

bool isValidLabel(std::string_view text, std::size_t bound) noexcept
{
    if (text.size() > bound)
    {
        return false;
    }
    for (std::size_t offset = 0; offset < text.size();)
    {
        const auto first = static_cast<unsigned char>(text[offset++]);
        if (first == 0)
        {
            return false;
        }
        if (first < 0x80)
        {
            continue;
        }
        const unsigned trailing = first >= 0xC2 && first <= 0xDF ? 1U
            : first >= 0xE0 && first <= 0xEF ? 2U : first >= 0xF0 && first <= 0xF4 ? 3U : 0U;
        if (!trailing || text.size() - offset < trailing)
        {
            return false;
        }
        unsigned codepoint = first & (0x7FU >> trailing);
        for (unsigned index = 0; index < trailing; ++index)
        {
            const auto next = static_cast<unsigned char>(text[offset++]);
            if ((next & 0xC0U) != 0x80U)
            {
                return false;
            }
            codepoint = (codepoint << 6) | (next & 0x3FU);
        }
        const unsigned minimum = trailing == 1 ? 0x80U : trailing == 2 ? 0x800U : 0x10000U;
        if (codepoint < minimum || codepoint > 0x10FFFFU || (codepoint >= 0xD800U && codepoint <= 0xDFFFU))
        {
            return false;
        }
    }
    return true;
}

constexpr bool isValidMission(MissionId id) noexcept
{
    return id == MissionId::reclaimFront || id == MissionId::secureRelay;
}

constexpr float textMultiplier(TextScale scale) noexcept
{
    switch (scale)
    {
    case TextScale::standard: return 1.0F;
    case TextScale::large: return 1.25F;
    case TextScale::extraLarge: return 1.5F;
    }
    return 0;
}

void validateView(const FrontendView& view)
{
    const auto& io = ImGui::GetIO();
    const auto& safe = view.safeContent_;
    if (static_cast<unsigned>(view.screen_) > static_cast<unsigned>(FrontendScreen::quitConfirmation)
        || !view.transitionSerial_ || !isValidMission(view.selectedMission_) || !textMultiplier(view.options_.textScale_)
        || view.cards_.size() > 2 || !isValidLabel(view.continueReason_, 256)
        || !isValidLabel(view.status_, 512) || !isValidLabel(view.error_, 512)
        || !std::isfinite(safe.x_) || !std::isfinite(safe.y_) || !std::isfinite(safe.width_) || !std::isfinite(safe.height_)
        || safe.x_ < 0 || safe.y_ < 0 || std::min(safe.width_, safe.height_) < 320
        || std::max(safe.width_, safe.height_) < 480
        || !std::isfinite(io.DisplaySize.x) || !std::isfinite(io.DisplaySize.y)
        || safe.x_ + safe.width_ > io.DisplaySize.x + 0.5F || safe.y_ + safe.height_ > io.DisplaySize.y + 0.5F
        || !std::isfinite(io.DisplayFramebufferScale.x) || !std::isfinite(io.DisplayFramebufferScale.y)
        || io.DisplayFramebufferScale.x < 0.5F || io.DisplayFramebufferScale.x > 4.0F
        || io.DisplayFramebufferScale.y < 0.5F || io.DisplayFramebufferScale.y > 4.0F)
    {
        throw std::invalid_argument("Frontend view exceeds its screen, text or safe-content contract");
    }
    for (const auto& card : view.cards_)
    {
        if (!isValidMission(card.id_) || !isValidLabel(card.title_, 96)
            || !isValidLabel(card.objective_, 256) || !isValidLabel(card.disabledReason_, 256))
        {
            throw std::invalid_argument("Frontend mission card is invalid");
        }
    }
    if (view.cards_.size() == 2 && view.cards_[0].id_ == view.cards_[1].id_)
    {
        throw std::invalid_argument("Frontend mission IDs must be distinct");
    }
    if (view.progress_ && static_cast<unsigned>(view.progress_->outcome) > static_cast<unsigned>(ReclamationMissionOutcome::lost))
    {
        throw std::invalid_argument("Frontend mission outcome is invalid");
    }
}

std::vector<char> readFont(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    const auto size = input.tellg();
    if (!input || size <= 100 || size > static_cast<std::streamoff>(maxFontBytes))
    {
        throw std::runtime_error("Production font is missing or exceeds its 2MiB bound");
    }
    std::vector<char> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    if (!input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))
        || bytes[0] != 0 || bytes[1] != 1 || bytes[2] != 0 || bytes[3] != 0)
    {
        throw std::runtime_error("Production font is not the required bundled TrueType asset");
    }
    return bytes;
}

ImFont* addFont(const std::vector<char>& bytes, std::string_view name)
{
    std::unique_ptr<void, void(*)(void*)> owned(ImGui::MemAlloc(bytes.size()), ImGui::MemFree);
    if (!owned)
    {
        throw std::bad_alloc{};
    }
    std::memcpy(owned.get(), bytes.data(), bytes.size());
    ImFontConfig config;
    config.FontDataOwnedByAtlas = true;
    const auto copied = std::min(name.size(), sizeof(config.Name) - 1);
    std::copy_n(name.begin(), copied, config.Name);
    config.Name[copied] = 0;
    auto* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(owned.get(), static_cast<int>(bytes.size()), 18, &config);
    owned.release();
    if (!font)
    {
        throw std::runtime_error("Production font atlas registration failed");
    }
    return font;
}

void text(std::string_view value)
{
    if (!value.empty())
    {
        ImGui::TextUnformatted(value.data(), value.data() + value.size());
    }
}

constexpr ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
}

struct Frontend::State
{
    ImGuiContext& context_;
    ImFont* body_{}; // non-owning; the root context's atlas owns the font.
    ImFont* display_{}; // non-owning; the root context's atlas owns the font.
    ImGuiStyle baseStyle_{};
    std::optional<FrontendScreen> previousScreen_{};
    std::uint64_t previousTransition_{};
    std::array<float, 5> previousLayout_{};
    ImGuiID focusedItem_{};
    float multiplier_{1};
    bool defaultFocus_{};
    bool ensureVisibility_{};

    State(ImGuiContext& context, const std::filesystem::path& directory) : context_(context)
    {
        if (ImGui::GetCurrentContext() != &context_ || ImGui::GetIO().Fonts->Locked)
        {
            throw std::invalid_argument("Frontend construction requires its current context and a mutable atlas");
        }
        const auto body = readFont(directory / "source-sans" / "SourceSans3-Regular.ttf");
        const auto display = readFont(directory / "rajdhani" / "Rajdhani-SemiBold.ttf");
        body_ = addFont(body, "Source Sans 3 / body");
        display_ = addFont(display, "Rajdhani / display");
        ImGui::StyleColorsDark(&baseStyle_);
        baseStyle_.WindowPadding = {22, 22};
        baseStyle_.FramePadding = {15, 15};
        baseStyle_.ItemSpacing = {12, 12};
        baseStyle_.WindowRounding = 8;
        baseStyle_.FrameRounding = 5;
        baseStyle_.GrabRounding = 4;
        baseStyle_.WindowBorderSize = 1;
        baseStyle_.FrameBorderSize = 1;
        baseStyle_.ScrollbarSize = 12;
        baseStyle_.FontSizeBase = 18;
        baseStyle_.Colors[ImGuiCol_Text] = silver;
        baseStyle_.Colors[ImGuiCol_TextDisabled] = muted;
        baseStyle_.Colors[ImGuiCol_WindowBg] = {0.025F, 0.045F, 0.065F, 0.94F};
        baseStyle_.Colors[ImGuiCol_Border] = {0.21F, 0.32F, 0.39F, 0.75F};
        baseStyle_.Colors[ImGuiCol_FrameBg] = {0.08F, 0.13F, 0.17F, 1};
        baseStyle_.Colors[ImGuiCol_FrameBgHovered] = {0.12F, 0.23F, 0.29F, 1};
        baseStyle_.Colors[ImGuiCol_FrameBgActive] = {0.15F, 0.31F, 0.37F, 1};
        baseStyle_.Colors[ImGuiCol_Button] = {0.075F, 0.14F, 0.18F, 1};
        baseStyle_.Colors[ImGuiCol_ButtonHovered] = {0.13F, 0.30F, 0.36F, 1};
        baseStyle_.Colors[ImGuiCol_ButtonActive] = {0.18F, 0.39F, 0.43F, 1};
        baseStyle_.Colors[ImGuiCol_CheckMark] = cyan;
        baseStyle_.Colors[ImGuiCol_SliderGrab] = cyan;
        baseStyle_.Colors[ImGuiCol_SliderGrabActive] = silver;
        baseStyle_.Colors[ImGuiCol_Header] = {0.09F, 0.20F, 0.25F, 1};
        baseStyle_.Colors[ImGuiCol_HeaderHovered] = {0.14F, 0.31F, 0.37F, 1};
        baseStyle_.Colors[ImGuiCol_HeaderActive] = {0.18F, 0.36F, 0.41F, 1};
        baseStyle_.Colors[ImGuiCol_NavCursor] = cyan;
        ImGui::GetStyle() = baseStyle_;
    }

    void heading(std::string_view value)
    {
        ImGui::PushFont(display_, 32 * multiplier_);
        text(value);
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void caption(std::string_view value, const ImVec4& color = muted)
    {
        ImGui::PushFont(body_, 14 * multiplier_);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        text(value);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    void focus(bool enabled)
    {
        if (enabled && defaultFocus_)
        {
            ImGui::SetKeyboardFocusHere(-1);
            ImGui::SetNavCursorVisible(true);
            defaultFocus_ = false;
        }
        if (ImGui::IsItemFocused())
        {
            const auto id = ImGui::GetItemID();
            if (ensureVisibility_ || focusedItem_ != id)
            {
                ImGui::SetScrollHereY(0.5F);
            }
            focusedItem_ = id;
        }
    }

    void button(const char* label, NavigationAction action, const FrontendView& view,
        std::optional<FrontendIntent>& intent, bool enabled = true, std::string_view reason = {})
    {
        ImGui::PushID(static_cast<int>(action));
        ImGui::BeginDisabled(!enabled);
        ImGui::PushFont(display_, 22 * multiplier_);
        if (ImGui::Button(label, {-1, 48 * multiplier_}) && !intent)
        {
            intent = FrontendIntent{view.screen_, view.transitionSerial_, action};
        }
        focus(enabled);
        ImGui::PopFont();
        ImGui::EndDisabled();
        ImGui::PopID();
        if (!enabled && !reason.empty())
        {
            caption(reason);
        }
    }

    void drawCards(const FrontendView& view, std::optional<FrontendIntent>& intent)
    {
        for (const auto& card : view.cards_)
        {
            ImGui::PushID(static_cast<int>(card.id_));
            ImGui::PushFont(display_, 22 * multiplier_);
            ImGui::PushStyleColor(ImGuiCol_Text, card.id_ == view.selectedMission_ ? cyan : silver);
            text(card.title_);
            ImGui::PopStyleColor();
            ImGui::PopFont();
            text(card.objective_);
            ImGui::BeginDisabled(!card.enabled_);
            if (ImGui::Button("View briefing", {-1, 48 * multiplier_}) && !intent)
            {
                intent = FrontendIntent{view.screen_, view.transitionSerial_, SelectMission{card.id_}};
            }
            focus(card.enabled_);
            ImGui::EndDisabled();
            if (!card.enabled_)
            {
                caption(card.disabledReason_, amber);
            }
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::PopID();
        }
    }

    void drawOptions(const FrontendView& view, std::optional<FrontendIntent>& intent)
    {
        bool reduced = view.options_.reducedMotion_;
        if (ImGui::Checkbox("Reduced motion", &reduced) && !intent)
        {
            intent = FrontendIntent{view.screen_, view.transitionSerial_, SetReducedMotion{reduced}};
        }
        focus(true);
        caption("Use the settled title composition and quiet transitions.");
        bool fullscreen = view.options_.fullscreen_;
        if (ImGui::Checkbox("Fullscreen", &fullscreen) && !intent)
        {
            intent = FrontendIntent{view.screen_, view.transitionSerial_, SetFullscreen{fullscreen}};
        }
        focus(true);
        caption("Text size");
        constexpr std::array<const char*, 3> labels{"Standard", "Large", "Extra large"};
        for (std::size_t index = 0; index < labels.size(); ++index)
        {
            const auto scale = static_cast<TextScale>(index);
            if (ImGui::RadioButton(labels[index], view.options_.textScale_ == scale) && !intent)
            {
                intent = FrontendIntent{view.screen_, view.transitionSerial_, SetTextScale{scale}};
            }
            focus(true);
        }
        caption("Changes apply without advancing a paused mission.");
        button("Back", NavigationAction::back, view, intent);
    }

    std::optional<FrontendIntent> draw(const FrontendView& view)
    {
        if (ImGui::GetCurrentContext() != &context_)
        {
            throw std::invalid_argument("Frontend draw requires its original current context");
        }
        validateView(view);
        multiplier_ = textMultiplier(view.options_.textScale_);
        auto style = baseStyle_;
        style.ScaleAllSizes(multiplier_);
        style.FontScaleMain = 1;
        style.FontScaleDpi = 1;
        ImGui::GetStyle() = style;
        defaultFocus_ = !previousScreen_ || *previousScreen_ != view.screen_
            || (previousTransition_ != view.transitionSerial_ && view.screen_ != FrontendScreen::options);
        previousScreen_ = view.screen_;
        previousTransition_ = view.transitionSerial_;
        std::optional<FrontendIntent> intent;
        const auto& safe = view.safeContent_;
        const std::array layout{safe.x_, safe.y_, safe.width_, safe.height_, multiplier_};
        ensureVisibility_ = previousLayout_ != layout || defaultFocus_;
        previousLayout_ = layout;
        const bool overlay = view.screen_ == FrontendScreen::intro || view.screen_ == FrontendScreen::playing;
        const bool wide = safe.width_ / safe.height_ >= 1.2F;
        const float margin = 16;
        const float top = overlay ? margin : wide ? margin : safe.height_ * 0.38F;
        const float width = overlay ? safe.width_ - 2 * margin
            : wide ? std::clamp(safe.width_ * 0.32F, 280.0F, 440.0F) : safe.width_ - 2 * margin;
        const bool playing = view.screen_ == FrontendScreen::playing;
        const float panelWidth = playing ? std::min(width, 480.0F) : width;
        const float panelHeight = playing ? std::min(safe.height_ - top - margin, 144.0F * multiplier_)
            : safe.height_ - top - margin;
        ImGui::SetNextWindowPos({safe.x_ + margin, safe.y_ + top});
        ImGui::SetNextWindowSize({panelWidth, panelHeight});
        ImGui::SetNextWindowBgAlpha(overlay ? 0.0F : 0.94F);
        ImGui::PushFont(body_, 18 * multiplier_);
        const bool visible = ImGui::Begin("##Crucible production controls", nullptr, panelFlags);
        if (!visible)
        {
            ImGui::End();
            ImGui::PopFont();
            return std::nullopt;
        }
        ImGui::PushID(static_cast<int>(view.screen_));
        ImGui::PushTextWrapPos(0);
        if (!overlay)
        {
            auto* draw = ImGui::GetWindowDrawList();
            const auto position = ImGui::GetWindowPos();
            draw->AddLine({position.x + 22, position.y + 2}, {position.x + width - 22, position.y + 2},
                ImGui::GetColorU32(cyan), 2);
        }
        switch (view.screen_)
        {
        case FrontendScreen::intro:
            caption("NANITE SYNCHRONIZATION", silver);
            ImGui::Dummy({0, std::max(0.0F, ImGui::GetContentRegionAvail().y - 112 * multiplier_)});
            caption("Escape or Skip opens the menu immediately.");
            button("Skip intro", NavigationAction::skipIntro, view, intent);
            break;
        case FrontendScreen::menu:
            heading("Operations");
            caption("MISSION-BOUNDARY PROGRESS", cyan);
            button("Continue", NavigationAction::continueRun, view, intent, view.canContinue_, view.continueReason_);
            button("New run", NavigationAction::newRun, view, intent);
            button("Missions", NavigationAction::showMissions, view, intent);
            button("Options", NavigationAction::showOptions, view, intent);
            button("Replay intro", NavigationAction::replayIntro, view, intent);
            button("Quit", NavigationAction::requestQuit, view, intent);
            break;
        case FrontendScreen::missions:
            heading("Mission selection");
            caption("Choose an objective, then review its briefing.");
            drawCards(view, intent);
            button("Back", NavigationAction::back, view, intent);
            break;
        case FrontendScreen::briefing:
        {
            const auto selected = std::find_if(view.cards_.begin(), view.cards_.end(),
                [&](const MissionCard& card) { return card.id_ == view.selectedMission_; });
            heading("Mission briefing");
            if (selected != view.cards_.end())
            {
                text(selected->title_);
                text(selected->objective_);
                caption("Guide the collective with fields. Biomass remains conserved.");
            }
            const bool enabled = selected != view.cards_.end() && selected->enabled_;
            button("Begin mission", NavigationAction::beginMission, view, intent, enabled,
                selected != view.cards_.end() ? selected->disabledReason_ : "Mission unavailable.");
            button("Back", NavigationAction::back, view, intent);
            break;
        }
        case FrontendScreen::playing:
            if (view.progress_)
            {
                ImGui::Text("Recovered %llu / %llu   |   Tick %llu / %llu",
                    static_cast<unsigned long long>(view.progress_->reclaimed),
                    static_cast<unsigned long long>(view.progress_->settings.target_reclaimed),
                    static_cast<unsigned long long>(view.progress_->completed_tick),
                    static_cast<unsigned long long>(view.progress_->settings.deadline_ticks));
            }
            button("Pause", NavigationAction::pause, view, intent);
            break;
        case FrontendScreen::paused:
            heading("Mission paused");
            text("No simulation ticks advance while this screen is open.");
            button("Resume", NavigationAction::resume, view, intent);
            button("Options", NavigationAction::showOptions, view, intent);
            button("Return to menu", NavigationAction::back, view, intent);
            button("Quit", NavigationAction::requestQuit, view, intent);
            break;
        case FrontendScreen::options:
            heading("Options");
            drawOptions(view, intent);
            break;
        case FrontendScreen::results:
        {
            const bool won = view.progress_ && view.progress_->outcome == ReclamationMissionOutcome::won;
            const bool terminal = view.progress_ && view.progress_->outcome != ReclamationMissionOutcome::active;
            heading(won ? "Mission complete" : terminal ? "Mission failed" : "Outcome unavailable");
            if (view.progress_)
            {
                ImGui::Text("Recovered %llu / %llu", static_cast<unsigned long long>(view.progress_->reclaimed),
                    static_cast<unsigned long long>(view.progress_->settings.target_reclaimed));
                ImGui::Text("Completed ticks: %llu", static_cast<unsigned long long>(view.progress_->completed_tick));
            }
            const bool next = won && view.selectedMission_ == MissionId::reclaimFront
                && std::any_of(view.cards_.begin(), view.cards_.end(), [](const MissionCard& card)
                    { return card.id_ == MissionId::secureRelay && card.enabled_; });
            button("Next mission", NavigationAction::nextMission, view, intent, next,
                won ? "No further unlocked mission." : "Complete this objective to unlock the next mission.");
            button("Retry mission", NavigationAction::retryMission, view, intent, terminal);
            button("Return to menu", NavigationAction::back, view, intent);
            break;
        }
        case FrontendScreen::saveFailed:
            heading("Progress not saved");
            text("The last committed progress remains intact. Retry saving or choose an explicit unsaved exit.");
            button("Retry save", NavigationAction::retrySave, view, intent);
            button("Continue without saving", NavigationAction::continueWithoutSaving, view, intent);
            break;
        case FrontendScreen::quitConfirmation:
            heading("Quit Crucible?");
            text("Only committed mission-boundary progress is retained. The active mission is not a checkpoint.");
            button("Cancel", NavigationAction::cancelQuit, view, intent);
            button("Quit application", NavigationAction::confirmQuit, view, intent);
            break;
        }
        if (!view.status_.empty())
        {
            ImGui::Separator();
            caption(view.status_, cyan);
        }
        if (!view.error_.empty())
        {
            caption(view.error_, failure);
        }
        ImGui::PopTextWrapPos();
        ImGui::PopID();
        ImGui::End();
        ImGui::PopFont();
        return intent;
    }
};

Frontend::Frontend(ImGuiContext& context, const std::filesystem::path& fontDirectory)
    : state_(std::make_unique<State>(context, fontDirectory))
{
}
Frontend::~Frontend() = default;
std::optional<FrontendIntent> Frontend::draw(const FrontendView& view)
{
    return state_->draw(view);
}
}
