#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cfenv>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>

#if defined(_M_X64) || defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#include <immintrin.h>
#define CRUCIBLE_TEST_HAS_MXCSR 1
#else
#define CRUCIBLE_TEST_HAS_MXCSR 0
#endif

#include "crucible/fields/FieldSet.hpp"
#include "crucible/scheduling/row_partitions.hpp"
#include "crucible/spatial/Grid.hpp"
#include "crucible/swarm/Steering.hpp"

namespace
{
using namespace crucible;
using scheduling::RowPartitions;
using scheduling::RowRange;
using Status = RowPartitions::RunStatus;

void check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool equalBits(const SampleState& left, const SampleState& right) noexcept
{
    const auto equalFloat = [](float a, float b)
    {
        return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
    };
    return left.id == right.id && left.activity == right.activity &&
        equalFloat(left.position.x, right.position.x) && equalFloat(left.position.y, right.position.y) &&
        equalFloat(left.velocity.x, right.velocity.x) && equalFloat(left.velocity.y, right.velocity.y);
}

#if CRUCIBLE_TEST_HAS_MXCSR
unsigned int supportedDenormals() noexcept
{
    alignas(16) std::array<unsigned char, 512> image{};
    _fxsave(image.data());
    unsigned int mask{};
    // Intel SDM: MXCSR_MASK at bytes28..31 determines writable DAZ/FTZ bits.
    std::memcpy(&mask, image.data() + 28, sizeof(mask));
    if (mask == 0)
    {
        mask = 0xFFBFU;
    }
    return mask & 0x8040U;
}
#endif

void receiveKernel(std::size_t count)
{
    const GridConfig config{8, 8, 1};
    std::vector<SampleState> input(count), expected(count), pending(count);
    std::vector<spatial::SpatialSample> gathered(count);
    for (std::size_t row = 0; row < count; ++row)
    {
        input[row] = {{1001 + row * 997}, row < count / 2 ? Position{4, 4}
            : Position{static_cast<float>(row % 9), static_cast<float>((row * 7) % 9)},
            {static_cast<float>(row % 3) - 1, static_cast<float>(row % 5) - 2}};
        gathered[row] = {input[row].id, input[row].position};
    }
    spatial::Grid grid{config, count};
    swarm::Steering steering{config, {1.5F, 6, 16, 4}, count};
    fields::FieldSet fields{2};
    check(fields.TryApplyEdit({FieldEditKind::set, 0, {4, 4}, 6, -9}) == fields::EditResult::applied,
        "Radial FP fixture rejected");
    check(fields.TryApplyEdit({FieldEditKind::set_flow, 1, {1, 4}, 3, 8, {7, 4}}) == fields::EditResult::applied,
        "Flow FP fixture rejected");
    constexpr std::size_t partitions = 7;
    std::vector<std::vector<SampleId>> scratch(partitions, std::vector<SampleId>(count));
    for (const std::size_t workers : {1U, 2U, 4U})
    {
        RowPartitions execution{count, {workers, partitions}, [&](RowRange range)
        {
            return steering.tryComputeRows(input, fields, grid, range.firstRow,
                std::span{pending}.subspan(range.firstRow, range.rowCount), scratch[range.partitionIndex]);
        }};
        for (const int mode : {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO})
        {
            check(std::fesetround(mode) == 0, "Standard rounding mode unavailable");
#if CRUCIBLE_TEST_HAS_MXCSR
            const auto controls = _mm_getcsr() & ~0x8040U;
            const auto supported = supportedDenormals();
            for (const unsigned int denormals : {0U, 0x8000U, 0x40U, 0x8040U})
            {
                if ((denormals & supported) != denormals)
                {
                    continue;
                }
                _mm_setcsr(controls | denormals);
#else
            {
#endif
                check(grid.TryRebuild(gathered), "FP fixture index rebuild failed");
                check(steering.TryCompute(input, fields, grid, expected), "Independent whole-kernel FP reference failed");
                check(execution.tryRun(count) == Status::complete, "Supported FP row graph failed");
                check(std::ranges::equal(pending, expected, equalBits), "Pooled FP row kernel differs bitwise");
                check(std::fegetround() == mode, "Row graph changed coordinator rounding mode");
#if CRUCIBLE_TEST_HAS_MXCSR
                check((_mm_getcsr() & 0xFFC0U) == ((controls | denormals) & 0xFFC0U),
                    "Row graph changed coordinator native FP controls");
#endif
            }
        }
    }
}

void receiveFlagsAndThrow()
{
    for (const std::size_t workers : {1U, 2U})
    {
        std::atomic<bool> fail{false};
        RowPartitions execution{8, {workers, 2}, [&](RowRange)
        {
            if (std::fegetround() != FE_DOWNWARD)
            {
                return false;
            }
            if (std::feraiseexcept(FE_INEXACT) != 0)
            {
                return false;
            }
            if (fail.load())
            {
                (void)std::fesetround(FE_UPWARD);
                throw std::runtime_error("FP restore unwinding fixture");
            }
            return true;
        }};
        check(std::fesetround(FE_DOWNWARD) == 0 && std::feclearexcept(FE_ALL_EXCEPT) == 0,
            "Coordinator FP setup failed");
        check(execution.tryRun(8) == Status::complete, "FP flags fixture failed");
        check((std::fetestexcept(FE_INEXACT) != 0) == (workers == 1),
            "Inline flags were erased or pool flags leaked into coordinator");
        fail = true;
        check(execution.tryRun(8) == Status::failed, "Throwing row callback was not failed");
        fail = false;
        check(std::fesetround(FE_DOWNWARD) == 0, "Coordinator rounding reset failed");
        check(execution.tryRun(8) == Status::complete, "FP worker restore or graph recovery failed after throw");
    }
}
}

int main()
{
    std::fenv_t saved;
    check(std::fegetenv(&saved) == 0, "Initial FP environment unavailable");
#if CRUCIBLE_TEST_HAS_MXCSR
    const auto nativeSaved = _mm_getcsr();
    std::cout << "x86SupportedDenormalControls=0x" << std::hex << supportedDenormals() << std::dec << '\n';
#endif
    receiveKernel(23);
    receiveKernel(257);
    receiveFlagsAndThrow();
    check(std::fesetenv(&saved) == 0, "Test FP environment restoration failed");
#if CRUCIBLE_TEST_HAS_MXCSR
    _mm_setcsr(nativeSaved);
#endif
}
