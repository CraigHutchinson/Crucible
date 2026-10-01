#pragma once

#include <cstddef>
#include <optional>

namespace crucible {
/// Derived storage and world bounds of a validated rectangular grid.
struct GridExtent { std::size_t cells{}; float width{}, height{}; };

/// Startup grid dimensions; positions clamp to its closed world rectangle.
struct GridConfig {
    std::size_t columns{}, rows{};
    float cell_size{};

    /// Rejects empty/nonfinite geometry, arithmetic overflow and unrepresentable float extents.
    [[nodiscard]] std::optional<GridExtent> TryValidate() const noexcept;
};
}
