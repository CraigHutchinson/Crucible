# Nanite swarm intro spike

Headless, deterministic prototype of the twelve-second CRUCIBLE title sequence's swarm
behaviour: the building block S0 of the
[nanite swarm design](../../docs/concepts/phase15-experience/nanite-swarm-design.md).
It answers a narrow question: **can a few thousand view-owned nanites perform the
director's beats with intelligent-looking behaviour near the camera, cheap non-penetrating
motion elsewhere, and a deterministic outcome that does not depend on the level of detail?**

It is not production rendering, audio, a gameplay system or a native performance budget.
The images are concept2D/dev-test captures through an orthographic stand-in camera.

## What it does

A view-owned Sub0ECS world holds `Identity`, `Pose`, `Motion` and `Mind` components plus one
LOD tier tag (`HeroTier`, `NearTier`, `FarTier`, `HiddenTier`). Each tier tag is a separate
partition, so a tier's system iterates only that tier. Systems run in a fixed order
on a frozen tick-start gather; tier migration (a structural change) happens only after
every system has finished.

| System | Behaviour | Prior art |
|---|---|---|
| Planner (load time) | Predicts the answer wave by BFS hop count from the first wedge; splits units over three lanes by bed position; pairs wake order with far-first slot order. | Kilobot seed gradient; CAPT goal assignment |
| Signal | A dormant unit wakes once a neighbour has been awake for one hop delay: the traveling answer of shot02. | Kilobot gradient |
| Behaviour: Hero | Sampled velocity obstacles against senior neighbours (right of way by wake order), anticipation turn before moving, climb over resting units, docking ritual. At most `heroCap` units. | RVO / ORCA, Disney animation principles |
| Behaviour: Near | Desired velocity plus reactive separation; climbs over resting units. | Reynolds boids |
| Behaviour: Far | Desired velocity plus an O(1) per-cell density-gradient push. | Continuum crowds |
| Behaviour: Hidden | Analytic goal motion; no neighbour queries. | Crowd LOD |
| Negotiation | Neighbouring seekers in the same lane swap slots when that lowers summed squared distance. | d-CAPT |
| Resolve | Four Jacobi position-projection passes over visible movers. Resting units (dormant, docking, locked, moored) form a layer movers glide over. | PBD; Big Hero 6 microbots sliding over one another |
| LOD | Tier per unit from the scripted camera: hidden outside the view plus margin; hero near the focus (capped, nearest first); near or far by screen size and radius. Hysteresis of six ticks; leaving Hidden is immediate. | Crowd LOD |

Acts: `Dormant → Waking → Lane → Seek → Dock → Locked` for title units,
`Waking → Moor → Dock → Moored` for ribbon units, `Waking → Reserve` for surplus.

## Build and run

```sh
cmake --preset release -DCRUCIBLE_BUILD_SWARM_SPIKE=ON
cmake --build --preset release --target crucible_swarm_intro
./build/release/spikes/swarm_intro/crucible_swarm_intro --verify            # also a CTest case
./build/release/spikes/swarm_intro/crucible_swarm_intro --frames frames --every 4 --metrics metrics.csv
python3 spikes/swarm_intro/render_swarm.py frames --sheet beats.svg --tiers tiers.svg --gif intro.gif
```

Options: `--count N` (default 1536), `--hero-cap N` (128), `--lod camera|hero|near|far`,
`--seconds S`. The JSON summary line reports completion times, visible contacts, tier
counts, hero yields, goal swaps, dock clicks and per-system host wall time.

## Acceptance (`--verify`)

Thresholds were set from the director brief before tuning, except the contact gate, which
was set after observing glancing contacts and is recorded as such.

- Title at least 99% locked by 8.4 s (shot04 exit) and 100% by 10.0 s; ribbon fully moored by 10.0 s.
- Hero tier never exceeds its cap.
- Visible mover contacts (centres closer than 0.8 diameter) at most 3% of visible movers in
  any tick, and no visible pair closer than 0.5 diameter: wedges may touch, never pass through.
- Two identical runs produce the same state hash (sampled once per second).
- With a uniform far tier the title completes by 10.0 s; with a uniform hero tier by 11.0 s.

## Evidence, 2026-10-12

Host: shared 4-vCPU Intel Xeon 2.80 GHz cloud container, GCC 13.3, single thread. Costs are
advisory host wall time with diagnostics excluded, not a device budget.

| Run | Title complete | Ribbon at 10 s | Worst contacts / min gap | Tick p50 / p95 |
|---|---|---|---|---|
| Camera LOD, 1536 | 8.28 s | 100% | 2.4% / 0.62 d | 0.29 / 1.65 ms |
| Uniform near | 8.35 s | 100% | 2.4% / 0.60 d | 0.32 / 1.26 ms |
| Uniform far | 8.35 s | 100% | 4.2% / 0.48 d | 0.25 / 1.09 ms |
| Uniform hero | 10.35 s | 98.9% | 0.8% / 0.70 d | 1.14 / 4.68 ms |
| Camera LOD, 4096 | 7.92 s | 100% | 2.8% / 0.57 d | 0.51 / 2.37 ms |
| Camera LOD, 16384 | 7.87 s | 100% | 4.1% / 0.50 d | 2.46 / 5.45 ms |

Release, Debug and ASan/UBSan builds all pass `--verify` and produce the same state hash
(`95a41ce1d6713935`, 1536 units) with this compiler; cross-compiler or cross-platform bit
equality is not claimed.

![Six beats](evidence/beats.svg)

Beats at 1.0, 3.0, 5.0, 6.8, 8.4 and 11.9 s: one wedge wakes; the answer wave spreads and
units turn toward their lanes; three lanes; letters fill far-first while the surplus
leaves for the ribbon; title complete; camera opens.

![LOD tiers](evidence/tiers.svg)

Same frames coloured by decision tier: amber hero, cyan near, violet far; hidden units
are not drawn. Heroes follow the camera's focus; the counts are in each tile's label.

## Findings

1. **Full intelligence everywhere is slower, costlier and can freeze.** With reciprocal
   avoidance on every unit, the first uniform-hero run reached only 82% of the title by
   10 s: units kept yielding to each other and dense streams stalled. Yielding only to
   senior units (right of way by wake order) fixed the stall, but uniform hero is still
   2 s slower than the camera-LOD run and about four times its cost. Heroes should be few,
   and the reactive stream around them carries them.
2. **Lane throughput sets the timeline.** Assembly time is bounded by lane width times
   cruise speed. 30 diameters/s with 8-diameter lanes meets the 8.4 s exit; that pace is
   tuned, and a perspective camera needs its own review.
3. **Decentralised slot swaps remove most crossings.** About 4,900 local swaps per run
   lowered worst contacts from 3.3% to 1.9% and finished the title 0.35 s sooner (measured
   at 28 diameters/s, before the right-of-way change).
4. **The signal system dominates at 16K** (1.5 ms mean), because every dormant unit polls
   its neighbours each tick. An event-driven wave front is the next optimisation.
5. **Dock clicks need aggregation.** Up to 55 locks fall in one 100 ms window. Audio must
   follow the brief's sparse authored beats, not one sound per lock.
6. **Hidden units interpenetrate by design.** The resolve pass covers only visible tiers.
   Units entering the view margin are promoted at once and resolve before they reach the
   frame edge.

## Limits

Orthographic top-down 2D stand-in for the lens path; no depth of field, materials, bloom or
real wedge mesh. One fixed seed. Glyphs are a prototype stroke font, not the licensed
wordmark asset. The planner runs at load. Skip, reduced motion, resize and focus behaviour
belong to the frontend package and are not exercised. No GPU, audio or participant evidence.
