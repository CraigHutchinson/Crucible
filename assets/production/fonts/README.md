# Production typography

Two unchanged original TrueType fonts are bundled for the Phase15 production
frontend. Rajdhani SemiBold supplies angular headings and actions; Source Sans 3
Regular supplies readable objectives, explanations and status. Neither replaces
the authored three-dimensional CRUCIBLE wordmark.

| Original asset | Immutable official source | Bytes | SHA256 |
|---|---|---:|---|
| `rajdhani/Rajdhani-SemiBold.ttf` | [Google Fonts revision 51303ca](https://github.com/google/fonts/blob/51303ca9e8ac9dcea7b12d307ba568fd0e6fcfca/ofl/rajdhani/Rajdhani-SemiBold.ttf) | 390340 | `94bbd25a18ca665999feb05a537de9fd2b860dcfb78bbe9ca00270825bf235da` |
| `source-sans/SourceSans3-Regular.ttf` | [Adobe revision 87b37a2](https://github.com/adobe-fonts/source-sans/blob/87b37a2daaed80fcb8e8ccb0085c4d72ddade12e/TTF/SourceSans3-Regular.ttf) | 431196 | `4644c81b86ec9caaa76b634889968ed3c4f4f52f054855933acc7c2b21e53b0f` |

Fetched on 2026-10-09 using HTTPS from these exact official revisions. The original
binary contents and filenames are preserved. Only these two fonts and their
licenses are imported; no font repository, font generator or executable is imported.
Each font is below the production loader's 2MiB per-file bound.

The unchanged licenses accompany the fonts:

- Rajdhani: `rajdhani/OFL.txt`, 4373 bytes, SHA256
  `f62ef357d3a1c3d27edd35a6e1ba350e8a8d13499797964eeadefbf0b3b15d1f`.
- Source Sans 3: `source-sans/LICENSE.md`, 4486 bytes, SHA256
  `56af9b9c6715597e458284a474dc118a50a4150e9d547c70f7b4a33c3e6a9328`.

Both originals include the SIL Open Font License 1.1 and their copyright notices.
Keep those files with packaged assets. Source Sans includes its reserved-name
notice; no font outline, naming-table or license modification is made here.

The asset root consumed by the frontend contains these two subdirectories.
Packaging and executable-relative asset discovery belong to the production app.
Loading fonts does not establish that every later ImGui glyph size is already
baked or uploaded. UI warm-state, atlas growth, device upload and native legibility
receiving are separate from these source/hash receipts.

## Visually inspected typography study

![Original-font typography and palette study](font-specimen.png)

`make_specimen.py` renders this original-font/palette study with Pillow12.3.0;
run it from any directory with Python and Pillow to reproduce the1440×900 PNG.
It was visually inspected on2026-10-09. SHA256:
`52fbec1f9b823be4971da746e1831edd81f00a989fbd793f6fcebaedc480f643`.
This static study demonstrates the imported letterforms and palette. It is not
a functional-menu capture, a Filament frame or native legibility evidence.
