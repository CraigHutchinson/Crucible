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
