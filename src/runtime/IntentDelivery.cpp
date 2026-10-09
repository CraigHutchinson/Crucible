#include <crucible/runtime/IntentDelivery.hpp>

#include <sub0pub/broker.hpp>
#include <exception>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace crucible::runtime {
namespace {
// A lock policy defers registration until the most-derived sink is ready.
// The adapter itself still requires one coordinator: its receipt is reused.
struct BrokerLock {
    std::mutex mutex;
    void lock() { mutex.lock(); }
    void unlock() noexcept { mutex.unlock(); }
};

struct IntentBatch {
    using sub0_config = sub0::config<sub0::Scoped, sub0::Capacity<1>,
        sub0::NoFilter, sub0::LockWith<BrokerLock>>;
    std::uint64_t run_id{}, request_id{};
    std::span<const BoundaryCommand> commands;
};

struct AdmissionSink final : sub0::Subscribe<IntentBatch> {
    CommandIngress& ingress;
    IntentDelivery::Receipt receipt{};
    std::exception_ptr failure;

    AdmissionSink(sub0::Domain<IntentBatch>& domain, CommandIngress& queue)
        : sub0::Subscribe<IntentBatch>{domain}, ingress{queue} {
        if (trySubscribe() != sub0::SubscribeResult::Subscribed)
            throw std::runtime_error("intent sink registration failed");
    }
    ~AdmissionSink() { unsubscribe(); }
    void receive(const IntentBatch& batch) noexcept override {
        try {
            receipt = {batch.run_id, batch.request_id, ingress.TryAdmitCommands(batch.commands)};
        } catch (...) {
            failure = std::current_exception();
        }
    }
};

struct IntentSource final : sub0::Publish<IntentBatch> {
    explicit IntentSource(sub0::Domain<IntentBatch>& domain)
        : sub0::Publish<IntentBatch>{domain} {}
    void Send(const IntentBatch& batch) { sub0::publish(*this, batch); }
};
}

struct IntentDelivery::Impl {
    sub0::Domain<IntentBatch> domain;
    AdmissionSink sink;
    IntentSource source;
    const std::uint64_t run_id;
    std::uint64_t request_id{};

    Impl(CommandIngress& ingress, std::uint64_t run)
        : sink{domain, ingress}, source{domain}, run_id{run} {}
};

IntentDelivery::IntentDelivery(CommandIngress& ingress, std::uint64_t run_id) {
    if (run_id == 0) throw std::invalid_argument("intent run ID must be nonzero");
    impl_ = std::make_unique<Impl>(ingress, run_id);
}
IntentDelivery::~IntentDelivery() = default;

IntentDelivery::Receipt IntentDelivery::TryAdmitCommands(
    std::span<const BoundaryCommand> commands) {
    if (impl_->request_id == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("intent request ID exhausted");
    const auto request_id = ++impl_->request_id;
    impl_->sink.failure = {};
    impl_->source.Send({impl_->run_id, request_id, commands});
    if (impl_->sink.failure) std::rethrow_exception(impl_->sink.failure);
    if (impl_->sink.receipt.run_id != impl_->run_id ||
        impl_->sink.receipt.request_id != request_id)
        throw std::logic_error("intent request had no admission sink");
    return impl_->sink.receipt;
}

}
