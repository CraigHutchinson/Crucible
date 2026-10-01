#include <crucible/blight/Grid.hpp>

#include <stdexcept>

namespace crucible::blight {
Grid::Grid(GridConfig config) {
    const auto extent = config.TryValidate();
    if (!extent) {
        throw std::invalid_argument("Invalid Blight grid geometry");
    }
    if (extent->cells > current_.max_size() || extent->cells > next_.max_size()) {
        throw std::length_error("Blight grid exceeds buffer capacity");
    }
    columns_ = config.columns;
    rows_ = config.rows;
    current_.resize(extent->cells);
    next_.resize(extent->cells);
}

bool Grid::TrySeed(std::size_t column, std::size_t row) noexcept {
    if (column >= columns_ || row >= rows_) {
        return false;
    }
    auto &cell = current_[row * columns_ + column];
    if (cell == 0) {
        cell = 1;
        ++infected_count_;
    }
    return true;
}

void Grid::Step() noexcept {
    std::size_t next_count = 0;
    for (std::size_t row = 0; row < rows_; ++row) {
        for (std::size_t column = 0; column < columns_; ++column) {
            const auto index = row * columns_ + column;
            const bool infected = current_[index] != 0 ||
                                  (row > 0 && current_[index - columns_] != 0) ||
                                  (row < rows_ - 1 && current_[index + columns_] != 0) ||
                                  (column > 0 && current_[index - 1] != 0) ||
                                  (column < columns_ - 1 && current_[index + 1] != 0);
            next_[index] = static_cast<std::uint8_t>(infected);
            next_count += static_cast<std::size_t>(infected);
        }
    }
    current_.swap(next_);
    infected_count_ = next_count;
}

std::size_t Grid::GetInfectedCount() const noexcept { return infected_count_; }

bool Grid::IsInfected(std::size_t column, std::size_t row) const noexcept {
    return column < columns_ && row < rows_ && current_[row * columns_ + column] != 0;
}
} // namespace crucible::blight
