# Phase 8 desktop flow handoff

Status: source handoff; execution and captures remain architect receiving. This
worker ran no builds or tests. The [frozen decision](../../decisions/phase8-flow.md)
controls geometry and input; [phase plan](../../phases/phase8.md) records ownership.

The production caller is DesktopApp: key4 or the appended FLOW toolbar button
selects a straight current. Left-down captures the world start, pointer motion
updates an owned preview, and valid inside release admits exactly one complete
FieldEdit. The fixed UI radius8/magnitude4 and four shared slots are unchanged.
Middle drag owns camera manipulation; competing left presses cannot start flows.
Escape, tool/slot changes, fit/restart, focus loss, suspension, pan or wheel zoom
cancel unfinished input. Releasing outside or at the start point admits nothing.

An admitted preview is separate from a new gesture or refused preview. Pausing
leaves accepted input queued; only a completed boundary confirms solid snapshot
geometry. A full queue preserves the refused owned preview; redraw a gesture after
capacity frees to retry. Cancelling a gesture does not undo already queued input.
Closing and terminal outcomes prevent edits; restart resets preview and admission.

ScenePainter consumes the owned snapshot. Straight capsules have clipped boundary
strokes and three directional chevrons. Radial effects have inward/outward
chevrons independent of color. White dashed geometry is uncommitted preview;
selected slots have thicker boundaries. Zero radius and unused slots draw no
field; zero strength draws its boundary without directional arrows. Double
clipping precedes narrowing to SDL vertices. Rendering remains primitive software
geometry with bounded field work and no new backend or renderer interface.

## Receiving

Use the prerequisite and permission-repair workflow before configured tests:

```sh
python scripts/run_tests.py --preset desktop-debug -R 'field_tools|scene_painter_software|desktop'
```

The architect should substitute the actual retained preset if it differs.
Receiving must include Release, ASan/UBSan and native X11 event smoke where the
probe qualifies the session. Source fixtures cover toolbar/key selection,
preview-only input, one release admission, paused confirmation, old confirmation
versus fresh gesture, replace/erase, every cancel path, letterbox conversion,
terminal denial, queue overflow and restart. Actual SDL gesture traces replay on
fresh matching Simulation and compare samples/velocities, field endpoints,
infection, stock and ledger exactly. Software pixel fixtures independently pin
radial direction, flow reversal, corridor width/caps, selected thickness, dashed
gaps, fit/zoom and viewport clipping, including extreme finite endpoints.

## Actual visual examples

The configured `crucible_scene_painter_test` supports:

```sh
<configured-scene-painter-test> --export-flow-fit <new-fit.bmp>
<configured-scene-painter-test> --export-flow-zoom <new-zoom.bmp>
```

Both execute the receiving fixtures first and export the production painter's
owned tick60 state: applied FLOW slot0, attract slot1, repel slot2, and white dashed
uncommitted FLOW slot3. Zoom uses factor2 around the midpoint. Record source/head,
preset and command, output hash, tick, backend/software classification, and visual
inspection in the architect's capture provenance. Images are actual simulation
states, not generated concept art or faction gameplay. A before frame can be the
initial inspector or a radial-only export. Native gestures require separate event
evidence; these exports alone do not prove input or human comprehension.

## Open gates

The [human protocol](../../playtests/reclamation-phase8.md) prepares comparison of
FLOW/attract/repel, slot replacement and queue feedback. No participant session,
physical high-DPI device, touch input, visual fidelity, faction mechanics or
multiplayer acceptance is claimed. Preserve P05-F01/P04-F02 and the hardware
backlog. Integration owns actual executions and findings before sprint closure.
