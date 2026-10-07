# Reference challenge: recover biomass before the deadline

2026-10-03. A consumed mission increment before relay protection and structural fusion.

The desktop reference arena uses its existing 64×32 grid, 2,048 fixed nanites, four
field slots and finite resource rules. Recover 1,780 biomass quanta (from 8,192
initial substrate stock) by completed tick 900 (15 simulation seconds at 60 Hz).
The initial 6,144 quota was rejected by receiving experiments: passive recovery was
1,774 and tested routes reached at most 1,969 (the latter used a larger radius than
the live tool). The final tutorial quota is 1,780; an attractor swept through the
actual radius-8/strength-4 tool positions reached 1,794 in the pre-tuning tick-900
probe. With final settings the production CLI wins at tick 267 with 1,780 reclaimed
and five applied edits; the passive run loses at tick 900 with 1,774. Both full
states independently replay. [Raw evidence](../workstreams/integration/phase5-evidence/metadata.json)
establishes a consumed winning route and passive loss, not human-playtested difficulty. Quota/deadline
are configurable for deterministic receiving fixtures; zero values and a quota
greater than available startup substrate are invalid. Existing inspector consumers
can omit a mission and retain their previous behavior.

Progress is current reserve minus startup reserve. It measures actual recovered
substrate, not infection-free area or externally supplied mobile biomass. This
increment adds no spending, growth, attrition or structural rules; no existing
biomass is created, destroyed or reclassified. Initial progress is zero at tick 0.

Evaluate after each successfully completed tick. Reaching quota wins, including
on the deadline tick; otherwise reaching the deadline loses. Latch the first
terminal outcome and its frame, close admission, discard carried time and stop
further ticks. Never run past a terminal boundary during four-tick catch-up. Pause
and failed/blocked boundaries cannot advance progress or the deadline. Resume cannot
revive a terminal run. Restart constructs a complete replacement before retiring the
old run, resetting mission, clock, frame, queue and trace.

The mission HUD shows recover/target, ticks left, instructions and textual ACTIVE,
WON or LOST alongside shape/color feedback. Field previews still distinguish queued
intent from applied edits. Outcome controls retain camera inspection and restart;
field editing is denied with explicit feedback. Human comprehension/difficulty and
final typography require a later playtest. This is not completion of Secure the
Relay: lattice/fusion/protected duration and defeat by viable mass remain open.
