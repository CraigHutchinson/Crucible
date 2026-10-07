# P13-01 personal receiving: readable relay controls

Use after matching automated gates in the [Phase13 review](../sprint-reviews/phase-13.md).
This review covers native readability/input and comprehension separately from the
software/dummy-window tests. Sound is not part of this package.

From a VS developer PowerShell at the repository root, the isolated receiving build:

```powershell
.\build\phase13-release\src\desktop\crucible_desktop.exe --structural
```

For an ordinary preset build, use `build/desktop-release` instead. Add
`--fullscreen` to exercise fullscreen startup. Windowed remains the default.

1. Read the recovered quota, remaining ticks, mass and relay rows. All toolbar
   labels should fit at1026x607 and at your monitor size. Note output size and DPI.
2. Use the visible PAUSE/RESUME button. Select4 FLOW and drag in the world. While
   paused, expect a dashed preview and Queued feedback, with no completed action.
   Resume: expect application at a completed boundary and solid field arrows.
3. Select1 ATTRACT and click inside the relay ring. Once eligible mass reaches64,
   use F FUSE. Expect an amber lattice and increasing hold ticks. X SHATTER returns48
   and adds16 lost. An early fuse explains insufficient mass without stopping play.
4. Use F11 FULL, then F11 WIN or Escape. Expect a usable window and preserved
   mission, camera, selected slot/tool and already accepted input. An unfinished
   drag is canceled. Repeat, resize, change focus and inspect any clipping/stale input.
5. R or RESTART clears queued work and resets the mission. A terminal run stays
   stopped until restart. Try the legacy launch without `--structural`: fuse/shatter
   are visibly disabled and submit nothing.

Record renderer, window/output sizes, display scale, exact source/build, screenshot
and any failed step. Participant questions: identify the objective; explain queued
versus applied; explain fuse/shatter's mobility cost and the terminal result.
Computer control is reserved mainly for diagnosing a reported failure. Passing
automated fixtures does not close participant accessibility, balance or heard-audio gates.
