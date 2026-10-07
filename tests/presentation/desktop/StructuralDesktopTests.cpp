// Exercises real SDL key/radial input and the production desktop coordinator.
// Admission while paused is distinct from completion; normal refusal stays live.
#include <iostream>
#include <SDL3/SDL.h>
#include <stdexcept>

#include "crucible/presentation/ScenarioSnapshot.hpp"
#include "desktop/DesktopApp.hpp"

namespace {
using namespace crucible;
void Require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
void Key(desktop::DesktopApp& app, SDL_Keycode key) {
    SDL_Event event{}; event.type = SDL_EVENT_KEY_DOWN; event.key.key = key;
    Require(app.HandleEvent(event) == SDL_APP_CONTINUE, "structural key stays live");
}
void Receive(desktop::DesktopApp& app, std::uint64_t sequence) {
    for (int i = 0; i < 20 && app.GetSession().GetTrace().size() < sequence; ++i) {
        SDL_Delay(20); Require(app.Iterate() == SDL_APP_CONTINUE, "structural input iteration");
    }
    Require(app.GetSession().GetTrace().size() >= sequence, "structural key completes through bounded trace");
}
void Run() {
    desktop::DesktopApp app{{}, true};
    Key(app, SDLK_SPACE); Key(app, SDLK_F);
    Require(app.GetSession().GetSummary().ingress_pending == 1 &&
        !app.GetSession().GetSnapshot().GetInfo()->structural->occupied, "paused fuse queues without changing owned state");
    Require(app.Iterate() == SDL_APP_CONTINUE && app.GetSession().GetTrace().empty(), "paused structural iteration draws");
    Key(app, SDLK_SPACE); Receive(app, 1);
    Require(app.GetSession().GetLastStructuralResult() == StructuralCommandResult::insufficient_mass &&
        app.GetSession().GetStatus() == runtime::ClockDriver::Status::running, "normal insufficient-mass key refusal does not stop game");
    Key(app, SDLK_X); Receive(app, 2);
    Require(app.GetSession().GetLastStructuralResult() == StructuralCommandResult::empty, "empty shatter is visible normal refusal");
    Key(app, SDLK_R);
    Require(app.GetSession().GetTrace().empty() && app.GetSession().GetSnapshot().GetInfo()->completed_tick == 0,
        "restart clears structural commands/results and state");
    Key(app, SDLK_1);
    const auto relay = *app.GetCamera().TryToScreen({48.5F, 16.5F});
    SDL_Event click{}; click.type = SDL_EVENT_MOUSE_BUTTON_DOWN; click.button.button = SDL_BUTTON_LEFT;
    click.button.x = static_cast<float>(relay.x); click.button.y = static_cast<float>(relay.y);
    Require(app.HandleEvent(click) == SDL_APP_CONTINUE, "actual radial gather event");
    for (int i = 0; i < 120 && app.GetSession().GetSnapshot().GetInfo()->completed_tick < 60; ++i) {
        SDL_Delay(20); Require(app.Iterate() == SDL_APP_CONTINUE, "live gather iteration");
    }
    Require(app.GetSession().GetSnapshot().GetInfo()->structural->eligible_mobile >= 64, "live radial events gather eligible64");
    Key(app, SDLK_F); Receive(app, 2);
    Require(app.GetSession().GetSnapshot().GetInfo()->structural->occupied &&
        app.GetSession().GetSnapshot().GetInfo()->biomass->structure_mass == 64, "F applies real fusion");
    Key(app, SDLK_X); Receive(app, 3);
    Require(!app.GetSession().GetSnapshot().GetInfo()->structural->occupied &&
        app.GetSession().GetSnapshot().GetInfo()->biomass->lost_mass == 16 &&
        app.GetSession().GetSnapshot().GetInfo()->biomass->mobile_mass == 2032, "X applies real refund48 and loss16");
    Key(app, SDLK_SPACE); Key(app, SDLK_F); Key(app, SDLK_R);
    Require(app.GetSession().GetSummary().ingress_pending == 0 && app.GetSession().GetTrace().empty() &&
        app.GetSession().GetSnapshot().GetInfo()->biomass->lost_mass == 0, "restart discards queued structure intent");
    desktop::DesktopApp legacy;
    Key(legacy, SDLK_SPACE); Key(legacy, SDLK_F); Key(legacy, SDLK_X);
    Require(legacy.GetSession().GetSummary().ingress_pending == 0 && !legacy.GetSession().GetSnapshot().GetInfo()->structural,
        "legacy F fits and X does not admit structural commands");
}
}
int main() {
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"); SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) return 1;
    int result = 0;
    try { Run(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; result = 1; }
    SDL_Quit(); return result;
}
