# Nanite swarm intro spike

Headless, deterministic prototype of the twelve-second CRUCIBLE title sequence's swarm
behaviour: building blocks S0 to S0c of the
[nanite swarm design](../../docs/concepts/phase15-experience/nanite-swarm-design.md).
It answers a narrow question: **can a few thousand view-owned nanites perform the
director's beats with intelligent-looking behaviour near the camera, cheap non-penetrating
motion elsewhere, and a deterministic outcome that does not depend on the level of detail?**
User direction (2026-10-12) adds that the final title **stands upright**; a ground-based
build is acceptable for early milestones.

It is not production rendering, audio, a gameplay system or a native performance budget.
The images are concept2D/dev-test captures through stand-in cameras: top-down orthographic
for the lying build, and an oblique front view (screen height = z + 0.4 × depth) for the
upright builds.

## Three builds

| `--build` | Stage | How the title forms | Ends |
|---|---|---|---|
| `lying` (default) | S0 | Units assemble the title flat on the substrate. | Lying title |
| `climb` | S0b | Letters grow upright in place, bottom-up. A slot can be filled only when the ground or a locked lower neighbour supports it. Units queue in front of the wall, step onto the wall face at its base and climb over the built units to their slot. | Upright, about 13.5 s |
| `raise` | S0c | Each letter assembles flat behind its baseline. As soon as its last unit locks, the interlocked letter hinges up on its bottom row like a drawbridge, letters rising one after another. | Upright, about 9.0 s |

The climb build runs on two connected surfaces: the ground and the wall face. The wall face
is stored as a second interaction surface, offset so the two never interact, and joined to
the ground along the wall's base. That is the "surface" interpretation of upright
assembly. Full three-axis interlocking (units attaching on any face, overhangs, depth) is a
later stage in the design.

## What it does

A view-owned Sub0ECS world holds `Identity`, `Pose`, `Motion` and `Mind` components plus one
LOD tier tag (`HeroTier`, `NearTier`, `FarTier`, `HiddenTier`). Each tier tag is a separate
partition, so a tier's system iterates only that tier. Systems run in a fixed order
on a frozen tick-start gather. Tier migration, a structural change, happens only after
every system has finished.

| System | Behaviour | Prior art |
|---|---|---|
| Planner (load time) | Predicts the answer wave by BFS hop count from the first wedge. Splits units over three lanes by bed position. Pairs wake order with slot order: far-first when lying; bottom-up by build depth, then far-first, when climbing. | Kilobot seed gradient; CAPT goal assignment |
| Build graph (load time) | Links slots of a letter within 1.6 pitch, gives each a depth from the letter's ground row (multi-source BFS) and an entry point on the wall base. | TERMES-style structure compilation |
| Signal | A dormant unit wakes once a neighbour has been awake for one hop delay: the traveling answer of shot02. | Kilobot gradient |
| Behaviour: Hero | Sampled velocity obstacles against senior neighbours (right of way by wake order). Turns toward its goal before moving (anticipation), climbs over resting units, docks with a ritual. At most `heroCap` units. | RVO / ORCA, Disney animation principles |
| Behaviour: Near | Desired velocity plus reactive separation; climbs over resting units. | Reynolds boids |
| Behaviour: Far | Desired velocity plus an O(1) per-cell density-gradient push. | Continuum crowds |
| Behaviour: Hidden | Analytic goal motion; no neighbour queries. | Crowd LOD |
| Climb (climb build) | A unit waits in a staging band until its slot is within `climbWindow` (12) levels of its letter's built frontier. It then steps onto the wall base nearest its slot. It routes over the letter's locked and nearly seated units by shortest path, with the ground linking separate strokes. Near its slot it turns to the stroke's heading. It slides in once a lower neighbour is seated, and locks only when that support is locked. | M-Blocks pivoting, Big Hero 6 microbots |
| Negotiation | Neighbouring seekers in the same lane swap slots when that lowers summed squared distance (climbing: same depth only). | d-CAPT |
| Resolve | Six Jacobi position-projection passes over visible ground movers. Resting units (dormant, docking, locked, moored) form a layer movers glide over. Climbers flow over the face and over each other in layers instead of colliding. | PBD; Big Hero 6 microbots sliding over one another |
| LOD | Tier per unit from the scripted camera, using each build's view projection. Hidden outside the view plus margin; hero near the focus (capped, nearest first); near or far by screen size and radius. Hysteresis of six ticks; leaving Hidden is immediate. | Crowd LOD |

Acts: `Dormant → Waking → Lane → Seek → (Climb →) Dock → Locked` for title units,
`Waking → Moor → Dock → Moored` for ribbon units, `Waking → Reserve` for surplus.

## Build and run

```sh
cmake --preset release -DCRUCIBLE_BUILD_SWARM_SPIKE=ON
cmake --build --preset release --target crucible_swarm_intro
./build/release/spikes/swarm_intro/crucible_swarm_intro --verify --build raise   # CTest: swarm_intro_{lying,raise,climb}
./build/release/spikes/swarm_intro/crucible_swarm_intro --build raise --frames frames --every 4 --metrics metrics.csv
python3 spikes/swarm_intro/render_swarm.py frames --sheet beats.svg --tiers tiers.svg --gif intro.gif
```

Options: `--build lying|climb|raise`, `--count N` (default 1536), `--hero-cap N` (128),
`--lod camera|hero|near|far`, `--climb-window N` (12), `--seconds S`. The JSON summary line
reports completion and raise times, visible contacts, unsupported locks, off-surface
climbers, tier counts, hero yields, goal swaps, dock clicks and per-system host wall time.

## Acceptance (`--verify`)

The lying and raise gates come from the director brief and were written before tuning.
The contact gate was set after observing glancing contacts. The climb gates were set after
measuring that bottom-up climbing cannot meet the brief's 8.4 s. Both are recorded as such.

- All builds: no lock without a locked support; hero tier within its cap; ribbon fully
  moored by 10.0 s. Two identical runs produce the same state hash (sampled once per
  second). A uniform far tier completes the title as well.
- Lying and raise: title at least 99% locked by 8.4 s (shot04 exit) and 100% by 10.0 s.
  Visible mover contacts (centres closer than 0.8 diameter) at most 3% of visible movers in
  any tick, and no visible pair closer than 0.5 diameter: wedges may touch, never pass
  through. Raise: every letter upright by 10.0 s (end of shot05).
- Lying: a uniform hero tier completes the title by 11.0 s.
- Climb (16 s run): title complete by 15.0 s. At most 200 off-surface climber-ticks.
  Visible ground contacts at most 8% and no visible pair closer than 0.3 diameter, looser
  because of the staging crowd in front of the wall.

## Evidence, 2026-10-12

Host: shared 4-vCPU Intel Xeon 2.80 GHz cloud container, GCC 13.3, single thread. Costs are
advisory host wall time with diagnostics excluded, not a device budget. 959 title slots.

| Run | Title complete | Upright | Worst contacts / min gap | Tick p50 / p95 |
|---|---|---|---|---|
| Lying, camera LOD, 1536 | 7.77 s | – | 1.3% / 0.64 d | 0.22 / 2.02 ms |
| Lying, uniform near | 7.90 s | – | 1.4% / 0.60 d | 0.38 / 1.77 ms |
| Lying, uniform far | 7.73 s | – | 2.3% / 0.62 d | 0.30 / 1.49 ms |
| Lying, uniform hero | 10.47 s | – | 0.0% / 0.81 d | 1.36 / 5.73 ms |
| Raise, camera LOD, 1536 | 7.97 s | 9.02 s | 1.4% / 0.62 d | 0.22 / 2.24 ms |
| Raise, 4096 | 7.92 s | 8.97 s | 1.6% / 0.59 d | 0.58 / 2.83 ms |
| Raise, 16384 | 7.93 s | 8.98 s | 1.8% / 0.60 d | 3.87 / 8.35 ms |
| Climb, camera LOD, 1536 | 13.45 s | 13.45 s | 5.6% / 0.38 d | 0.98 / 4.22 ms |
| Climb, 4096 | 14.23 s | 14.23 s | 9.2% / 0.38 d | 1.58 / 5.26 ms |
| Climb, 16384 | 13.80 s | 13.80 s | 7.7% / 0.33 d | 3.97 / 7.29 ms |

Release, Debug and ASan/UBSan builds all pass `--verify` for every build. They produce the
same state hashes with this compiler (16 s runs, 1536 units: lying `bf4217b304a54a77`,
raise `1618fdd10c3f283e`, climb `701355b230c8422d`). Cross-compiler or cross-platform bit
equality is not claimed.

### Lying (S0)

![Lying build: six beats](evidence/beats.svg)

Beats at 1.0, 3.0, 5.0, 6.8, 8.4 and 11.9 s: one wedge wakes; the answer wave spreads and
units turn toward their lanes; three lanes; letters fill far-first while the surplus
leaves for the ribbon; title complete; camera opens.

![Lying build: LOD tiers](evidence/tiers.svg)

Same frames coloured by decision tier: amber hero, cyan near, violet far; hidden units
are not drawn. Heroes follow the camera's focus; the counts are in each tile's label.

### Raise (S0c)

![Raise build: six beats](evidence/raise.svg)

At 5.0, 6.8, 7.6, 8.4, 9.2 and 11.9 s through the oblique camera: lanes feed letters lying
flat; letters near completion; the first C has risen while the rest are still flat; most
letters upright, the second C rising; all upright; camera opens.

### Climb (S0b)

![Climb build: six beats](evidence/climb.svg)

At 5.0, 7.0, 9.0, 10.5, 12.0 and 13.9 s: footings and lower strokes; letters growing
bottom-up with climbers flowing over the face; readable but incomplete; tops and arms
filling; nearly complete; complete.

## Findings

1. **Full intelligence everywhere is slower, costlier and can freeze.** With reciprocal
   avoidance on every unit, an early uniform-hero run reached only 82% of the title by 10 s:
   units kept yielding to each other and dense streams stalled. Yielding only to senior
   units (right of way by wake order) fixed the stall. Uniform hero is still 2.7 s slower
   than the camera-LOD run, at about six times its median cost. Heroes should be few, and
   the reactive stream around them carries them.
2. **Lane throughput sets the lying timeline.** Assembly time is bounded by lane width times
   cruise speed. 30 diameters/s with 8-diameter lanes meets the 8.4 s exit. That pace is
   tuned; a perspective camera needs its own review.
3. **Decentralised slot swaps remove most crossings.** About 5,900 local swaps per lying run.
   In S0 they lowered worst contacts from 3.3% to 1.9% and finished 0.35 s sooner (measured
   at 28 diameters/s, before the right-of-way change).
4. **The signal system dominates at 16K** (2.0 ms mean, lying build), because every dormant
   unit polls its neighbours each tick. An event-driven wave front is the next optimisation.
5. **Dock clicks need aggregation.** Up to 78 locks fall in one 100 ms window. Audio must
   follow the brief's sparse authored beats, not one sound per lock.
6. **Hidden units interpenetrate by design.** The resolve pass covers only visible ground
   tiers. Units entering the view margin are promoted at once and resolve before they reach
   the frame edge.
7. **Honest climbing is about 5 s too slow for the brief.** With a support rule, assembly
   time follows the deepest build chain (22 levels, the E's top arm) times the latency per
   level. After the fixes below, levels lock about 0.38 s apart and the title completes at
   about 13.5 s; the brief's shot04 exit is 8.4 s. Faster docking barely changed it. The
   remaining limit is getting the right unit next to each newly supported slot.
8. **Climbing needed five rules before it worked**, each found by measuring a stall:
   - Which surface a unit is on must follow its state, not its position; otherwise
     projection pushed climbers into the gap between the surfaces.
   - Units must queue on the ground, in a staging band set back by their wait, rather than
     crowd the face or the wall's base.
   - Climbers must flow over each other in layers, as in *Big Hero 6*; colliding on the
     face jammed it.
   - The route is a shortest path over built units, with the ground linking strokes. Hop
     counts sent climbers back and forth along the base.
   - Levels are pipelined: a climber slides in once its support is nearly seated, but
     locks only on a locked support.
9. **Raise keeps the timing and the upright ending.** Assembling flat and hinging each letter
   up costs no extra simulation (the rotation is a pose transform of locked units). Letters
   rise in completion order, and every letter is upright by 9.0 s. That is 0.6 s after the
   brief's shot04 exit, inside shot05's hold.
10. **The camera needs a raise beat.** In the oblique view a flat letter is foreshortened to
    0.4 of its height, so the title reads only as letters rise. The beat would replace the
    brief's "camera rises toward a nearly frontal baseline" with "letters rise to face the
    camera". This is a director decision, not a technical one.

## Limits

Orthographic top-down and oblique 2D stand-in cameras; no perspective, depth of field,
materials, bloom or real wedge mesh. One fixed seed. Glyphs are a prototype stroke font with
square caps, not the licensed wordmark asset. The planner and build graph run at load.

The raise hinge is a rigid pose transform: nothing yet shows what lifts the letter.
Climbers stack in layers on the face; only ground movers are held apart. The climb build's
ground staging crowd keeps contacts above the lying build's.

Skip, reduced motion, resize and focus behaviour belong to the frontend package and are
not exercised. No GPU, audio or participant evidence.
