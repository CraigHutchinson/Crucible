#pragma once
#include <crucible/contracts/SampleState.hpp>
#include <crucible/contracts/FieldEdit.hpp>
#include <crucible/contracts/GridConfig.hpp>
#include <cstdint>
#include <span>

namespace crucible {
/// Caller-owned, mutually disjoint destinations; storage never escapes the copy call.
struct StateCopyDestination {
    std::span<SampleState> samples;
    std::span<FieldEdit> fields;
    std::span<std::uint8_t> blight;
};
/// Geometry and used lengths of a coordinator-quiescent scenario copy.
struct ScenarioStateInfo {
    GridConfig grid;
    std::uint64_t completed_tick{};
    std::size_t samples{}, fields{}, cells{};
};
}
