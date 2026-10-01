# Bounded elapsed-time clock increment

Worker base: `a4e83f0`, branch `phase2-runtime`. API and arithmetic were frozen
with the architect before authoring. `ClockDriver` is consumed by the architect's
headless main; it exclusively borrows `HeadlessSession` and allocates no storage.

## Decision

Elapsed input is signed `std::chrono::nanoseconds`. Internally one nanosecond
contributes 60 integer units and a tick costs exactly 1,000,000,000 units. Thus
50 ms produces exactly three ticks with no rounded 16,666,667 ns period drift.
At most four successful boundaries run per pump. After those boundaries, whole
overdue ticks are discarded and only the fractional remainder carries. Each
boundary still captures its own ingress cutoff through `HeadlessSession`.

Negative durations and durations larger than `(UINT64_MAX - remainder) / 60`
nanoseconds are rejected before mutation. Unsupported huge durations are not
silently clamped. Discarded-time accumulation saturates at UINT64_MAX; its
explicitly named units are 1/60 nanosecond. No simulation-time counter derives
from discarded or rejected elapsed time.

Pause discards the existing fraction; valid elapsed supplied while paused is
counted as discarded play time. Resume has no pending time and callers must
establish a fresh outer steady-clock baseline. The driver stores no wall-clock
timestamp. Close discards backlog, closes admission and is idempotent. Pumping a
closed or blocked driver adds no elapsed time. Invalid input leaves state and
counters unchanged even when stopped. Pause/resume cannot unblock a failure.

A rejected boundary latches blocked, leaves its clock time unconsumed and does
not record a completed tick. Closed ingress instead latches closed when a due
boundary observes it. A throwing boundary latches blocked before rethrowing;
the session already closes ingress/stops on a throwing simulation tick. There
is no repeated boundary retry after trace exhaustion. Closing a blocked driver
explicitly discards its retained backlog. Existing session failure semantics
permit partial field application before an unexpected failure, without a
completed trace record; the driver makes no rollback promise.

## Consumed observations and ownership

The owned summary contains completed tick, completed trace command count,
ingress pending/accepted/rejected counts and discarded time. Completed command
count deliberately excludes edits at failed boundaries. Ingress statistics are
one mutex-protected sample; concurrent producers can change them immediately
after sampling. No ECS borrow, scratch span, telemetry framework or adapter is
published. `HeadlessSession::GetCompletedTick` is the sole new session observer.

Exact command-trace replay remains a separate session operation, independent of
wall time. The exclusive driver borrow ends before direct session lifecycle,
step or replay use. The architect supplies the main caller and complete owned
state comparisons across clock schedules and replay.

## Review and requested validation

Loaded `cpp-write` and idiom/modernisation/commenting/anti-pattern references
before authoring, then `cpp-review` for the owned diff. No module AGENTS overlay
or organisation type overlay exists. Review covered ownership, minimal consumed
surface, checked arithmetic, nodiscard failures, includes and startup-only
allocation. No remaining owned MUST finding identified. Production caller is
an integration gate owned by the architect, not claimed in this worker tree.

`runtime_clock` fixtures cover zero/fractional/exact thresholds, four-tick cap,
whole-time discard with fractional carry, negative/multiply/addition overflow,
counter saturation, idempotent lifecycle, bounded input rejection, completed
command observations, trace-full blocked latch, external ingress close and
equivalent tick state for two elapsed schedules. Existing `runtime_ingress`
pins late cutoff arrivals and `runtime_session` pins exact trace replay.
Unexpected application failure and tick exceptions have no injection fixture:
valid admitted edits should apply; no test-only fault API was added. The blocked
branch is exercised with real trace exhaustion; root reviews the throwing path.

Worker ran `git diff --check` only (passed). No configure, build, CTest, sanitizer,
benchmark, install or Git write was performed. Requested architect commands:
Debug/Release preset configure/build, `ctest --preset debug -L runtime
--no-tests=error`, unfiltered combined Debug/Release suites and supported
ASan/UBSan suite. Full-state clock/trace oracle is architect-owned. This is a
review-ready bounded increment, not a completed W2/W8a or throughput claim.

## Integrated root verification

The REQUESTED handoff checks above are now fulfilled: combined unfiltered MSVC
Debug/Release and GCC ASan/UBSan each passed 13/13. Full-state schedule/replay
checks run at populations 0/8/2048. See [phase 2 evidence](../integration/phase2-validation.md)
for commits, root review fixes, CLI/export consumer and remaining gates. No performance
claim follows from these functional checks.
