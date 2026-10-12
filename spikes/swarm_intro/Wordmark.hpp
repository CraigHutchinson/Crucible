#pragma once

#include <cstdint>
#include <vector>

namespace crucible::spike::swarm
{
/** One docking target of the authored title mask. */
struct Slot
{
    float x = 0.0F;          ///< World position of the slot centre.
    float y = 0.0F;
    float heading = 0.0F;    ///< Stroke tangent, radians; the wedge docks along its stroke.
    std::uint8_t letter = 0; ///< Index into the eight-letter word.
};

/** Fixed glyph geometry: authored stroke centre lines sampled into slots. */
struct WordmarkSettings
{
    float capHeight = 20.0F;   ///< Letter height in nanite-diameter units.
    float pitch = 1.15F;       ///< Slot spacing along and across a stroke.
    int strokeRows = 4;        ///< Parallel rows forming one stroke's thickness.
    float letterGap = 5.0F;    ///< Space between letter boxes.
};

/** The CRUCIBLE title as a deterministic slot list, centred on the origin.
 *  The mask is authored geometry (director brief, shot03): readability, open counters
 *  and spacing are decided here, never derived from swarm behaviour or a raster.
 *  @param settings Glyph scale and sampling.
 *  @return Slots grouped by letter, letter order C R U C I B L E. */
[[nodiscard]] std::vector<Slot> buildWordmark(const WordmarkSettings& settings);

/** Horizontal centre of each letter box, for lane exits and diagnostics.
 *  @param settings Same settings as buildWordmark.
 *  @return Eight centres in letter order. */
[[nodiscard]] std::vector<float> letterCentres(const WordmarkSettings& settings);
} // namespace crucible::spike::swarm
