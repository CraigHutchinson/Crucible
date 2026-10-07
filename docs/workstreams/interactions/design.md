# Finite reclamation provider

`interactions::Reclamation` owns uniform integer substrate, its conserved ledger,
a second stock buffer and population-bounded proposals/ID scratch. Construction checks
geometry, positive per-sample mass, integer products/sums and vector limits before
allocation. No public state setter, spawning, loss bucket or structural transition exists.

The receiving caller is the architect-owned resource-enabled `Simulation`: pass fixed
samples with post-move positions to `TryStep`; inspect `GetLedger` and `GetStocks` for
owned snapshots and headless output. Provider commits are not whole-tick ECS rollback.
The caller must stop on rejection and suppress successful completed-tick publication.

`TryStep` requires exactly the startup population and IDs 1..population once each.
It rejects nonfinite positions, clamps finite outside positions, and uses containing
rectangular Blight cells regardless of swarm bin topology. Cell then ID ordering and
successful-action budgets implement the [frozen rule](../../decisions/phase3-resource-rules.md).

Input/geometry validation precedes preparation. Work modifies only pending stocks,
ledger and the exclusive Blight prepared-step lease. Any rejection abandons the
lease and preserves all committed resource/infection values. After validated work,
commit swaps existing infection/stock buffers and assigns the ledger without failure.
All calls are coordinator-exclusive and retain no input borrows. `GetStocks` borrows
expire after a successful step or destruction; the architect copies only after
validating all snapshot capacities. No unused copy convenience API is shipped.
