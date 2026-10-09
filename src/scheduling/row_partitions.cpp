#include <algorithm>
#include <atomic>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <sub0pipeline/executor/priority_executor.hpp>
#include <sub0pipeline/pipeline.hpp>

#if defined(_M_X64) || defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#include <xmmintrin.h>
#define CRUCIBLE_SCHEDULING_HAS_MXCSR 1
#else
#define CRUCIBLE_SCHEDULING_HAS_MXCSR 0
#endif

#include "crucible/scheduling/row_partitions.hpp"

namespace crucible::scheduling
{
namespace
{
struct FloatingPointEnvironment
{
    [[nodiscard]] static std::optional<FloatingPointEnvironment> capture() noexcept
    {
        FloatingPointEnvironment result;
        if (std::fegetenv(&result.environment_) != 0)
        {
            return std::nullopt;
        }
        result.rounding_ = std::fegetround();
        if (result.rounding_ == -1)
        {
            return std::nullopt;
        }
#if CRUCIBLE_SCHEDULING_HAS_MXCSR
        result.mxcsr_ = _mm_getcsr();
#endif
        return result;
    }

    [[nodiscard]] bool install() const noexcept
    {
        const bool standardInstalled = std::fesetenv(&environment_) == 0;
#if CRUCIBLE_SCHEDULING_HAS_MXCSR
        _mm_setcsr(mxcsr_);
        // Verify control bits; arithmetic may legitimately change sticky flags.
        constexpr unsigned int controlMask = 0xFFC0U;
        const bool nativeInstalled = (_mm_getcsr() & controlMask) == (mxcsr_ & controlMask);
#else
        const bool nativeInstalled = true;
#endif
        return standardInstalled && nativeInstalled && std::fegetround() == rounding_;
    }

    std::fenv_t environment_{};
    int rounding_{};
#if CRUCIBLE_SCHEDULING_HAS_MXCSR
    unsigned int mxcsr_{};
#endif
};

/** Restores a persistent worker even when its row callable throws. */
class WorkerEnvironment
{
public:
    explicit WorkerEnvironment(std::atomic<bool>& poisoned) noexcept
        : original_{FloatingPointEnvironment::capture()}, poisoned_{poisoned}
    {
    }

    ~WorkerEnvironment()
    {
        if (active_)
        {
            (void)restore();
        }
    }

    WorkerEnvironment(const WorkerEnvironment&) = delete;
    WorkerEnvironment& operator=(const WorkerEnvironment&) = delete;

    [[nodiscard]] bool isAvailable() const noexcept
    {
        return original_.has_value();
    }

    [[nodiscard]] bool restore() noexcept
    {
        active_ = false;
        if (!original_ || !original_->install())
        {
            poisoned_.store(true, std::memory_order_release);
            return false;
        }
        return true;
    }

private:
    std::optional<FloatingPointEnvironment> original_;
    std::atomic<bool>& poisoned_; // non-owning, adapter outlives every joined job
    bool active_{original_.has_value()};
};
}

struct RowPartitions::Impl
{
    struct Partition
    {
        RowRange range{};
        bool unsupported{};
    };

    Impl(std::size_t rowCapacity, ExecutionSettings settings, RowFunction rowFunction)
        : rowCapacity_{rowCapacity}, settings_{settings}, rowFunction_{std::move(rowFunction)},
          partitions_(settings.partitions)
    {
        graph_.reserve(settings.partitions);
        for (std::size_t partition = 0; partition < settings.partitions; ++partition)
        {
            (void)graph_.emplace([this, partition] { return runPartition(partition); })
                .name("rows_" + std::to_string(partition));
        }
        // Prime cached roots and traversal/failure storage without calling rows.
        if (!graph_.runInline())
        {
            throw std::runtime_error("Inactive row graph priming failed");
        }
        if (settings.workers > 1)
        {
            pool_ = std::make_unique<sub0pipeline::PriorityExecutor>(
                sub0pipeline::PriorityExecutor::Options{
                    .threadCount = static_cast<unsigned int>(settings.workers),
                    .queueCapacity = settings.partitions});
        }
    }

    [[nodiscard]] std::expected<void, sub0pipeline::PipelineError> runPartition(std::size_t index) noexcept
    try
    {
        if (!active_)
        {
            return {};
        }
        if (poisoned_.load(std::memory_order_acquire))
        {
            return std::unexpected(sub0pipeline::PipelineError::kJobFailed);
        }
        auto& partition = partitions_[index];
        if (partition.range.rowCount == 0)
        {
            return {};
        }
        if (!pool_)
        {
            return rowFunction_(partition.range)
                ? std::expected<void, sub0pipeline::PipelineError>{}
                : std::unexpected(sub0pipeline::PipelineError::kJobFailed);
        }

        WorkerEnvironment previous{poisoned_};
        if (!previous.isAvailable())
        {
            partition.unsupported = true;
            return {};
        }
        if (!requested_->install())
        {
            partition.unsupported = true;
            return previous.restore()
                ? std::expected<void, sub0pipeline::PipelineError>{}
                : std::unexpected(sub0pipeline::PipelineError::kJobFailed);
        }
        const bool computed = rowFunction_(partition.range);
        // Checked restoration precedes success publication; the guard covers throws.
        const bool restored = previous.restore();
        return computed && restored
            ? std::expected<void, sub0pipeline::PipelineError>{}
            : std::unexpected(sub0pipeline::PipelineError::kJobFailed);
    }

    catch (...)
    {
        // Pipeline executor bodies must not throw. Worker restoration unwinds first.
        return std::unexpected(sub0pipeline::PipelineError::kJobFailed);
    }
    [[nodiscard]] RunStatus tryRun(std::size_t rows)
    {
        if (poisoned_.load(std::memory_order_acquire))
        {
            return RunStatus::failed;
        }
        if (rows > rowCapacity_)
        {
            return RunStatus::invalidRows;
        }
        if (rows == 0)
        {
            return RunStatus::complete;
        }
        const auto activePartitions = std::min(rows, settings_.partitions);
        const auto quotient = rows / activePartitions;
        const auto remainder = rows % activePartitions;
        std::size_t firstRow = 0;
        for (std::size_t index = 0; index < partitions_.size(); ++index)
        {
            const auto count = index < activePartitions ? quotient + (index < remainder ? 1U : 0U) : 0U;
            partitions_[index] = {{firstRow, count, index}, false};
            firstRow += count;
        }
        if (pool_)
        {
            requested_ = FloatingPointEnvironment::capture();
            if (!requested_)
            {
                return RunStatus::unsupportedFloatingPoint;
            }
        }
        active_ = true;
        struct ActiveGuard
        {
            bool& active;
            ~ActiveGuard() { active = false; }
        } active{active_};
        std::expected<void, sub0pipeline::PipelineError> result;
        try
        {
            result = pool_ ? graph_.run(*pool_) : graph_.runInline();
        }
        catch (...)
        {
            if (pool_)
            {
                pool_->waitAll();
            }
            return RunStatus::failed;
        }
        if (!result || poisoned_.load(std::memory_order_acquire))
        {
            return RunStatus::failed;
        }
        const bool unsupported = std::ranges::any_of(partitions_, &Partition::unsupported);
        return unsupported ? RunStatus::unsupportedFloatingPoint : RunStatus::complete;
    }

    std::size_t rowCapacity_;
    ExecutionSettings settings_;
    RowFunction rowFunction_;
    std::vector<Partition> partitions_;
    std::optional<FloatingPointEnvironment> requested_;
    std::atomic<bool> poisoned_{false};
    bool active_{false};
    sub0pipeline::Pipeline graph_;
    // Destruction joins the pool before releasing graph, metadata or callback state.
    std::unique_ptr<sub0pipeline::PriorityExecutor> pool_;
};

RowPartitions::RowPartitions(std::size_t rowCapacity, ExecutionSettings settings, RowFunction rowFunction)
{
    if (!rowFunction || settings.workers == 0 || settings.partitions == 0 ||
        settings.partitions > std::max(rowCapacity, std::size_t{1}))
    {
        throw std::invalid_argument("Invalid row execution settings or empty callable");
    }
    constexpr auto maximumWorkers = static_cast<std::size_t>(std::numeric_limits<int>::max());
    constexpr auto maximumJobs = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
    if (settings.workers > maximumWorkers || settings.workers > maximumJobs ||
        settings.partitions > maximumJobs - settings.workers || settings.partitions > maxPartitions ||
        settings.partitions > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(Impl::Partition))
    {
        throw std::length_error("Row execution storage or task counts are unrepresentable");
    }
    impl_ = std::make_unique<Impl>(rowCapacity, settings, std::move(rowFunction));
}

RowPartitions::~RowPartitions() = default;

RowPartitions::RunStatus RowPartitions::tryRun(std::size_t rows)
{
    return impl_->tryRun(rows);
}
}
