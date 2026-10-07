# Runtime implementation

Own explicit source registration in CMakeLists.txt. See [workstream brief](../../docs/workstreams/runtime/README.md).

Sources implement the compatibility runner, mutex-protected owned command ring
and bounded sequential session/replay. See stream validation for integration gates;
`ClockDriver` supplies bounded injected-time orchestration; architect production
wiring and combined verification remain gates. IntentDelivery supplies scoped synchronous Pub admission; BoundaryPipeline orders
commit, owned capture and completion inline. Inspector and CLI consume both.
See [Phase11](../../docs/phases/phase11.md) for failure/lifetime and parity receiving.
