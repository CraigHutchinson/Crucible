#include "DesktopApp.hpp"
#include <crucible/presentation/FieldTool.hpp>
#include <crucible/presentation/desktop/SceneUi.hpp>
#include <cmath>
#include <stdexcept>
#include <string_view>
namespace crucible::desktop {
namespace {
void Check(bool ok) { if (!ok) throw std::runtime_error(SDL_GetError()); }
std::string_view StructuralFeedback(StructuralCommandResult result) noexcept {
    switch (result) {
    case StructuralCommandResult::applied: return "Structure action applied at completed boundary";
    case StructuralCommandResult::disabled: return "Structure actions unavailable in this run";
    case StructuralCommandResult::occupied: return "Fuse refused: lattice already occupied";
    case StructuralCommandResult::insufficient_mass: return "Fuse refused: gather 64 mobile nanites inside the relay ring";
    case StructuralCommandResult::empty: return "Shatter refused: relay has no lattice";
    case StructuralCommandResult::stale_generation: return "Shatter refused: target generation has changed";
    case StructuralCommandResult::generation_exhausted: return "Structure generation exhausted: restart";
    }
    return "Unknown structural result";
}
}
DesktopApp::DesktopApp(ReclamationMissionSettings mission, bool structural)
    : session_(2048, {64, 4096}, mission, structural), structural_(structural) {
    if (structural_) message_ = "Gather 64 at relay, F fuse, X shatter; quota plus 120 held ticks wins";
    window_.reset(SDL_CreateWindow(structural_ ? "Crucible - secure the relay" : "Crucible - reclamation challenge", 1280, 720,
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
void DesktopApp::CancelGesture() noexcept {
    flow_start_.reset(); preview_.reset(); dragging_ = false;
}
void DesktopApp::Preview(presentation::ScreenPoint point) {
    if (const auto mission = session_.GetMission(); mission && mission->outcome != ReclamationMissionOutcome::active) {
        preview_.reset();
        return;
    }
    if (tool_ == presentation::FieldTool::flow) {
        if (flow_start_) {
            const auto world = camera_.TryToWorld(point);
            preview_ = world ? presentation::TryBuildFieldEdit({tool_, slot_, 8, 4}, *flow_start_, 4, *world)
                             : std::nullopt;
        }
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
    if (admission_->status == runtime::CommandIngress::AdmissionStatus::accepted) {
        admitted_preview_ = edit; admitted_sequence_ = admission_->last_sequence; preview_.reset();
    } else if (admission_->status == runtime::CommandIngress::AdmissionStatus::closed) {
        preview_.reset(); admitted_preview_.reset(); admitted_sequence_ = 0;
    }
}
void DesktopApp::Suspend(bool value) {
    if (value == suspended_) return;
    if (value) {
        restore_running_ = session_.GetStatus() == runtime::ClockDriver::Status::running;
        session_.Pause();
        CancelGesture();
    } else if (restore_running_) session_.Resume();
    suspended_ = value;
    baseline_ = std::chrono::steady_clock::now();
}
void DesktopApp::Act(Action action) {
    using presentation::FieldTool;
    switch (action) {
    case Action::attract: CancelGesture(); tool_ = FieldTool::attract; break;
    case Action::repel: CancelGesture(); tool_ = FieldTool::repel; break;
    case Action::erase: CancelGesture(); tool_ = FieldTool::remove; break;
    case Action::flow: CancelGesture(); tool_ = FieldTool::flow; break;
    case Action::slot: CancelGesture(); slot_ = (slot_ + 1) % 4; break;
    case Action::pause:
        if (session_.GetStatus() == runtime::ClockDriver::Status::paused) session_.Resume();
        else session_.Pause();
        baseline_ = std::chrono::steady_clock::now();
        break;
    case Action::restart:
        session_.Restart(); camera_.ResetFit(); CancelGesture(); admitted_preview_.reset(); admitted_sequence_ = 0; admission_.reset(); structural_admission_.reset();
        message_ = structural_ ? "Fresh relay: gather 64, F fuse, hold 120 ticks and recover quota" : "Fresh challenge - recover biomass before the deadline";
        if (suspended_) { restore_running_ = true; session_.Pause(); }
        baseline_ = std::chrono::steady_clock::now();
        break;
    case Action::fit: CancelGesture(); camera_.ResetFit(); break;
    case Action::fuse:
        CancelGesture(); structural_admission_ = session_.TryFuseRelay(); break;
    case Action::shatter:
        CancelGesture(); structural_admission_ = session_.TryShatterRelay(); break;
    }
}
SDL_AppResult DesktopApp::HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        CancelGesture(); admitted_preview_.reset(); admitted_sequence_ = 0; session_.Close(); return SDL_APP_SUCCESS;
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
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) { CancelGesture(); return SDL_APP_CONTINUE; }
    int width{}, height{};
    Check(SDL_GetRenderOutputSize(renderer_.get(), &width, &height));
    if (suspended_ || width <= 0 || height <= 0) return SDL_APP_CONTINUE;
    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        switch (event.key.key) {
        case SDLK_1: Act(Action::attract); break;
        case SDLK_2: Act(Action::repel); break;
        case SDLK_3: Act(Action::erase); break;
        case SDLK_4: Act(Action::flow); break;
        case SDLK_TAB: Act(Action::slot); break;
        case SDLK_SPACE: Act(Action::pause); break;
        case SDLK_R: Act(Action::restart); break;
        case SDLK_F: Act(structural_ ? Action::fuse : Action::fit); break;
        case SDLK_X: if (structural_) Act(Action::shatter); break;
        case SDLK_ESCAPE: CancelGesture(); break;
        case SDLK_DELETE: CancelGesture(); Admit({FieldEditKind::remove, slot_, {}, 0, 0}); break;
        default: break;
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        const auto point = ToLogical(event.button.x, event.button.y);
        if (event.button.button == SDL_BUTTON_MIDDLE && camera_.TryToWorld(point)) {
            CancelGesture(); dragging_ = true; last_pointer_ = point;
        } else if (event.button.button == SDL_BUTTON_LEFT && !dragging_) {
            // Fixed prototype buttons use the painter's logical toolbar layout.
            if (point.y >= 632 && point.y < 664) {
                for (std::size_t i = 0; i < presentation::desktop::ToolbarButtonCount; ++i) {
                    const auto bounds = presentation::desktop::ToolbarButton(i);
                    if (point.x >= bounds.x && point.x < bounds.x + bounds.width) Act(static_cast<Action>(i));
                }
            } else if (const auto world = camera_.TryToWorld(point)) {
                if (tool_ == presentation::FieldTool::flow) {
                    CancelGesture();
                    if (session_.GetStatus() != runtime::ClockDriver::Status::closed) flow_start_ = *world;
                } else if (const auto edit = presentation::TryBuildFieldEdit({tool_, slot_, 8, 4}, *world, 4)) Admit(*edit);
            }
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_MIDDLE) {
        dragging_ = false;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT && flow_start_) {
        const auto start = *flow_start_;
        const auto world = camera_.TryToWorld(ToLogical(event.button.x, event.button.y));
        CancelGesture();
        if (world) {
            if (const auto edit = presentation::TryBuildFieldEdit({tool_, slot_, 8, 4}, start, 4, *world)) Admit(*edit);
        }
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        const auto point = ToLogical(event.motion.x, event.motion.y);
        if (dragging_) {
            static_cast<void>(camera_.TryPan({point.x - last_pointer_.x, point.y - last_pointer_.y}));
            last_pointer_ = point;
        } else Preview(point);
    } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        CancelGesture();
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
    // Rejected admission feedback cannot obscure retirement of an earlier accepted preview.
    if (admitted_preview_ && session_.GetTrace().size() >= admitted_sequence_) {
        admitted_preview_.reset(); admitted_sequence_ = 0;
    }
    std::string_view message = message_;
    if (admission_) {
        using Status = runtime::CommandIngress::AdmissionStatus;
        switch (admission_->status) {
        case Status::accepted:
            if (session_.GetTrace().size() >= admission_->last_sequence) {
                message = "Applied at completed boundary"; admitted_preview_.reset(); admitted_sequence_ = 0; admission_.reset();
                message_ = message;
            } else message = "Queued - waiting for completed boundary";
            break;
        case Status::full: message = "Queue full - edit retained for retry"; break;
        case Status::closed: message = "Run closed - restart to edit"; break;
        case Status::invalid: message = "Invalid edit - adjust and retry"; break;
        case Status::sequence_exhausted: message = "Sequence exhausted - restart"; break;
        }
    }
    if (structural_admission_) {
        using Status = runtime::CommandIngress::AdmissionStatus;
        switch (structural_admission_->status) {
        case Status::accepted:
            if (session_.GetTrace().size() >= structural_admission_->last_sequence) {
                const auto result = session_.GetLastStructuralResult();
                message_ = result ? StructuralFeedback(*result) : "Structure boundary completed";
                message = message_; structural_admission_.reset();
            } else message = "Structure action queued - waiting for completed boundary";
            break;
        case Status::full: message = "Queue full - retry structure action"; break;
        case Status::closed: message = "Run stopped - restart for structure actions"; break;
        case Status::invalid: message = "Invalid structure action"; break;
        case Status::sequence_exhausted: message = "Sequence exhausted - restart"; break;
        }
    }
    if (session_.GetStatus() == runtime::ClockDriver::Status::blocked) message = "Trace full - boundary blocked; restart";
    const auto mission = session_.GetMission();
    if (mission && mission->outcome != ReclamationMissionOutcome::active) {
        CancelGesture(); admitted_preview_.reset(); admitted_sequence_ = 0;
        if (admission_ && admission_->status == runtime::CommandIngress::AdmissionStatus::closed)
            message = "Run stopped - field edits disabled; R or RESTART to edit";
        else
            message = mission->outcome == ReclamationMissionOutcome::won
                ? (structural_ ? "Quota and relay hold secured - R or RESTART for a fresh run" : "Quota secured - press R or RESTART for a fresh challenge")
                : (structural_ ? "Deadline or insufficient recoverable mass - R or RESTART to retry" : "Deadline reached - press R or RESTART to try another route");
    }
    const presentation::desktop::SceneUi ui{
        session_.GetStatus() == runtime::ClockDriver::Status::paused,
        session_.GetStatus() == runtime::ClockDriver::Status::blocked, tool_, slot_, GetPreview(), message, mission};
    Check(painter_.TryDraw(*renderer_, session_.GetSnapshot(), camera_, ui));
    Check(SDL_RenderPresent(renderer_.get()));
    return SDL_APP_CONTINUE;
}
}
