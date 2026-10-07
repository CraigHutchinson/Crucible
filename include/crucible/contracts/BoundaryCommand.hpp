#pragma once
#include <crucible/contracts/FieldEdit.hpp>
#include <cstdint>
#include <type_traits>

namespace crucible {
/// One owned command stream; structural intent never masquerades as a field slot.
enum class BoundaryAction { field, fuse, shatter };
struct BoundaryCommand {
    FieldEdit field{};
    BoundaryAction action{BoundaryAction::field};
    std::uint64_t generation{};
    BoundaryCommand() = default;
    constexpr BoundaryCommand(FieldEdit edit) noexcept : field(edit) {}
    [[nodiscard]] static constexpr BoundaryCommand Fuse() noexcept {
        BoundaryCommand command; command.action = BoundaryAction::fuse; return command;
    }
    [[nodiscard]] static constexpr BoundaryCommand Shatter(std::uint64_t value) noexcept {
        BoundaryCommand command; command.action = BoundaryAction::shatter;
        command.generation = value; return command;
    }
    [[nodiscard]] bool IsValid(std::size_t fields) const noexcept {
        switch (action) {
        case BoundaryAction::field: return generation == 0 && field.IsValid(fields);
        case BoundaryAction::fuse: return generation == 0;
        case BoundaryAction::shatter: return true; // stale/empty is a normal application result
        }
        return false;
    }
};
static_assert(std::is_trivially_copyable_v<BoundaryCommand>);
}
