# Blight design and handoff

`crucible::blight::Grid` owns two byte-per-cell buffers allocated at construction.
It validates the accepted GridConfig geometry and each buffer's vector max_size
before allocation; allocation failure propagates normally. There is no configurable
memory budget in this module: scenario admission owns that aggregate budget.

TrySeed and Step update an exact cached infection count. Step uses only the original
current values and overwrites every next value before swapping, with no allocations.
No buffer views escape. Calls are coordinated sequentially; simultaneous mutation
and observation are unsupported. Grid is explicitly noncopyable and nonmovable;
its Simulation owner keeps it at a stable address.

The named production caller is the architect's optional Simulation scenario:
construct from its GridConfig, seed columns/2, rows/2 at startup, Step each tick,
and report GetInfectedCount in headless output. The prototype rule and manually
derived fixture counts are in decisions.md.

## Staged consumption

`TryPrepareStep` computes the same cardinal rule into the existing next buffer and
returns a move-only exclusive lease. Only that lease can clear pending infection;
committed observation remains unchanged until a live rvalue Commit succeeds.
Destruction abandons pending changes, move transfers ownership, and Commit consumes
ownership so a consumed/moved-from destructor cannot release a newer lease.

Second preparation is rejected; TrySeed rejects while leased. Legacy Step now throws
logic_error if a lease is active instead of interfering with pending state. With no
lease its evolution remains unchanged and does not allocate. Grid must outlive the
lease and callers coordinate sequentially. Prepared-lifetime fixtures prove abandonment,
moves, invalid/consumed commit, pending clear, exact cached count and lease exclusion.
