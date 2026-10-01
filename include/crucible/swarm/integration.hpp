#pragma once
#include <crucible/contracts/Position.hpp>
#include <crucible/contracts/Velocity.hpp>
#include <crucible/contracts/timing.hpp>

namespace crucible::swarm {
/// Advance an exclusively owned position by one fixed tick; velocity is borrowed
/// only for this call. The caller must exclude concurrent writers to position.
inline void integrate_position(Position& position, const Velocity& velocity) noexcept {
    position.x += velocity.x * tick_seconds;
    position.y += velocity.y * tick_seconds;
}
}
