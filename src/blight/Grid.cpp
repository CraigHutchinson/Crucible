#include <crucible/blight/Grid.hpp>

#include <stdexcept>
#include <utility>

namespace crucible::blight {
Grid::Grid(GridConfig config) {
    const auto extent = config.TryValidate();
    if (!extent) {
        throw std::invalid_argument("Invalid Blight grid geometry");
    }
    if (extent->cells > current_.max_size() || extent->cells > next_.max_size()) {
        throw std::length_error("Blight grid exceeds buffer capacity");
    }
    config_ = config;
    current_.resize(extent->cells);
    next_.resize(extent->cells);
}

bool Grid::TrySeed(std::size_t column, std::size_t row) noexcept {
    if (prepared_ || column >= config_.columns || row >= config_.rows) {
        return false;
    }
    auto &cell = current_[row * config_.columns + column];
    if (cell == 0) {
        cell = 1;
        ++infected_count_;
    }
    return true;
}

void Grid::Step() {
    auto prepared = TryPrepareStep();
    if (!prepared) throw std::logic_error("Blight step already prepared");
    static_cast<void>(std::move(*prepared).Commit());
}

std::optional<Grid::PreparedStep> Grid::TryPrepareStep() noexcept {
    if (prepared_) return std::nullopt;
    std::size_t next_count = 0;
    for (std::size_t row = 0; row < config_.rows; ++row) {
        for (std::size_t column = 0; column < config_.columns; ++column) {
            const auto index = row * config_.columns + column;
            const bool infected = current_[index] != 0 ||
                                  (row > 0 && current_[index - config_.columns] != 0) ||
                                  (row < config_.rows - 1 && current_[index + config_.columns] != 0) ||
                                  (column > 0 && current_[index - 1] != 0) ||
                                  (column < config_.columns - 1 && current_[index + 1] != 0);
            next_[index] = static_cast<std::uint8_t>(infected);
            next_count += static_cast<std::size_t>(infected);
        }
    }
    next_count_ = next_count;
    prepared_ = true;
    return PreparedStep{*this};
}

Grid::PreparedStep::PreparedStep(Grid &owner) noexcept : owner_(&owner) {}
Grid::PreparedStep::PreparedStep(PreparedStep &&other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)) {}
Grid::PreparedStep::~PreparedStep() {
    if (owner_) owner_->prepared_ = false;
}

bool Grid::PreparedStep::IsInfected(std::size_t index) const noexcept {
    return owner_ && index < owner_->next_.size() && owner_->next_[index] != 0;
}

bool Grid::PreparedStep::TryClear(std::size_t index) noexcept {
    if (!owner_ || index >= owner_->next_.size()) return false;
    if (owner_->next_[index] != 0) {
        owner_->next_[index] = 0;
        --owner_->next_count_;
    }
    return true;
}

bool Grid::PreparedStep::Commit() && noexcept {
    if (!owner_) return false;
    owner_->current_.swap(owner_->next_);
    owner_->infected_count_ = owner_->next_count_;
    owner_->prepared_ = false;
    owner_ = nullptr;
    return true;
}

std::size_t Grid::GetInfectedCount() const noexcept { return infected_count_; }

bool Grid::IsInfected(std::size_t column, std::size_t row) const noexcept {
    return column < config_.columns && row < config_.rows && current_[row * config_.columns + column] != 0;
}
} // namespace crucible::blight
