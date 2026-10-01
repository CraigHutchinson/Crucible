# Bounded Blight prototype rule

Decision recorded before implementation, 2026-10-01.

A cell is infected at the next tick exactly when it is infected now or any in-bounds
north, south, east or west neighbor is infected now. There is no diagonal spread
and no wraparound. Every output reads the same immutable current grid, so infection
moves at most one cardinal edge per tick. Empty grids remain empty; infection is
monotone. Cell coordinates are zero-based column, row.

The coordinator seeds cells at boundaries. Seeding an already infected cell succeeds
without changing the count; out-of-bounds seeding fails without changing state.
Out-of-bounds observation returns false. Step writes the complete separate next
buffer, then swaps it into current. Observers expose values, not mutable buffers.

Hand-calculated fixtures: a 3x3 center seed has counts 1, 5, 9; a 4x2 upper-left
seed has counts 1, 3, 5, 7, 8. A 5x1 left seed has counts 1, 2, 3, 4, 5;
the last cell must remain clear after the first tick. A full grid stays full.

This is a sequential gameplay prototype. Resource quantities, consumption,
interactions and worker partitions are not implemented. W4/W6 acceptance remains
open until production integration and measured acceptance checks exist.
