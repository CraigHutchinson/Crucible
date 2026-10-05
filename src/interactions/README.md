# Interactions implementation

Own explicit source registration in CMakeLists.txt. See [workstream brief](../../docs/workstreams/interactions/README.md).

Implemented finite stock-to-reserve reclamation, consumed by Simulation.
Stock/infection/ledger publish together in stable cell/ID order; structural
anchor/release transfers, explicit loss and protected pending clearing are now received.
