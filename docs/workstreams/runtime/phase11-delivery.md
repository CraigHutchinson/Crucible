# Phase 11: synchronous owned admission through Pub

`IntentDelivery` owns one scoped Sub0Pub domain, one most-derived admission sink
and its publisher. The domain has capacity one; its type is private to the source
file, so callers cannot register another sink or publish into another run. The
public API borrows the existing ingress and takes a nonzero owner-assigned run ID.
It returns an owned receipt containing run ID, monotonically increasing request ID
and the unchanged ingress admission status/sequence range. Queue acceptance is
distinct from later boundary application. Request-ID exhaustion throws before
publication rather than wrapping correlation.

The typed envelope borrows the caller's batch **only during synchronous publish**.
The sole callback immediately calls the ingress's all-or-nothing owned copy; it
retains no span or caller storage and never mutates ECS. This removes a redundant
intermediate batch allocation and preserves validation precedence, oversized/full
rejection and counters exactly. Source mutation after return cannot affect admitted
commands. No reply queue exists; the caller receives a copied value.
Unexpected ingress exceptions are captured by the noexcept callback and rethrown
after synchronous publish returns; they cannot escape through the callback ABI.

The pinned Pub v2 `2cd3daf15e44c9429fd2cfbc4ca2fcb723b77ced` was compared with
v2 discovery head `b1166d908dbbd35eb56ca617bf1ee51d72e6e21f`: broker and config
headers are identical. Latest changes concern IPC, evidence and CI; this local
boundary needs no pin promotion or upstream API change. Its registration result
is checked at startup. A local mutex policy defers registration until the
most-derived sink constructor; the sink explicitly disconnects before derived
destruction. Publisher and sink are destroyed before their domain. An isolated
domain cannot encounter another run's subscription-capacity limit.

Admission and destruction are coordinator-thread operations. Pub's locked broker
does not make the adapter's reusable receipt safe for concurrent callers. Owners
must quiesce callers before teardown and keep ingress alive until route destruction.
Concurrent producer delivery and asynchronous transports remain outside this
increment; adding either requires separate receipt/storage/lifetime receiving.

`tests/runtime/IntentDelivery.cpp` receives two live-domain isolation, field/fuse/
shatter source-copy ownership, owned receipts, full/invalid/closed batch behavior
and statistics against direct ingress, oversized malformed validation precedence,
cutoff preservation and repeated teardown/replacement. Production wiring and
Debug/Release/sanitizer execution are integrator-owned gates, not claimed here.
No visual changes arise from this routing adapter; the integrated route's actual
owned-frame exports are phase-level evidence.
