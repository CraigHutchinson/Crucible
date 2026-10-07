# Completed-boundary reclamation mission

The [phase 5 mission rule](../../decisions/phase5-reclamation.md) is consumed by
the desktop inspector through the optional third `InspectorSession` constructor
argument. Omitted settings preserve the existing inspector and exact replay path.
Mission observations are owned optional values; no renderer or ECS borrow escapes.

Startup validates positive quota/deadline and quota no greater than the initial
ledger's remaining substrate stock. Progress starts at zero and uses current
reserve minus the captured startup reserve, excluding externally supplied biomass.
The shared `Simulation::GetBiomassLedger` returns only an owned ledger value, so
the objective does not require extracting all samples/cells after every tick.

`ClockDriver` optionally owns a startup-allocated completed-boundary callback.
It runs sequentially after each successful tick, including every catch-up tick;
true closes ingress, discards all remaining time and returns immediately. An empty
callback preserves the original clock path. The callback cannot reenter the driver
and its captured state must outlive invocation. Exceptions latch blocked and
propagate after the completed boundary; no rollback is promised. Rejected or paused
boundaries never invoke it.

The inspector run owns simulation, session, mission and callback. Its heap address
remains stable across outer `unique_ptr` replacement; the callback's run borrow
is destroyed before its captured state. Quota is evaluated before deadline, so
success on the deadline wins. Terminal progress latches because closed clocks
cannot resume or invoke another boundary. The retained snapshot is published after
the pump at exactly the terminal/completed tick. Restart allocates and validates a
complete fresh run before closing and replacing the old one.

Independent fixtures cover the reference arena's five tick-one contacts and
thirteen tick-two contacts, exact-deadline win/loss, catch-up stopping, elapsed
schedule equivalence, full-state replay, terminal admission, pause, blocked trace,
restart, owned observations, startup rejection and legal maximum stock quota.
Clock fixtures separately cover stop observation, fractional-time discard and
throwing-callback latching. Existing no-mission clock/inspector fixtures remain.
Unexpected invariant failure and restart allocation failure have no injected
fixture; exception/ownership paths were reviewed without test-only production APIs.

Worker validation is read-only self-review and `git diff --check`; builds, platform
checks and default-mission outcome evaluation belong to the architect. Initial
tuning has no playtest or balanced-difficulty claim.
