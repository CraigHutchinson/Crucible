#include <crucible/runtime/BoundaryPipeline.hpp>
#include <sub0pipeline/sub0pipeline.hpp>
#include <expected>
#include <stdexcept>
#include <utility>

namespace crucible::runtime {
struct BoundaryPipeline::Impl {
    using Status = HeadlessSession::StepStatus;
    using JobResult = std::expected<void, sub0pipeline::PipelineError>;

    HeadlessSession& session;
    Simulation& simulation;
    presentation::ScenarioSnapshot first, second;
    presentation::ScenarioSnapshot* published{&first};
    presentation::ScenarioSnapshot* staging{&second};
    Completion completion;
    HeadlessSession::StepResult result{};
    bool failed{}, terminal{};
    // Destroy the graph before the stable context captured by its jobs.
    sub0pipeline::Pipeline graph;

    Impl(HeadlessSession& input, Simulation& world, std::size_t samples,
        std::size_t fields, std::size_t cells, Completion callback)
        : session(input), simulation(world), first(samples, fields, cells),
          second(samples, fields, cells), completion(std::move(callback)) {
        if (!published->TryCapture(simulation))
            throw std::invalid_argument("boundary frame capacity or scenario is invalid");

        const auto boundary = graph.emplace([this]() -> JobResult {
            result = session.TryStep();
            if (result.status != Status::advanced)
                return std::unexpected(sub0pipeline::PipelineError::kCancelled);
            return {};
        }).name("boundary.commit");
        const auto capture = graph.emplace([this]() -> JobResult {
            if (!staging->TryCapture(simulation))
                return std::unexpected(sub0pipeline::PipelineError::kJobFailed);
            return {};
        }).name("frame.capture").succeed(boundary);
        graph.emplace([this]() -> JobResult {
            // Caller output is committed before frame publication. If it throws,
            // the graph fails and the previous published frame remains visible.
            const bool stop = completion && completion(*staging);
            if (stop) session.GetIngress().Close();
            std::swap(published, staging);
            terminal = stop;
            return {};
        }).name("mission.publish").succeed(capture);
    }

    HeadlessSession::StepResult Step() {
        if (failed) return {Status::application_failed, session.GetCompletedTick(), 0};
        if (terminal) return {Status::closed, session.GetCompletedTick(), 0};
        result = {Status::application_failed, session.GetCompletedTick(), 0};
        try {
            const auto run = graph.run_inline();
            if (run) return result;
            if (result.status == Status::paused || result.status == Status::closed ||
                result.status == Status::trace_full) return result;
        } catch (...) {
            // Startup storage exists, but upstream dispatch/run bookkeeping may
            // allocate or throw. Untimed inline execution has no outstanding work.
            failed = true;
            session.GetIngress().Close();
            throw;
        }
        failed = true;
        session.GetIngress().Close();
        if (result.status == Status::advanced) result.status = Status::application_failed;
        result.tick = session.GetCompletedTick();
        return result;
    }
};

BoundaryPipeline::BoundaryPipeline(HeadlessSession& session, Simulation& simulation,
    std::size_t samples, std::size_t fields, std::size_t cells, Completion completion)
    : impl_(std::make_unique<Impl>(session, simulation, samples, fields, cells,
          std::move(completion))) {}
BoundaryPipeline::~BoundaryPipeline() = default;
HeadlessSession::StepResult BoundaryPipeline::TryStep() { return impl_->Step(); }
const presentation::ScenarioSnapshot& BoundaryPipeline::GetFrame() const noexcept {
    return *impl_->published;
}
bool BoundaryPipeline::IsTerminal() const noexcept { return impl_->terminal; }
}
