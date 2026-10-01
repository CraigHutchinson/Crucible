# Runtime implementation

Own explicit source registration in CMakeLists.txt. See [workstream brief](../../docs/workstreams/runtime/README.md).

Sources implement the compatibility runner, mutex-protected owned command ring
and bounded sequential session/replay. See stream validation for integration gates;
`ClockDriver` supplies bounded injected-time orchestration; architect production
wiring and combined verification remain gates. Pipeline orchestration is pending.
