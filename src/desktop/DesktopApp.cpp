#include "DesktopApp.hpp"
#include <crucible/presentation/FieldTool.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <cmath>
#include <stdexcept>
#include <string_view>
namespace crucible::desktop {
namespace {
void Check(bool ok) { if (!ok) throw std::runtime_error(SDL_GetError()); }
}
DesktopApp::DesktopApp(ReclamationMissionSettings mission)
    : session_(2048, {64, 4096}, mission) {
    window_.reset(SDL_CreateWindow("Crucible - reclamation challenge", 1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));
    Check(static_cast<bool>(window_));
    renderer_.reset(SDL_CreateRenderer(window_.get(), nullptr));
    Check(static_cast<bool>(renderer_));
    Check(SDL_SetRenderLogicalPresentation(renderer_.get(), 1280, 720, SDL_LOGICAL_PRESENTATION_LETTERBOX));
    // Best effort pacing: software/dummy renderers may not support vertical sync.
    static_cast<void>(SDL_SetRenderVSync(renderer_.get(), 1));
    baseline_ = std::chrono::steady_clock::now();
}
DesktopApp::~DesktopApp() = default;
presentation::ScreenPoint DesktopApp::ToLogical(float x, float y) const {
    float lx{}, ly{};
    Check(SDL_RenderCoordinatesFromWindow(renderer_.get(), x, y, &lx, &ly));
    return {lx, ly};
}
void DesktopApp::Preview(presentation::ScreenPoint point) {
    if (const auto mission = session_.GetMission(); mission && mission->outcome != ReclamationMissionOutcome::active) {
        preview_.reset();
        return;
    }
    if (const auto world = camera_.TryToWorld(point))
        preview_ = presentation::TryBuildFieldEdit({tool_, slot_, 8, 4}, *world, 4);
    else if (!admission_ || admission_->status == runtime::CommandIngress::AdmissionStatus::accepted)
        preview_.reset();
}
void DesktopApp::Admit(FieldEdit edit) {
    preview_ = edit;
    admission_ = session_.TryAdmitFieldEdit(edit);
}
void DesktopApp::Suspend(bool value) {
    if (value == suspended_) return;
    if (value) {
        restore_running_ = session_.GetStatus() == runtime::ClockDriver::Status::running;
        session_.Pause();
        dragging_ = false;
    } else if (restore_running_) session_.Resume();
    suspended_ = value;
    baseline_ = std::chrono::steady_clock::now();
}
void DesktopApp::Act(Action action) {
    using presentation::FieldTool;
    switch (action) {
    case Action::attract: tool_ = FieldTool::attract; preview_.reset(); break;
    case Action::repel: tool_ = FieldTool::repel; preview_.reset(); break;
    case Action::erase: tool_ = FieldTool::remove; preview_.reset(); break;
    case Action::slot: slot_ = (slot_ + 1) % 4; preview_.reset(); break;
    case Action::pause:
        if (session_.GetStatus() == runtime::ClockDriver::Status::paused) session_.Resume();
        else session_.Pause();
        baseline_ = std::chrono::steady_clock::now();
        break;
    case Action::restart:
        session_.Restart(); camera_.ResetFit(); preview_.reset(); admission_.reset(); dragging_ = false;
        message_ = "Fresh challenge - recover biomass before the deadline";
        if (suspended_) { restore_running_ = true; session_.Pause(); }
        baseline_ = std::chrono::steady_clock::now();
        break;
    case Action::fit: camera_.ResetFit(); break;
    }
}
SDL_AppResult DesktopApp::HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        session_.Close(); return SDL_APP_SUCCESS;
    }
    if (event.type == SDL_EVENT_WILL_ENTER_BACKGROUND || event.type == SDL_EVENT_DID_ENTER_BACKGROUND) {
        background_ = true; Suspend(background_ || minimized_); return SDL_APP_CONTINUE;
    }
    if (event.type == SDL_EVENT_WINDOW_MINIMIZED) {
        minimized_ = true; Suspend(background_ || minimized_); return SDL_APP_CONTINUE;
    }
    if (event.type == SDL_EVENT_DID_ENTER_FOREGROUND) {
        background_ = false; Suspend(background_ || minimized_); return SDL_APP_CONTINUE;
    }
    if (event.type == SDL_EVENT_WINDOW_RESTORED) {
        minimized_ = false; Suspend(background_ || minimized_); return SDL_APP_CONTINUE;
    }
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) dragging_ = false;
    int width{}, height{};
    Check(SDL_GetRenderOutputSize(renderer_.get(), &width, &height));
    if (suspended_ || width <= 0 || height <= 0) return SDL_APP_CONTINUE;
    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        switch (event.key.key) {
        case SDLK_1: Act(Action::attract); break;
        case SDLK_2: Act(Action::repel); break;
        case SDLK_3: Act(Action::erase); break;
        case SDLK_TAB: Act(Action::slot); break;
        case SDLK_SPACE: Act(Action::pause); break;
        case SDLK_R: Act(Action::restart); break;
        case SDLK_F: Act(Action::fit); break;
        case SDLK_ESCAPE: preview_.reset(); admission_.reset(); break;
        case SDLK_DELETE: Admit({FieldEditKind::remove, slot_, {}, 0, 0}); break;
        default: break;
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        const auto point = ToLogical(event.button.x, event.button.y);
        if (event.button.button == SDL_BUTTON_MIDDLE && camera_.TryToWorld(point)) {
            dragging_ = true; last_pointer_ = point;
        } else if (event.button.button == SDL_BUTTON_LEFT) {
            // Seven fixed prototype buttons use the painter's logical toolbar layout.
            if (point.y >= 632 && point.y < 664) {
                for (std::size_t i = 0; i < presentation::desktop::ToolbarButtonCount; ++i) {
                    const auto bounds = presentation::desktop::ToolbarButton(i);
                    if (point.x >= bounds.x && point.x < bounds.x + bounds.width) Act(static_cast<Action>(i));
                }
            } else if (const auto world = camera_.TryToWorld(point)) {
                if (const auto edit = presentation::TryBuildFieldEdit({tool_, slot_, 8, 4}, *world, 4)) Admit(*edit);
            }
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_MIDDLE) {
        dragging_ = false;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        const auto point = ToLogical(event.motion.x, event.motion.y);
        if (dragging_) {
            static_cast<void>(camera_.TryPan({point.x - last_pointer_.x, point.y - last_pointer_.y}));
            last_pointer_ = point;
        } else Preview(point);
    } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        const auto point = ToLogical(event.wheel.mouse_x, event.wheel.mouse_y);
        const double direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
        static_cast<void>(camera_.TryZoom(point, std::pow(1.2, direction * event.wheel.y)));
        Preview(point);
    }
    return SDL_APP_CONTINUE;
}
SDL_AppResult DesktopApp::Iterate() {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - baseline_);
    baseline_ = now;
    int width{}, height{};
    Check(SDL_GetRenderOutputSize(renderer_.get(), &width, &height));
    if (suspended_ || width <= 0 || height <= 0) return SDL_APP_CONTINUE;
    static_cast<void>(session_.TryPump(elapsed));
    std::string_view message = message_;
    if (admission_) {
        using Status = runtime::CommandIngress::AdmissionStatus;
        switch (admission_->status) {
        case Status::accepted:
            if (session_.GetTrace().size() >= admission_->last_sequence) {
                message = "Applied at completed boundary"; preview_.reset(); admission_.reset();
                message_ = message;
            } else message = "Queued - waiting for completed boundary";
            break;
        case Status::full: message = "Queue full - edit retained for retry"; break;
        case Status::closed: message = "Run closed - restart to edit"; break;
        case Status::invalid: message = "Invalid edit - adjust and retry"; break;
        case Status::sequence_exhausted: message = "Sequence exhausted - restart"; break;
        }
    }
    if (session_.GetStatus() == runtime::ClockDriver::Status::blocked) message = "Trace full - boundary blocked; restart";
    const auto mission = session_.GetMission();
    if (mission && mission->outcome != ReclamationMissionOutcome::active) {
        preview_.reset();
        if (admission_ && admission_->status == runtime::CommandIngress::AdmissionStatus::closed)
            message = "Run stopped - field edits disabled; R or RESTART to edit";
        else
            message = mission->outcome == ReclamationMissionOutcome::won
                ? "Quota secured - press R or RESTART for a fresh challenge"
                : "Deadline reached - press R or RESTART to try another route";
    }
    const presentation::desktop::SceneUi ui{
        session_.GetStatus() == runtime::ClockDriver::Status::paused,
        session_.GetStatus() == runtime::ClockDriver::Status::blocked, tool_, slot_, preview_, message, mission};
    Check(painter_.TryDraw(*renderer_, session_.GetSnapshot(), camera_, ui));
    Check(SDL_RenderPresent(renderer_.get()));
    return SDL_APP_CONTINUE;
}
}
