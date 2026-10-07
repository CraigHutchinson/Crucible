# Phase 8: straight flow as a playable primitive

2026-10-03. Architect freeze for the production desktop/simulation consumer.
Baseline: PR19 merge `50f61d89bb1324d2f3bdaa09e1b16bcf34fec6d9`.
The user prioritizes a playable mock-up before fidelity; new concept art is a later
visual target. This increment adds one straight drag current and readable radial
cues, not arbitrary painted curves, faction combat or a new renderer.

## Command and force

Append `set_flow` after existing FieldEditKind values and append `Position end{}`
to FieldEdit. Preserve existing aggregate initializers and enum values. For flow,
center is A and end is B; both must be finite and distinct. Radius and strength
must be finite and nonnegative. Zero radius/strength is an inactive contribution.
Radial set and remove ignore end; copied radial/remove observations canonicalize
it to zero. Slots retain their kind and endpoint. Four startup-owned slots remain
shared across attract, repel and flow, with atomic replace and idempotent erase.
Field count is not faction count. No new command queue, scheduler or ECS owner.

For a finite position P, compute Q, the closest point on bounded segment AB. The
contribution is `unit(B-A) * strength * max(0, 1-distance(P,Q)/radius)`.
Use double intermediates before subtraction, dot products and normalization;
there is no epsilon/implicit minimum segment length. The capsule's end caps keep
pushing along AB, not toward B. Boundary/outside positions contribute zero.
Accumulate flow and radial fields in stable slot order; retain finite component
saturation and existing steering acceleration/speed caps. Sampling allocates no
storage. Invalid edits preserve every slot; insufficient observation output is
unchanged. This is a force field, not a traversable path or collision constraint.

Independent oracle: A=(2,3), B=(10,3), radius4, strength8. At (6,3) the acceleration
is (8,0); at (6,5), (0,3) and (12,3) it is (4,0); at (6,7) it is zero. Reversing
endpoints reverses the vector. Receive vertical/diagonal, overlap, extreme finite
coordinates, nonfinite rejection, zero contribution and allocation/capacity cases.

## Desktop admission and feedback

Select FLOW with key4 or a visible toolbar button. Left-down inside the arena
captures an owned world start. Drag previews the current segment; left-up inside
the arena admits exactly one valid command. Positive UI radius8/strength4 are the
initial fixed tuning, matching existing tool magnitudes. A zero-length drag or
outside release cancels without admission. Escape, focus loss, suspend, restart,
tool/slot change and starting camera manipulation cancel the unfinished gesture.
Middle-drag remains pan. Coordinates use existing logical/window/camera conversion.
No per-motion admission or borrowed event storage. Preview never mutates fields.

One accepted command remains queued until the completed boundary, including while
paused. Solid committed geometry comes only from the owned snapshot. Dashed preview,
selected slot and queued/applied/rejected messages must remain distinct. Full/invalid
admission retains owned preview for retry; closing/winning/losing disables editing.
Restart clears all flow state. Receive event cancellation, DPI/letterbox conversion,
paused admission, terminal state, overflow, replace/erase and live-versus-replay.

## Primitive visual receiving

Draw a clipped straight corridor with directional chevrons for committed flow;
selected and preview states remain inspectable. Radial attract/repel receive inward/
outward shape cues independent of color. Zero-strength/radius and unused slots draw
no directional effect. Clip in double before narrowing SDL geometry. Use existing
startup capacity and concrete software painter; no textures, meshes, new GPU path,
entity Renderable interface or live ECS queries. Actual before/after production
painter captures demonstrate geometry/preview at fit and zoom; native event tests
prove the input consumer. Do not infer human comprehension from pixel fixtures.

## Replay, exports and retained policy

All full-state field comparisons must include kind, slot, center, radius, strength
and end. Existing ingress/replay owns complete commands; add flow-specific replay,
source-mutation and retained-snapshot fixtures. Audit SVG/JSON exports so flow is
never serialized as remove. Legacy radial mission/replay cases remain unchanged.
Current mission stays 2048 samples, quota1780/deadline900; conserved resource policy
stays initial = stock + mobile + reserve. New flow changes player strategies, not
these rules or automatic mission tuning.

Resource/faction/deathmatch art is explicitly proposed. Four color+badge examples
are presentation studies, not a Faction type, hostility rule or multiplayer proof.
Deathmatch needs a separate consumed rules spike: kind/faction/controller identity,
relations/authority, commands for both kinds, shared/per-owner ledger, contested
ordering, combat/attrition/loss, elimination/ties and full-state replay. Hardware
receiving/shader shootout remains in HW-02..HW-07; no physical inventory is invented.
