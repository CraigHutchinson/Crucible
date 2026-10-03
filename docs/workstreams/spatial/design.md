# Complete stable spatial bins

Simulation owns Grid and its startup-sized storage. Stable SampleId/Position inputs
are copied, validated, ID-sorted and clamped to GridConfig's closed physical rectangle.
The committed bins use pinned Sub0HexGrid H2 pointy geometry when the checked covering
region and startup scaling guard are supported; private rectangular compatibility
indexing preserves existing geometry/environment domains. See the [receiving decision](decisions.md)
for the coverage proof, memory guard and publication/rounding policy.

No ECS row pointers or handles enter bins. Duplicate IDs, excess capacity and nonfinite
positions reject before changing committed state. Pending cell mappings commit with
samples; there is no failure after publication. Counts/offsets/scatter and output IDs
reuse owned arrays. Occupancy is a representation diagnostic, not a gameplay quantity.

Queries clamp centers consistently. Every finite nonnegative float radius is accepted;
nonfinite/negative input rejects. H2 candidates are conservative r/q ranges; exact double
squared-distance filtering removes false positives and complete results sort ascending
SampleId. Unsupported candidate arithmetic or nonnearest querying of hex bins scans
committed samples using the same predicate. Radius zero includes coincidence.

Returned spans expire on the next query or rebuild. Consume every result immediately
before the next query; Grid needs exclusive access. Synchronous tick-start gather,
complete rebuild and immediate steering consumption remain coordinator responsibilities.
No input borrow survives a call; no hidden scheduler/cursor changes the tick contract.
