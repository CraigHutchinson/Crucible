# Windows native interaction receiving, 2026-10-05

Source `1eef613c08d72535975aac9c7ce01c31f98efb9c`; Release desktop built from
that revision. Documentation was dirty during capture; no C++ changes. Launch
`build/phase12-release/src/desktop/crucible_desktop.exe` with no arguments:
reclamation challenge, default scenario/seed, integrated runtime, diagnostics off.
Windows 11 build 26220, MSVC19.51, SDL native renderer reported `direct3d11` in
the preceding launch receipt; pixel density1.00/display scale1.25. This launch's
backend was not separately collected, so these images do not identify a GPU device.
Captures are bounded native window screenshots, 1026x607, through @oai/sky.

The automated operator received terminal LOST at tick900, restarted with R,
then paused with Space at tick456. Selected4 FLOW and dragged window coordinates
(350,300) to(650,300). Simulation remained paused at tick456; dashed preview and
text reported queued/waiting for completed boundary.

![Paused queued flow](native-flow-queued.jpg)

Resuming with Space published the command: screenshot at tick486 reported
applied at completed boundary and changed the field outline to solid green.
Paused again at tick891 and retained the applied capture below. Fit View toolbar
click responded without losing the retained field; a wheel action did not produce
a clearly visible zoom change, so zoom acceptance remains unconfirmed. Closed the
owned window with Alt+F4. Tool interaction delays consumed hundreds of running
ticks; this is not a controlled latency measurement or successful mission route.

![Paused applied flow](native-flow-applied.jpg)

The header's state/progress bar was readable, while numeric/body text was very
small at this captured size (roughly6px glyph height). The right FLOW toolbar
extends close to the viewport edge. Phase13 should receive scalable text/layout
and recognizable queued/applied/refused feedback at actual display scale.
This is actual rendering/input operator evidence, not participant comprehension,
balance, accessibility or audio evidence. Structural input was not exercised in
this retry; synthetic/full-state structural receiving is recorded separately.
