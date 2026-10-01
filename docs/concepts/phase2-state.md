# Actual phase 2 simulation frame

This is generated from implemented state, independently of the concept art. Tick 20,
2,048 samples, 800 infected cells, one active repulsive radial field. The initial
attraction field was removed at tick 11. Cyan dots are samples, red cells are Blight,
the orange ring is the repulsive field radius. The diagram's +Y points down.

Reproduce from the repository root after a Release build:

```sh
./build/release/crucible --export-svg docs/concepts/phase2-state.svg
```

Use `.exe` on Windows. Export is locale-independent and stable within a platform/
configuration; floating-point replay is not claimed bitwise across compilers.
The checked-in SVG was exported with MSVC Release. PNG preview was rendered offline
with the already-installed PyMuPDF and visually inspected at 1320 by 780; title,
legend, all world bounds, sample markers, infection and field extent are legible.
No graphics dependency is added to the build. Reproduction of the PNG preview:

```python
import pymupdf
from pathlib import Path
frame = pymupdf.open(stream=Path("docs/concepts/phase2-state.svg").read_bytes(), filetype="svg")
frame[0].get_pixmap(matrix=pymupdf.Matrix(1.5, 1.5)).save("docs/concepts/phase2-state.png")
```

This short scenario has modest sample displacement. It does not demonstrate resource
consumption, terrain, combat, relay capture, fusion or shatter, or a playable HUD.
