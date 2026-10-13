# Nanite detail lab

A browser lab for the procedural nanite: the production generator
(`src/presentation/production/nanite_model.cpp`) compiled to WebAssembly and lit with
three.js, so its two detail axes, posture and palette can be reviewed on any device.
Evidence and the platform decision: [phase15 nanite model](../../docs/workstreams/presentation/phase15-nanite-model.md).

The page renders an approximation of the production look (three.js physical materials,
a room environment, bloom). Material acceptance belongs to the Filament renderer; this lab
judges form, silhouette, detail steps and triangle budgets.

## Build and open

```bash
source <emsdk>/emsdk_env.sh            # Emscripten 4+ (tested with 6.0.12)
spikes/nanite_lab/build_lab.sh         # -> build/nanite_lab/{index.html,lab.html,nanite_model.mjs}
python3 -m http.server -d build/nanite_lab 8000   # then open http://localhost:8000
```

The page loads three.js 0.170.0 from jsDelivr. `lab.html` is the page body for the
artifact host, which adds the document skeleton; `index.html` wraps it for local use.

## Controls

- **View**: one unit (turntable), the 4 x 4 detail matrix (rows structure, columns surface,
  triangle count under each), or a small swarm at a hardware-class preset.
- **Structure** and **Surface**: the two independent axes (`NaniteStructure`, `NaniteSurface`).
- **Hardware class**: sets both axes to that class's hero cap; the table lists hero and swarm
  triangles per unit and the swarm's total at 4096 units.
- **Posture**: stretch (shell elongates toward the nose; core rigid), open (shields lift and
  hinge; the base face stays down), heart spin; or play the travel/contact cycle.
- **Palette slot**: cyan, amber, violet, lime heart and housing tint.
- **Render**: turntable, bloom, glass housing (transmission) or smoked, wireframe, underside camera.

## Headless captures

```bash
npm install three@0.170.0               # in a scratch directory; playwright from the environment
node spikes/nanite_lab/capture.mjs build/nanite_lab <scratch>/node_modules/three <out> [--video]
```

The script serves the lab locally, maps the jsDelivr three.js URLs to the local package,
renders with SwiftShader and writes the review stills, `triangles.json` and, with
`--video`, an 8 s turntable through the travel/contact cycle (`turntable.mp4`).
