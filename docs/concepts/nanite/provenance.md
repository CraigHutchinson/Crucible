# Nanite study provenance and review

2026-10-13. Source review at PR43 head `dc33dfe867f1c4043c43fc2de038945d4046007e`;
publication branch based on its main merge `0063735`. Built-in image generation,
opaque backgrounds, no CLI/API fallback. The tool did not expose a model version.
All selected PNG bytes are preserved unchanged in the downloadable artwork bundle.
The four new PNGs are not yet in repository Git LFS; authenticated upload is pending. No resizing, retouching or colour editing was applied. Pillow was used only
to inspect metadata; the images were visually inspected in the conversation.

## Exact assets and prompts

| Image | Exact submitted prompt | Dimensions / size | SHA256 |
| --- | --- | --- | --- |
| `assets/form-study-01.png` (bundle) | [form-study-1.txt](prompts/form-study-1.txt) | 1672 × 941, RGB; 2116386 bytes | `0ebc1bf44642234fb9f97547e260ceeb2b8b1edffc6a63a016c170f4b00d43fb` |
| `assets/form-study-02.png` (bundle) | [form-study-2.txt](prompts/form-study-2.txt) | 1672 × 941, RGB; 2061284 bytes | `9b9778142700caede58d68fbf2c05c2e538e6ceff3c6ee4efbefb6ddf7b6f57a` |
| `assets/form-study-02-core-corrected.png` (bundle) | [form-study-3.txt](prompts/form-study-3.txt) | 1672 × 941, RGB; 1981508 bytes | `5b7e2860e60d4dbe82f40c988da13d4019b3c80c5bea5a6e8b6c17a7eade0aac` |
| `assets/reactor-factions-01.png` (bundle) | [reactor-factions-01.txt](prompts/reactor-factions-01.txt) | 1672 × 941, RGB; 2214125 bytes | `dbbde71db36304d45a5d83e94ad999f21a8b8c6d47eb56c4d36ca744d59c1e98` |

## Reference chain

- Form study 01 used `docs/concepts/units-v1.png` and
  `docs/concepts/phase15-experience/assets/intro-hero-v1.png` as image references.
- Form study 02 used form study 01.
- Core-corrected study 02 used form study 02. The displayed title intentionally
  remains FORM STUDY 02; the filename records the correction.
- Reactor/faction study used core-corrected study 02. It is the latest inner-core
  direction, expanding the tetrahedron into housing plus a separate crystal.

## Inspection and acceptance limits

| Iteration | Visual finding | Use / remaining limitation |
| --- | --- | --- |
| Form study 01 | Full silver volume, sculpted underside and opening shell | Historical exploration; busy joints and potentially multiple-core reading |
| Form study 02 | Clear compact/open comparison | Superseded core: diamond/gem interpretation was not a tetrahedron |
| Core-corrected study 02 | Simple tetrahedral core reads clearly; shell preserved | Shell/form target; opaque core material superseded by reactor proposal |
| Reactor/faction study | Separate luminous heart inside dark translucent enclosure; cyan/circle, amber/triangle, violet/diamond, lime/square present | Current layered-material direction; right studies show full units instead of reactor-only insets; exact fourth housing vertex and matched pose/light require a model |

These are generated concepts, not executing renderer evidence or mechanically
consistent orthographic drawings. No rig, collision, packing, temporal animation,
accessibility or native performance gate is closed by these images. The single-unit
and reactor notes name the follow-up acceptance views and receiving requirements.

## Publication validation

Checked local Markdown link targets for every changed document, PNG decoding,
asset hashes, Git LFS attributes and whitespace. The initial complete local commit
`74e3a5c1321999f69365e21ee0a4d13a7e6d8b32` includes valid image pointers, but
Git/LFS push failed because the local client has no write credentials. The GitHub
connector can publish text but does not expose LFS upload. The published revision
therefore contains no missing-object pointers or broken local image references.

[Download all four original PNGs and exact prompts](https://chatgpt.com/api/library/files/libfile_2ff8ecf595c48191a40b1f8f52fd5a74/download)
(access requires the owning ChatGPT account). The archive includes import steps.
The hashes above and intended relative paths allow exact later LFS import. After
authenticated upload, verify a fresh checkout and restore relative image embeds.
This is a known artwork-discoverability limitation for repository-only readers. No runtime/build files
changed; no new local C++ suite is claimed. Hosted exact-head CI is recorded on the
publication PR. The original study remains intact as a historical snapshot while
the navigation and reactor note make the latest direction explicit.
