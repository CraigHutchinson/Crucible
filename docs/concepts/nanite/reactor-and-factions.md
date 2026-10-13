# Living reactor and multi-faction nanites

2026-10-13 · User-directed groundwork; visual proposals, not new gameplay rules.

![Reactor study: tetrahedral containment housing with one luminous crystal; cyan circle, amber triangle, violet diamond and lime square instances](assets/reactor-factions-01.png)

## Three nested layers

1. **Articulated shell:** silver face shields, short edge contacts and graphite
   joints. It supplies protection, silhouette and visible mechanical action.
2. **Tetrahedral containment:** one stable four-faced housing held in the cradle.
   This replaces the earlier solid cyan core treatment. Lightly tinted or smoky
   triangular windows and restrained edges reveal its volume. No octahedron,
   broad flat chassis or multiple independent face reactors.
3. **Inner heart:** one smaller luminous crystal, visibly separated from the
   housing by interior space. It supplies the magical-reactor character. Unlike
   the housing, it may be irregular, rotate, mutate its facets or reshape.

The housing stays rigid while the crystal changes. Keep the inner heart centred
within a bounded clearance envelope; do not pass through its walls, duplicate it
or change shell scale to accommodate an action. Start with a crystal maximum
radius safely inside the housing's inscribed sphere, then refine after animation
checks. This is an authoring constraint, not a new physics or biomass quantity.

Use faint housing edges and selective transparency so the crystal is distinct
from its enclosure. The image is a material target; it does not prove a consistent
tetrahedral mesh or feasible transparent rendering. The first model must show
the fourth housing vertex in an underside/turntable view. If transparency harms
clarity or cost, first test opaque dark windows with a contained emissive heart
impression. Retain the layered identity without requiring a new renderer feature.

## Faction identity

Extend the existing [faction concept board](../factions-deathmatch-v2.png) and
[faction/control boundary](../../decisions/faction-extensibility.md).
The following are palette slots, not named factions or a four-faction limit.

| Palette slot | Heart emission / subtle housing tint | Companion identity shape |
| --- | --- | --- |
| Cyan | Cool cyan | Circle |
| Amber | Warm amber | Triangle |
| Violet | Violet | Diamond |
| Lime | Yellow-green | Square |

Preserve the shared silver shell, anatomy and core location across slots.
Colour belongs mainly to the inner heart, with restrained spill into the housing
and seams. A shape badge can appear on a readable shell identification patch or
an inspection/selection marker; its placement needs a separate model/UI review.
The generated board places badges beside units and does not establish an in-world
badge solution. Small views need labels or markers when surface badges disappear.

Faction, controller, organism kind, relationship and health remain separate.
Cyan does not universally mean player or friendly; different colours do not
imply hostility. Neutral/unowned needs an explicit label/shape treatment at the
first consumer, not an invented fifth faction. Palette substitution must preserve
the associated shape and label. Validate grayscale, common colour-vision
differences and bloom-off readability with people; these checks remain pending.

Amber also appears in existing reserve/lattice concepts. Distinguish those
materials by granular/lattice form, spatial role and labels; do not recolour
resources to a faction slot or infer ownership from an amber glow.

## Close-up life and action vocabulary

These states are candidate visual mappings. Bind only to meaningful events the
eventual consumer actually exposes; do not invent health, energy or combat APIs
solely to animate this study.

| Observed state / action | Proposed inner-heart behaviour | Stable cue |
| --- | --- | --- |
| Rest / alive | Very slow rotation and a gentle bounded pulse | Faction hue, one heart, rigid housing |
| Wake / response | Brief increase in intensity, facets align then settle | No full-screen flash or new colour |
| Travel / work | Slightly faster rotation or controlled elongation inside the clearance envelope | Shell pose still conveys contact and heading |
| Dock / lock | Facets align and settle; one short contained pulse | Mechanical contact remains the timing anchor |
| Release / reshape | Smooth interpolation between a small set of authored crystal forms | No teleporting fragments or housing deformation |
| Strain / low vitality, if a real state exists | Reduced regularity or a constrained compressed form | Explicit status cue/label; faction hue preserved |
| Inactive / dead, if a real state exists | Motion stops and emission fades to a visible inert crystal | Identity badge remains legible |

Separate life-state intensity from faction hue: do not turn every damaged unit
red and erase its identity. Smooth form interpolation must avoid self-intersection
and clipping. Begin with a few authored forms with compatible topology; unrestricted
topology mutation and crystal fracture are later investigations, not prerequisites.
Crystal deformation does not create entities, spend energy or change the ledger.

For the intro, the authored timeline may drive these states. In gameplay, consume
completed observations and bounded action cues; visual rotation or camera distance
must not determine simulation outcomes. A stable per-instance phase can break
perfect synchrony, but it must not become per-frame random flicker or replay input.
Pause and reduced-motion policy need explicit receiving: choose a static crystal
pose under reduced motion and preserve important status through steady cues.

## Detail and cost boundaries

Hero close-ups may show housing depth, internal crystal facets and actual rotation.
At medium range collapse the heart to a simple bounded emissive form. At far range
keep one identity accent and the unit/collective silhouette; omit internal motion.
Blend transitions so colour and average brightness remain stable. Do not attach
a shadow-casting light, particle system or unique material to every nanite by default.
Shared geometry/material variants and bounded per-instance presentation data are
candidate approaches, to be measured in the actual consumer rather than assumed.

## Next receiving gates

- Turntable: one housing with four triangular faces, one inner crystal, one shell;
  distinguish all three layers at front, side and underside.
- Motion: rest → wake → contact → lock → release; verify bounds and intersections
  through the full cycle, including transitions between authored crystal forms.
- Identity: same pose/light/exposure for all four slots; shape/label recognition
  in grayscale and with bloom disabled; distinguish amber resources from amber units.
- View policy: stable pause/reduced-motion stills; no flicker or brightness pop at
  detail transitions; motion remains a presentation effect.
- Renderer: capture actual output and measure near/far cost before selecting
  transparency, internal refraction, dynamic lights or large-scale animation.

The static concept board passes only a visual direction review: shared machine
identity, separated inner heart and four colour/shape alternatives. It shows whole
units on the right rather than the requested reactor-only insets; that is useful
for faction comparison but is not a matched-pose technical comparison. Animated
life, exact topology, accessibility, docking and performance remain unreceived.
