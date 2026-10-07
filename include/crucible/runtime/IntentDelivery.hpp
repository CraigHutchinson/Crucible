#pragma once

#include <crucible/runtime/CommandIngress.hpp>
#include <memory>

namespace crucible::runtime {

/** One synchronous typed Pub route into an existing owned command queue.
 * Coordinator-thread only: admission calls and destruction must not overlap.
 * The ingress must outlive this object. Pub borrows caller input only during
 * the call; the sink copies the whole accepted batch into CommandIngress.
 * No callback mutates world state or retains caller input.
 */
class IntentDelivery {
public:
    /// Owned result of one request; queue admission is not boundary application.
    struct Receipt {
        std::uint64_t run_id{}, request_id{};
        CommandIngress::Admission admission{};
    };

    /// run_id is nonzero and chosen by the owner, distinct across its restarts.
    explicit IntentDelivery(CommandIngress& ingress, std::uint64_t run_id);
    ~IntentDelivery();
    IntentDelivery(const IntentDelivery&) = delete;
    IntentDelivery& operator=(const IntentDelivery&) = delete;

    /// Every request gets a new ID, including refused batches. Exhaustion throws
    /// overflow_error before delivery; IDs never wrap or reuse within this run.
    [[nodiscard]] Receipt TryAdmitCommands(std::span<const BoundaryCommand> commands);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
