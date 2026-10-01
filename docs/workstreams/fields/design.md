# Bounded radial fields

Simulation owns fixed slots initialized at startup. Runtime commands carry values,
not pointers. Set upserts one slot; remove empties it idempotently. A slot outside the
fixed capacity or a nonfinite/negative-radius set fails without mutation. A full field
array remains valid: replacing an occupied slot succeeds and no slot is appended.

Positive strength attracts toward the center; negative strength repels. At distance d
inside a positive radius r, contribution is strength * (1-d/r) * unit(center-position).
Outside the radius it is zero. Radius zero and coincident center samples contribute
zero, avoiding a direction singularity. This is a chosen prototype radial rule,
not a port of an external algorithm. Centers and sample positions are finite values.

Accumulate in double, clamp each output component to finite float range. Nonfinite
sample positions return zero. Simulation applies commands only at the boundary and
samples the immutable field slots during its sequential tick. No field operation
allocates after construction. Analytic tests pin sign, falloff, singularities, edits,
capacity and finite rejection. Architect performs builds and combined acceptance.
