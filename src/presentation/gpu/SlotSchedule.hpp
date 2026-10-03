#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
namespace crucible::presentation::gpu::detail {
/// Concrete three-slot submission bookkeeping, consumed by OffscreenRenderer.
/// Fences are owned by the renderer; this value records only identities/retirement.
class SlotSchedule {
public:
    static constexpr std::size_t Count = 3;
    struct Entry { std::uint64_t id{}, tick{}; bool submitted{}, complete{}; };
    [[nodiscard]] std::optional<std::size_t> FindAvailable() const noexcept;
    [[nodiscard]] std::uint64_t Submit(std::size_t slot, std::uint64_t tick) noexcept;
    void Complete(std::size_t slot) noexcept;
    [[nodiscard]] bool Matches(std::size_t slot, std::uint64_t id) const noexcept;
    [[nodiscard]] const Entry& Get(std::size_t slot) const noexcept { return entries_[slot]; }
    [[nodiscard]] bool CanSubmit() const noexcept;
private:
    std::array<Entry, Count> entries_{};
    std::uint64_t next_id_{1};
};
}
