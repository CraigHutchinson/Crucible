# Rectangular stable sample bins

Simulation owns a Grid and its startup-sized storage. It rebuilds from gathered stable
SampleId/Position values and immediately consumes query counts as headless diagnostics.
No ECS pointers or row offsets enter the grid. The constructor validates rectangular
geometry and capacity arithmetic; invalid input throws before a usable object exists.

Finite positions clamp to the closed rectangular world. Cell membership clamps the
upper edge to the final cell. Rebuild rejects capacity excess, nonfinite positions and
duplicate IDs before replacing committed samples. IDs order every query ascending,
independent of input order or cell traversal. Radius is finite and nonnegative; every
intersecting cell is searched and exact squared distance filters candidates. Radius
zero includes coincident samples. Query centers clamp consistently with samples.

All arrays are allocated at construction. Rebuild uses a second preallocated sample
array for validation/sorting before commit; counts, offsets, bins and query IDs are
reused. Query returns an immutable span into owned scratch, consumed immediately;
it expires on the next query or rebuild. The object requires exclusive caller access.

Validation fixtures compare complete IDs against brute force over dense, coincident,
border, empty and rectangular inputs, shuffled IDs and arbitrary multi-cell radii.
No builds or measurements run in this worker; architect owns serialized checks.
