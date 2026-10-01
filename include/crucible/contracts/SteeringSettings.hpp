#pragma once

namespace crucible {
/// Startup-only scenario steering values. The consumed steering module validates
/// finite positive radius/limits and finite nonnegative separation strength.
struct SteeringSettings {
    float neighbor_radius{1.5F};
    float separation_strength{6.0F};
    float maximum_acceleration{16.0F};
    float maximum_speed{4.0F};
};
}
