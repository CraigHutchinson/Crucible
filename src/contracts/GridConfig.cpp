#include <crucible/contracts/GridConfig.hpp>

#include <cmath>
#include <limits>

namespace crucible {
std::optional<GridExtent> GridConfig::TryValidate() const noexcept {
    if (columns == 0 || rows == 0 || !std::isfinite(cell_size) || cell_size <= 0.0F ||
        columns > std::numeric_limits<std::size_t>::max() / rows) return std::nullopt;
    const auto cells = columns * rows;
    const double width = static_cast<double>(columns) * cell_size;
    const double height = static_cast<double>(rows) * cell_size;
    const double maximum = std::numeric_limits<float>::max();
    if (!std::isfinite(width) || !std::isfinite(height) || width > maximum || height > maximum)
        return std::nullopt;
    return GridExtent{cells, static_cast<float>(width), static_cast<float>(height)};
}
}
