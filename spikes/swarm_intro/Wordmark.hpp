#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace crucible::spike::swarm
{
/** One docking target of the authored title mask. */
struct Slot
{
    float x = 0.0F;          ///< World position of the slot centre.
    float y = 0.0F;
    float heading = 0.0F;    ///< Stroke tangent (rows), or +-pi/2 for an up or down triangle (tessellated).
    std::uint8_t letter = 0; ///< Index into the eight-letter word.
    /** Tessellated: the tile's three corners relative to (x, y), in the letter's plane (x
     *  along the title, y up the letter). A seated unit morphs into this shape; corners on the
     *  outline are pulled onto it, and shared corners match their neighbours exactly. */
    std::array<float, 6> shape{};
    /** Nearest stroke segment (letter-local index, segments numbered across the glyph's
     *  centre lines), and whether that segment is a scaffold stroke: straight, upright, at
     *  least 45% of the cap height, and standing on the baseline. */
    std::int16_t segment = -1;
    bool scaffold = false;
};

/** What morphing did to the outline; zero-initialised when tessellation is off. */
struct WordmarkStats
{
    std::size_t boundaryCorners = 0;   ///< Lattice corners on a letter outline.
    float outlineRmsBefore = 0.0F;     ///< RMS distance of those corners from the true outline.
    float outlineRmsAfter = 0.0F;      ///< The same after morphing.
    float maxMorph = 0.0F;             ///< Largest corner displacement.
    float minAreaRatio = 1.0F;         ///< Smallest morphed / equilateral tile area (> 0: none inverted).
};

/** Fixed glyph geometry: authored stroke centre lines sampled into slots. */
struct WordmarkSettings
{
    float capHeight = 20.0F;   ///< Letter height in nanite-diameter units.
    float pitch = 1.15F;       ///< Slot spacing along and across a stroke.
    int strokeRows = 4;        ///< Parallel rows forming one stroke's thickness.
    float letterGap = 5.0F;    ///< Space between letter boxes.
    /** Tessellation: when positive, slots are equilateral triangles of this side on a
     *  triangular lattice clipped to the strokes, alternating up and down so neighbours share
     *  edges and the letter is a solid, gap-free face. Zero samples stroke rows instead.
     *  The default sqrt(3) gives each triangle an inscribed circle of one unit diameter. */
    float tile = 1.7320508F;
    /** Morph limit as a fraction of the tile side: outline corners move at most this far
     *  toward the true stroke outline. Zero keeps every tile equilateral. */
    float morph = 0.35F;
};

/** The CRUCIBLE title as a deterministic slot list, centred on the origin.
 *  The mask is authored geometry (director brief, shot03): readability, open counters
 *  and spacing are decided here, never derived from swarm behaviour or a raster.
 *  @param settings Glyph scale and sampling.
 *  @return Slots grouped by letter, letter order C R U C I B L E. */
[[nodiscard]] std::vector<Slot> buildWordmark(const WordmarkSettings& settings, WordmarkStats* stats = nullptr);

/** Horizontal centre of each letter box, for lane exits and diagnostics.
 *  @param settings Same settings as buildWordmark.
 *  @return Eight centres in letter order. */
[[nodiscard]] std::vector<float> letterCentres(const WordmarkSettings& settings);
} // namespace crucible::spike::swarm
