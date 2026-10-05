#include <crucible/interactions/Reclamation.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

#include <crucible/blight/Grid.hpp>

namespace crucible::interactions {
namespace {
constexpr std::optional<std::uint64_t> TryAdd(std::uint64_t left, std::uint64_t right) noexcept {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) return std::nullopt;
    return left + right;
}
bool Conserved(const BiomassLedger& ledger) noexcept {
    auto total = TryAdd(ledger.remaining_stock, ledger.mobile_mass);
    for (auto value : {ledger.reserve, ledger.structure_mass, ledger.lost_mass})
        total = total ? TryAdd(*total, value) : std::nullopt;
    return total && *total == ledger.initial_total;
}
std::uint64_t RequireProduct(std::size_t count, std::uint64_t value) {
    if (!std::in_range<std::uint64_t>(count) ||
        (value != 0 && static_cast<std::uint64_t>(count) > std::numeric_limits<std::uint64_t>::max() / value))
        throw std::overflow_error("Resource product exceeds uint64");
    return static_cast<std::uint64_t>(count) * value;
}
}

Reclamation::Reclamation(GridConfig config, std::size_t sample_capacity, ResourceSettings settings)
    : config_(config), settings_(settings) {
    const auto extent = config.TryValidate();
    if (!extent || settings.mass_per_sample == 0) throw std::invalid_argument("Invalid resource configuration");
    extent_ = *extent;
    const auto stock = RequireProduct(extent_.cells, settings.initial_stock_per_cell);
    const auto mobile = RequireProduct(sample_capacity, settings.mass_per_sample);
    const auto stock_and_mobile = TryAdd(stock, mobile);
    const auto total = stock_and_mobile ? TryAdd(*stock_and_mobile, settings.initial_reserve) : std::nullopt;
    if (!total) throw std::overflow_error("Initial biomass total exceeds uint64");
    if (extent_.cells > stocks_.max_size() || extent_.cells > pending_stocks_.max_size() ||
        sample_capacity > proposals_.max_size() || sample_capacity > seen_.max_size())
        throw std::length_error("Resource storage exceeds capacity");
    ledger_ = {*total, stock, mobile, settings.initial_reserve, 0, 0};
    stocks_.assign(extent_.cells, settings.initial_stock_per_cell);
    pending_stocks_.resize(extent_.cells);
    proposals_.resize(sample_capacity);
    seen_.resize(sample_capacity);
}

std::size_t Reclamation::CellIndex(Position position) const noexcept {
    const auto axis = [this](float coordinate, float maximum, std::size_t cells) {
        const double cell = std::floor(static_cast<double>(std::clamp(coordinate, 0.0F, maximum)) / config_.cell_size);
        // Test the bound before casting a potentially rounded large dimension.
        return cell >= static_cast<double>(cells - 1) ? cells - 1 : static_cast<std::size_t>(cell);
    };
    return axis(position.y, extent_.height, config_.rows) * config_.columns +
        axis(position.x, extent_.width, config_.columns);
}

bool Reclamation::TryStep(blight::Grid& blight, std::span<const SampleState> samples,
                          std::optional<ProtectedArea> protection) noexcept {
    const auto geometry = blight.GetConfig();
    if (geometry.columns != config_.columns || geometry.rows != config_.rows ||
        geometry.cell_size != config_.cell_size || samples.size() > proposals_.size()) return false;
    if (protection && (!std::isfinite(protection->center.x) || !std::isfinite(protection->center.y) ||
        !std::isfinite(protection->radius) || protection->radius < 0)) return false;
    if (static_cast<std::uint64_t>(samples.size()) != ledger_.mobile_mass / settings_.mass_per_sample ||
        ledger_.mobile_mass % settings_.mass_per_sample != 0) return false;
    std::ranges::fill(seen_, 0);
    std::size_t index = 0;
    for (const auto& sample : samples) {
        if (sample.activity != SampleActivity::mobile || sample.id.value == 0 || sample.id.value > proposals_.size() ||
            !std::isfinite(sample.position.x) || !std::isfinite(sample.position.y)) return false;
        auto& seen = seen_[static_cast<std::size_t>(sample.id.value - 1)];
        if (seen) return false;
        seen = 1;
        proposals_[index++] = {CellIndex(sample.position), sample.id};
    }
    auto proposals = std::span{proposals_}.first(samples.size());
    std::ranges::sort(proposals);
    auto pending = ledger_;
    if (!Conserved(pending)) return false;
    std::ranges::copy(stocks_, pending_stocks_.begin());
    auto step = blight.TryPrepareStep();
    if (!step) return false;
    std::uint64_t actions = 0;
    for (const auto& proposal : proposals) {
        if (actions == settings_.actions_per_tick) break;
        if (!step->IsInfected(proposal.cell)) continue;
        if (pending.work_actions == std::numeric_limits<std::uint64_t>::max()) return false;
        ++pending.work_actions;
        ++actions;
        auto& stock = pending_stocks_[proposal.cell];
        if (stock != 0) {
            if (pending.reserve == std::numeric_limits<std::uint64_t>::max() ||
                pending.harvested == std::numeric_limits<std::uint64_t>::max() || pending.remaining_stock == 0) return false;
            --stock;
            --pending.remaining_stock;
            ++pending.reserve;
            ++pending.harvested;
        }
        if (stock == 0 && !step->TryClear(proposal.cell)) return false;
    }
    if (protection) {
        const double squared_radius = static_cast<double>(protection->radius) * protection->radius;
        for (std::size_t row = 0; row < config_.rows; ++row)
            for (std::size_t column = 0; column < config_.columns; ++column) {
                const double x = (static_cast<double>(column) + .5) * config_.cell_size - protection->center.x;
                const double y = (static_cast<double>(row) + .5) * config_.cell_size - protection->center.y;
                if (x * x + y * y <= squared_radius && !step->TryClear(row * config_.columns + column))
                    return false;
            }
    }
    if (!std::move(*step).Commit()) return false;
    stocks_.swap(pending_stocks_);
    ledger_ = pending;
    return true;
}

bool Reclamation::TryAnchor(std::uint64_t mass) noexcept {
    if (!Conserved(ledger_) || mass == 0 || mass > ledger_.mobile_mass) return false;
    const auto anchored = TryAdd(ledger_.structure_mass, mass);
    if (!anchored) return false;
    ledger_.mobile_mass -= mass;
    ledger_.structure_mass = *anchored;
    return true;
}

bool Reclamation::TryRelease(std::uint64_t mass, std::uint64_t recovered) noexcept {
    if (!Conserved(ledger_) || mass == 0 || recovered > mass || mass > ledger_.structure_mass) return false;
    const auto mobile = TryAdd(ledger_.mobile_mass, recovered);
    const auto lost = TryAdd(ledger_.lost_mass, mass - recovered);
    if (!mobile || !lost) return false;
    ledger_.structure_mass -= mass;
    ledger_.mobile_mass = *mobile;
    ledger_.lost_mass = *lost;
    return true;
}

}
