#include "Wordmark.hpp"

#include <array>
#include <cmath>
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

std::vector<Slot> buildWordmark(const WordmarkSettings& settings)
{
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
    return slots;
}
} // namespace crucible::spike::swarm
