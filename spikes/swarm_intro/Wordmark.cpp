#include "Wordmark.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace crucible::spike::swarm
{
namespace
{
struct Point
{
    float x;
    float y;
};
using Polyline = std::vector<Point>;

constexpr std::array<char, 8> kWord{ 'C', 'R', 'U', 'C', 'I', 'B', 'L', 'E' };

/** Letter box width as a fraction of cap height. I is a single stroke. */
float widthFactor(char letter)
{
    switch (letter)
    {
    case 'C': return 0.75F;
    case 'R': return 0.70F;
    case 'U': return 0.70F;
    case 'I': return 0.15F;
    case 'B': return 0.70F;
    case 'L': return 0.60F;
    case 'E': return 0.60F;
    default: return 0.70F;
    }
}

/** Elliptical arc from a0 to a1 (radians, counter-clockwise), sampled finely. */
void appendArc(Polyline& line, Point centre, float rx, float ry, float a0, float a1)
{
    constexpr int kSteps = 48;
    for (int i = 0; i <= kSteps; ++i)
    {
        const float a = a0 + (a1 - a0) * static_cast<float>(i) / kSteps;
        line.push_back({ centre.x + rx * std::cos(a), centre.y + ry * std::sin(a) });
    }
}

/** Centre lines of one glyph in a box [0,w]x[0,h], already inset for stroke thickness. */
std::vector<Polyline> glyph(char letter, float w, float h, float inset)
{
    constexpr float pi = std::numbers::pi_v<float>;
    const float l = inset;
    const float r = w - inset;
    const float b = inset;
    const float t = h - inset;
    std::vector<Polyline> lines;
    switch (letter)
    {
    case 'C':
    {
        Polyline arc;
        appendArc(arc, { w * 0.5F, h * 0.5F }, w * 0.5F - inset, h * 0.5F - inset, pi * 0.24F, pi * 1.76F);
        lines.push_back(arc);
        break;
    }
    case 'R':
    {
        const float mid = h * 0.47F;
        const float bowlR = (t - mid) * 0.5F;
        Polyline bowl{ { l, t }, { r - bowlR, t } };
        appendArc(bowl, { r - bowlR, mid + bowlR }, bowlR, bowlR, pi * 0.5F, -pi * 0.5F);
        bowl.push_back({ l, mid });
        lines.push_back({ { l, b }, { l, t } });
        lines.push_back(bowl);
        lines.push_back({ { l + (r - l) * 0.45F, mid }, { r, b } });
        break;
    }
    case 'U':
    {
        const float radius = (r - l) * 0.5F;
        Polyline u{ { l, t } };
        appendArc(u, { (l + r) * 0.5F, b + radius }, radius, radius, pi, 2.0F * pi);
        u.push_back({ r, t });
        lines.push_back(u);
        break;
    }
    case 'I':
        lines.push_back({ { w * 0.5F, b }, { w * 0.5F, t } });
        break;
    case 'B':
    {
        const float mid = h * 0.52F;
        const float upperR = (t - mid) * 0.5F;
        const float lowerR = (mid - b) * 0.5F;
        Polyline upper{ { l, t }, { r - upperR * 1.1F, t } };
        appendArc(upper, { r - upperR * 1.1F, mid + upperR }, upperR, upperR, pi * 0.5F, -pi * 0.5F);
        upper.push_back({ l, mid });
        Polyline lower{ { l, mid }, { r - lowerR, mid } };
        appendArc(lower, { r - lowerR, b + lowerR }, lowerR, lowerR, pi * 0.5F, -pi * 0.5F);
        lower.push_back({ l, b });
        lines.push_back({ { l, b }, { l, t } });
        lines.push_back(upper);
        lines.push_back(lower);
        break;
    }
    case 'L':
        lines.push_back({ { l, t }, { l, b }, { r, b } });
        break;
    case 'E':
        lines.push_back({ { r, t }, { l, t }, { l, b }, { r, b } });
        lines.push_back({ { l, h * 0.5F }, { l + (r - l) * 0.8F, h * 0.5F } });
        break;
    default:
        break;
    }
    return lines;
}

/** Extends both open ends by half the stroke thickness: square caps, so stems reach the
 *  baseline (an upright title stands on every letter) and ends read crisp. */
Polyline capped(Polyline line, float extension)
{
    auto extend = [extension](Point end, Point inner) {
        const float dx = end.x - inner.x, dy = end.y - inner.y;
        const float length = std::hypot(dx, dy);
        return length > 1e-6F ? Point{ end.x + dx / length * extension, end.y + dy / length * extension } : end;
    };
    if (line.size() < 2) return line;
    line.front() = extend(line.front(), line[1]);
    line.back() = extend(line.back(), line[line.size() - 2]);
    return line;
}

/** Samples a polyline at fixed arc-length pitch, emitting parallel rows across it. */
void sample(const Polyline& line, float pitch, int rows, Point origin, std::uint8_t letter, std::vector<Slot>& out)
{
    float carry = 0.0F;   // arc length still owed before the next sample
    for (std::size_t i = 0; i + 1 < line.size(); ++i)
    {
        const Point a = line[i];
        const Point c = line[i + 1];
        const float dx = c.x - a.x;
        const float dy = c.y - a.y;
        const float length = std::hypot(dx, dy);
        if (length <= 1e-6F) continue;
        const float tx = dx / length;
        const float ty = dy / length;
        float s = carry;
        for (; s <= length; s += pitch)
        {
            for (int row = 0; row < rows; ++row)
            {
                const float offset = (static_cast<float>(row) - static_cast<float>(rows - 1) * 0.5F) * pitch;
                out.push_back({ origin.x + a.x + tx * s - ty * offset, origin.y + a.y + ty * s + tx * offset,
                                std::atan2(ty, tx), letter });
            }
        }
        carry = s - length;
    }
}
} // namespace

std::vector<float> letterCentres(const WordmarkSettings& settings)
{
    float total = 0.0F;
    for (char letter : kWord) total += widthFactor(letter) * settings.capHeight;
    total += settings.letterGap * static_cast<float>(kWord.size() - 1);
    std::vector<float> centres;
    float x = -total * 0.5F;
    for (char letter : kWord)
    {
        const float w = widthFactor(letter) * settings.capHeight;
        centres.push_back(x + w * 0.5F);
        x += w + settings.letterGap;
    }
    return centres;
}

/** Distance from a point to a polyline. */
float distanceTo(const Polyline& line, Point p)
{
    float best = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i + 1 < line.size(); ++i)
    {
        const Point a = line[i];
        const Point c = line[i + 1];
        const float dx = c.x - a.x, dy = c.y - a.y;
        const float len2 = dx * dx + dy * dy;
        const float t = len2 > 0.0F ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len2, 0.0F, 1.0F) : 0.0F;
        best = std::min(best, std::hypot(p.x - (a.x + dx * t), p.y - (a.y + dy * t)));
    }
    return best;
}

/** Nearest point on any centre line, and the distance to it. */
Point nearestOn(const std::vector<Polyline>& lines, Point p, float& distance)
{
    distance = std::numeric_limits<float>::max();
    Point best = p;
    for (const Polyline& line : lines)
    {
        for (std::size_t i = 0; i + 1 < line.size(); ++i)
        {
            const Point a = line[i];
            const Point c = line[i + 1];
            const float dx = c.x - a.x, dy = c.y - a.y;
            const float len2 = dx * dx + dy * dy;
            const float t = len2 > 0.0F ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len2, 0.0F, 1.0F) : 0.0F;
            const Point q{ a.x + dx * t, a.y + dy * t };
            const float d = std::hypot(p.x - q.x, p.y - q.y);
            if (d < distance)
            {
                distance = d;
                best = q;
            }
        }
    }
    return best;
}

/** Triangles of a lattice whose centroids lie within the strokes' half thickness. Row 0
 *  sits on the baseline, so up triangles stand on the ground with a flat edge.
 *
 *  Morphing: a lattice corner on the outline (touching an excluded tile) moves toward the
 *  stroke outline, at most `maxMorph`. The move is a function of the corner alone, so every
 *  tile sharing that corner moves it identically: joins stay seamless while the outline
 *  loses its saw-tooth. Ground-row corners stay on the ground. */
void tessellate(const std::vector<Polyline>& lines, float w, float h, float side, float halfThickness, float maxMorph,
                Point origin, std::uint8_t letter, std::vector<Slot>& out, WordmarkStats& stats, double& before, double& after)
{
    constexpr float pi = std::numbers::pi_v<float>;
    const float rowHeight = side * std::sqrt(3.0F) * 0.5F;
    const int rows = static_cast<int>(h / rowHeight) + 1;
    const int columns = static_cast<int>(w / (side * 0.5F)) + 2;
    auto centroidOf = [&](int r, int c) {
        const bool up = ((r + c) & 1) == 0;
        return Point{ static_cast<float>(c) * side * 0.5F, static_cast<float>(r) * rowHeight + rowHeight * (up ? 1.0F : 2.0F) / 3.0F };
    };
    std::vector<std::uint8_t> inside(static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns + 1), 0);
    auto included = [&](int r, int c) -> bool {
        if (r < 0 || r >= rows || c < 0 || c > columns) return false;
        return inside[static_cast<std::size_t>(r) * static_cast<std::size_t>(columns + 1) + static_cast<std::size_t>(c)] != 0;
    };
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c <= columns; ++c)
        {
            const Point centroid = centroidOf(r, c);
            if (centroid.y > h) continue;
            float d = std::numeric_limits<float>::max();
            for (const Polyline& line : lines) d = std::min(d, distanceTo(line, centroid));
            inside[static_cast<std::size_t>(r) * static_cast<std::size_t>(columns + 1) + static_cast<std::size_t>(c)] = d <= halfThickness ? 1 : 0;
        }
    }
    // Gap fill: an excluded tile whose two same-row neighbours are both included is a notch
    // between spikes; filling it turns a saw-tooth edge into a band the morph can straighten.
    if (maxMorph > 0.0F)
    {
        std::vector<std::uint8_t> fill(inside.size(), 0);
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c <= columns; ++c)
            {
                if (!included(r, c) && included(r, c - 1) && included(r, c + 1) && centroidOf(r, c).y <= h)
                {
                    fill[static_cast<std::size_t>(r) * static_cast<std::size_t>(columns + 1) + static_cast<std::size_t>(c)] = 1;
                }
            }
        }
        for (std::size_t k = 0; k < inside.size(); ++k) inside[k] = static_cast<std::uint8_t>(inside[k] | fill[k]);
        // Trim, one pass: a tile held by a single edge is a spike off the outline; dropping
        // it leaves a straight band the morph can follow. Ground-row tiles always stay.
        std::vector<std::uint8_t> trim(inside.size(), 0);
        for (int r = 1; r < rows; ++r)
        {
            for (int c = 0; c <= columns; ++c)
            {
                if (!included(r, c)) continue;
                const bool up = ((r + c) & 1) == 0;
                const int edges = (included(r, c - 1) ? 1 : 0) + (included(r, c + 1) ? 1 : 0) + (included(up ? r - 1 : r + 1, c) ? 1 : 0);
                if (edges <= 1) trim[static_cast<std::size_t>(r) * static_cast<std::size_t>(columns + 1) + static_cast<std::size_t>(c)] = 1;
            }
        }
        for (std::size_t k = 0; k < inside.size(); ++k) inside[k] = static_cast<std::uint8_t>(inside[k] & ~trim[k]);
    }
    // Lattice corner (i, j) sits at (i * side / 2, j * rowHeight); its six tiles are columns
    // i-1..i+1 of rows j-1 and j. Corners are stored at column index i + 1 (i >= -1).
    const int cornerColumns = columns + 3;
    auto at = [&](int i, int j) { return static_cast<std::size_t>(j) * static_cast<std::size_t>(cornerColumns) + static_cast<std::size_t>(i + 1); };
    std::vector<Point> shift(static_cast<std::size_t>(rows + 1) * static_cast<std::size_t>(cornerColumns), Point{ 0.0F, 0.0F });
    std::vector<float> scale(shift.size(), 1.0F);
    std::vector<std::uint8_t> onOutline(shift.size(), 0);
    auto base = [&](int i, int j) { return Point{ static_cast<float>(i) * side * 0.5F, static_cast<float>(j) * rowHeight }; };
    for (int j = 0; j <= rows; ++j)
    {
        for (int i = -1; i <= columns + 1; ++i)
        {
            if (((i + j + 1) & 1) != 0) continue;
            bool used = false;
            bool boundary = false;
            for (int r = j - 1; r <= j; ++r)
            {
                for (int c = i - 1; c <= i + 1; ++c)
                {
                    used = used || included(r, c);
                    boundary = boundary || !included(r, c);
                }
            }
            if (!used || !boundary || maxMorph <= 0.0F) continue;
            const Point p = base(i, j);
            float d = 0.0F;
            const Point q = nearestOn(lines, p, d);
            if (d < 1e-5F) continue;
            const Point target{ q.x + (p.x - q.x) / d * halfThickness, q.y + (p.y - q.y) / d * halfThickness };
            float mx = target.x - p.x;
            float my = j == 0 ? 0.0F : target.y - p.y;   // ground-row corners stay on the ground
            const float m = std::hypot(mx, my);
            if (m > maxMorph)
            {
                mx *= maxMorph / m;
                my *= maxMorph / m;
            }
            shift[at(i, j)] = { mx, my };
            onOutline[at(i, j)] = 1;
        }
    }
    auto corner = [&](int i, int j) {
        const Point p = base(i, j);
        const Point m = shift[at(i, j)];
        const float k = scale[at(i, j)];
        return Point{ p.x + m.x * k, p.y + m.y * k };
    };
    auto cornersOf = [&](int r, int c, std::array<std::size_t, 3>& ids) {
        const bool up = ((r + c) & 1) == 0;
        // Counter-clockwise: up = base-left, base-right, apex; down = top-right, top-left, apex.
        const std::array<std::pair<int, int>, 3> ij = up ? std::array<std::pair<int, int>, 3>{ { { c - 1, r }, { c + 1, r }, { c, r + 1 } } }
                                                         : std::array<std::pair<int, int>, 3>{ { { c + 1, r + 1 }, { c - 1, r + 1 }, { c, r } } };
        std::array<Point, 3> out{};
        for (std::size_t k = 0; k < 3; ++k)
        {
            ids[k] = at(ij[k].first, ij[k].second);
            out[k] = corner(ij[k].first, ij[k].second);
        }
        return out;
    };
    const float equilateral = side * rowHeight * 0.5F;
    auto areaRatio = [&](const std::array<Point, 3>& t) {
        return 0.5F * ((t[1].x - t[0].x) * (t[2].y - t[0].y) - (t[2].x - t[0].x) * (t[1].y - t[0].y)) / equilateral;
    };
    // Quality: a tile never drops below half its area. Shrinking the shared corners' moves
    // keeps neighbours seamless; a few rounds settle every tile deterministically.
    constexpr float kMinAreaRatio = 0.5F;
    for (int round = 0; round < 32; ++round)
    {
        bool changed = false;
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c <= columns; ++c)
            {
                if (!included(r, c)) continue;
                std::array<std::size_t, 3> ids{};
                if (areaRatio(cornersOf(r, c, ids)) >= kMinAreaRatio) continue;
                for (std::size_t id : ids) scale[id] *= 0.8F;
                changed = true;
            }
        }
        if (!changed) break;
    }
    for (std::size_t k = 0; k < shift.size(); ++k)
    {
        if (!onOutline[k]) continue;
        const int i = static_cast<int>(k % static_cast<std::size_t>(cornerColumns)) - 1;
        const int j = static_cast<int>(k / static_cast<std::size_t>(cornerColumns));
        const Point p = base(i, j);
        const Point moved = corner(i, j);
        float d0 = 0.0F, d1 = 0.0F;
        (void)nearestOn(lines, p, d0);
        (void)nearestOn(lines, moved, d1);
        ++stats.boundaryCorners;
        before += static_cast<double>(d0 - halfThickness) * (d0 - halfThickness);
        after += static_cast<double>(d1 - halfThickness) * (d1 - halfThickness);
        stats.maxMorph = std::max(stats.maxMorph, std::hypot(moved.x - p.x, moved.y - p.y));
    }
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c <= columns; ++c)
        {
            if (!included(r, c)) continue;
            const bool up = ((r + c) & 1) == 0;
            const Point centroid = centroidOf(r, c);
            std::array<std::size_t, 3> ids{};
            const std::array<Point, 3> corners = cornersOf(r, c, ids);
            Slot slot{ origin.x + centroid.x, origin.y + centroid.y, up ? pi * 0.5F : -pi * 0.5F, letter };
            for (std::size_t k = 0; k < 3; ++k)
            {
                slot.shape[2 * k] = corners[k].x - centroid.x;
                slot.shape[2 * k + 1] = corners[k].y - centroid.y;
            }
            stats.minAreaRatio = std::min(stats.minAreaRatio, areaRatio(corners));
            out.push_back(slot);
        }
    }
}

std::vector<Slot> buildWordmark(const WordmarkSettings& settings, WordmarkStats* stats)
{
    WordmarkStats local;
    double before = 0.0, after = 0.0;
    const float h = settings.capHeight;
    const float inset = static_cast<float>(settings.strokeRows - 1) * 0.5F * settings.pitch + 0.2F;
    const std::vector<float> centres = letterCentres(settings);
    std::vector<Slot> slots;
    for (std::size_t i = 0; i < kWord.size(); ++i)
    {
        const float w = widthFactor(kWord[i]) * h;
        const Point origin{ centres[i] - w * 0.5F, -h * 0.5F };
        std::vector<Slot> raw;
        const float cap = static_cast<float>(settings.strokeRows - 1) * 0.5F * settings.pitch;
        if (settings.tile > 0.0F)
        {
            std::vector<Polyline> lines;
            for (const Polyline& line : glyph(kWord[i], w, h, inset)) lines.push_back(capped(line, cap));
            const float halfThickness = static_cast<float>(settings.strokeRows) * 0.5F * settings.pitch;
            tessellate(lines, w, h, settings.tile, halfThickness, settings.morph * settings.tile, origin, static_cast<std::uint8_t>(i),
                       slots, local, before, after);
            continue;
        }
        for (const Polyline& line : glyph(kWord[i], w, h, inset))
        {
            sample(capped(line, cap), settings.pitch, settings.strokeRows, origin, static_cast<std::uint8_t>(i), raw);
        }
        // Corners and inner arc rows oversample: keep a slot only when it is a full
        // pitch-ish away from every kept slot, so docked wedges never overlap.
        const float minDistance = settings.pitch * 0.92F;
        std::vector<Slot> kept;
        for (const Slot& candidate : raw)
        {
            bool clear = true;
            for (const Slot& existing : kept)
            {
                if (std::hypot(candidate.x - existing.x, candidate.y - existing.y) < minDistance)
                {
                    clear = false;
                    break;
                }
            }
            if (clear) kept.push_back(candidate);
        }
        slots.insert(slots.end(), kept.begin(), kept.end());
    }
    if (local.boundaryCorners > 0)
    {
        local.outlineRmsBefore = static_cast<float>(std::sqrt(before / static_cast<double>(local.boundaryCorners)));
        local.outlineRmsAfter = static_cast<float>(std::sqrt(after / static_cast<double>(local.boundaryCorners)));
    }
    if (stats) *stats = local;
    return slots;
}
} // namespace crucible::spike::swarm
