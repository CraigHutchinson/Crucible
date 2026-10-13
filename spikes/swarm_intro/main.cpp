// Nanite swarm intro spike: headless twelve-second CRUCIBLE assembly with camera-driven
// decision LOD. Writes optional per-tick metrics and frame captures for render_swarm.py.
// See README.md for the questions this spike answers and what it does not claim.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "SwarmIntro.hpp"

namespace
{
using namespace crucible::spike::swarm;

struct Options
{
    IntroSettings settings{};
    float seconds = 0.0F;   // presentation seconds; 0 runs 12 s of choreography (15 s for climbing builds)
    std::string framesDir;
    int every = 4;
    std::string metricsPath;
    std::string eventsPath;   // Sub0Log JSONL messages
    std::string healthPath;   // one swarm_health object
    bool verify = false;
};

struct RunSummary
{
    float lockedAt84 = 0.0F;   // fraction of slots locked at the shot04 exit (8.4 s)
    float lockedAt10 = 0.0F;
    float mooredAt10 = 0.0F;
    float firstLock = -1.0F;
    float allLocked = -1.0F;
    float allRaised = -1.0F;   // raise build: every letter upright
    std::uint32_t worstOverlaps = 0;
    float worstOverlapRatio = 0.0F;
    float worstOverlapTime = 0.0F;
    float minVisibleGap = 1e9F;
    std::uint32_t maxHeroes = 0;
    std::uint32_t maxMigrations = 0;
    std::uint64_t yields = 0;
    std::uint64_t claims = 0;
    std::uint64_t stuck = 0;
    std::uint64_t recovered = 0;
    std::uint64_t pushes = 0;
    std::uint64_t reclaims = 0;
    std::uint32_t maxStuckNow = 0;
    std::uint32_t stuckAtEnd = 0;
    std::uint64_t climberTicks = 0;
    std::uint32_t lockedAtEnd = 0;
    bool diagnosticsOk = true;
    std::uint64_t swaps = 0;
    std::uint64_t unsupported = 0;
    std::uint64_t offSurface = 0;
    std::uint64_t clicks = 0;
    std::uint32_t peakClicksPer100ms = 0;
    std::vector<float> tickMicros;
    std::array<double, 6> systemMicros{};
    std::uint64_t hash = 0;
    std::size_t slots = 0;
    WordmarkStats wordmark{};
    std::uint8_t raisedMask = 0;
    std::uint32_t panels = 0;
    std::size_t ribbon = 0;
};

std::string label(const IntroSettings& s)
{
    const char* build = s.build == Build::Lying ? "lying" : (s.build == Build::Climb ? "climb" : (s.build == Build::Raise ? "raise" : (s.build == Build::Hybrid ? "hybrid" : "scaffold")));
    const char* lod = s.lod == LodMode::Camera ? "camera" : (s.lod == LodMode::AllHero ? "uniform-hero" : (s.lod == LodMode::AllNear ? "uniform-near" : "uniform-far"));
    return std::string(build) + "/" + lod;
}

const char* actName(Act act)
{
    switch (act)
    {
    case Act::Dormant: return "dormant";
    case Act::Waking: return "waking";
    case Act::Lane: return "lane";
    case Act::Seek: return "seek";
    case Act::Climb: return "climb";
    case Act::Dock: return "dock";
    case Act::Locked: return "locked";
    case Act::Moor: return "moor";
    case Act::Moored: return "moored";
    case Act::Reserve: return "reserve";
    case Act::Queue: return "queue";
    }
    return "?";
}

RunSummary run(const Options& options, bool capture)
{
    SwarmIntro intro(options.settings);
    const bool record = capture && !options.eventsPath.empty();
    std::unique_ptr<SwarmDiagnostics> diagnostics;
    if (record)
    {
        diagnostics = std::make_unique<SwarmDiagnostics>();
        intro.attachDiagnostics(diagnostics.get());
    }
    RunSummary summary;
    summary.slots = intro.assignedSlots();
    summary.wordmark = intro.wordmarkStats();
    summary.raisedMask = intro.raisedLetterMask();
    const std::uint32_t raiseTarget = intro.panelCount();
    summary.panels = raiseTarget;
    summary.ribbon = std::min(intro.ribbonCount(), options.settings.nanites - intro.assignedSlots());
    const int ticks = static_cast<int>(options.seconds / options.settings.tickSeconds + 0.5F);
    std::FILE* metrics = nullptr;
    if (capture && !options.metricsPath.empty())
    {
        metrics = std::fopen(options.metricsPath.c_str(), "w");
        if (metrics)
        {
            std::fprintf(metrics, "time,hero,near,far,hidden,migrations,visible_movers,visible_overlaps,visible_min_gap,"
                                  "locked,moored,dock_clicks,hero_yields,us_gather,us_signal,us_behaviour,us_resolve,us_lod,us_metrics,climbers,off_surface\n");
        }
    }
    if (capture && !options.framesDir.empty()) std::filesystem::create_directories(options.framesDir);
    std::vector<RenderRow> rows;
    std::uint32_t windowClicks = 0;
    int windowStart = 0;
    for (int t = 0; t < ticks; ++t)
    {
        intro.tick();
        const TickMetrics& m = intro.metrics();
        float total = 0.0F;
        for (std::size_t s = 0; s < m.systemMicros.size(); ++s)
        {
            total += m.systemMicros[s];
            summary.systemMicros[s] += m.systemMicros[s];
        }
        summary.tickMicros.push_back(total - m.systemMicros[5]);   // diagnostics excluded
        const float lockedFraction = summary.slots ? static_cast<float>(m.locked) / static_cast<float>(summary.slots) : 1.0F;
        if (m.locked > 0 && summary.firstLock < 0.0F) summary.firstLock = m.time;
        if (m.locked == summary.slots && summary.allLocked < 0.0F) summary.allLocked = m.time;
        if (raiseTarget > 0 && m.raised == raiseTarget && summary.allRaised < 0.0F) summary.allRaised = m.time;
        if (std::abs(m.time - 8.4F) < 0.5F * intro.stepSeconds()) summary.lockedAt84 = lockedFraction;
        if (std::abs(m.time - 10.0F) < 0.5F * intro.stepSeconds())
        {
            summary.lockedAt10 = lockedFraction;
            summary.mooredAt10 = summary.ribbon ? static_cast<float>(m.moored) / static_cast<float>(summary.ribbon) : 1.0F;
        }
        const float ratio = m.visibleMovers ? static_cast<float>(m.visibleOverlaps) / static_cast<float>(m.visibleMovers) : 0.0F;
        if (m.visibleOverlaps > summary.worstOverlaps)
        {
            summary.worstOverlaps = m.visibleOverlaps;
            summary.worstOverlapTime = m.time;
        }
        summary.worstOverlapRatio = std::max(summary.worstOverlapRatio, ratio);
        if (m.visibleMinGap >= 0.0F) summary.minVisibleGap = std::min(summary.minVisibleGap, m.visibleMinGap);
        summary.maxHeroes = std::max(summary.maxHeroes, m.tierCount[0]);
        if (t > 0) summary.maxMigrations = std::max(summary.maxMigrations, m.migrations);   // tick0 seeds every tier
        summary.yields += m.heroYields;
        summary.claims += m.claims;
        summary.stuck += m.stuck;
        summary.recovered += m.recovered;
        summary.pushes += m.pushes;
        summary.reclaims += m.reclaims;
        summary.maxStuckNow = std::max(summary.maxStuckNow, m.stuckNow);
        summary.lockedAtEnd = m.locked;
        summary.stuckAtEnd = m.stuckNow;
        summary.climberTicks += m.climbers;
        summary.swaps += m.goalSwaps;
        summary.unsupported += m.unsupported;
        summary.offSurface += m.offSurface;
        summary.clicks += m.dockClicks;
        windowClicks += m.dockClicks;
        if (t - windowStart >= 6)
        {
            summary.peakClicksPer100ms = std::max(summary.peakClicksPer100ms, windowClicks);
            windowClicks = 0;
            windowStart = t;
        }
        if ((t + 1) % 60 == 0) summary.hash = summary.hash * 1099511628211ull ^ intro.stateHash();   // once per second
        if (metrics)
        {
            std::fprintf(metrics, "%.4f,%u,%u,%u,%u,%u,%u,%u,%.4f,%u,%u,%u,%u,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%u,%u\n", m.time,
                         m.tierCount[0], m.tierCount[1], m.tierCount[2], m.tierCount[3], m.migrations, m.visibleMovers,
                         m.visibleOverlaps, m.visibleMinGap, m.locked, m.moored, m.dockClicks, m.heroYields, m.systemMicros[0],
                         m.systemMicros[1], m.systemMicros[2], m.systemMicros[3], m.systemMicros[4], m.systemMicros[5],
                         m.climbers, m.offSurface);
        }
        if (capture && !options.framesDir.empty() && (t + 1) % options.every == 0)
        {
            intro.snapshot(rows);
            char name[64];
            std::snprintf(name, sizeof name, "/frame_%04d.csv", t + 1);
            if (std::FILE* f = std::fopen((options.framesDir + name).c_str(), "w"))
            {
                const Camera& cam = intro.camera();
                std::fprintf(f, "# time=%.6f cx=%.4f cy=%.4f half_width=%.4f aspect=%.6f view=%s tilt=%.3f tile=%.4f pace=%.6f\n", m.time, cam.cx,
                             cam.cy, cam.halfWidth, cam.aspect, options.settings.build != Build::Lying ? "oblique" : "top",
                             options.settings.viewTilt, options.settings.wordmark.tile, options.settings.pace);
                std::fprintf(f, "x,y,z,heading,act,tier,act_since,plane,magnet,layer,s0x,s0y,s1x,s1y,s2x,s2y\n");
                for (const RenderRow& r : rows)
                {
                    std::fprintf(f, "%.3f,%.3f,%.3f,%.4f,%s,%d,%.3f,%.4f,%u,%u", r.x, r.y, r.z, r.heading, actName(r.act),
                                 static_cast<int>(r.tier), r.actSince, r.plane, static_cast<unsigned>(r.magnet), static_cast<unsigned>(r.layer));
                    if (r.shaped)
                    {
                        std::fprintf(f, ",%.4f,%.4f,%.4f,%.4f,%.4f,%.4f", r.shape[0], r.shape[1], r.shape[2], r.shape[3], r.shape[4], r.shape[5]);
                    }
                    std::fputc('\n', f);
                }
                std::fclose(f);
            }
        }
    }
    if (metrics) std::fclose(metrics);
    if (record)
    {
        // Cold export after the last tick: JSONL messages, then the health receipt.
        intro.recordSummary();
        std::ofstream events(options.eventsPath);
        std::ofstream healthFile;
        if (!options.healthPath.empty()) healthFile.open(options.healthPath);
        std::ostream& health = options.healthPath.empty() ? static_cast<std::ostream&>(std::cerr) : healthFile;
        summary.diagnosticsOk = diagnostics->writeJson(events, health);
    }
    return summary;
}

float percentile(std::vector<float> values, float p)
{
    if (values.empty()) return 0.0F;
    std::sort(values.begin(), values.end());
    const std::size_t k = std::min(values.size() - 1, static_cast<std::size_t>(p * static_cast<float>(values.size() - 1) + 0.5F));
    return values[k];
}

void print(const std::string& label, const Options& options, const RunSummary& s)
{
    const double ticks = static_cast<double>(s.tickMicros.size());
    // Beats and gates are choreography seconds; the presentation block is what a viewer sees.
    const double pace = options.settings.pace;
    const auto present = [&](float t) { return t < 0.0F ? -1.0 : static_cast<double>(t) / pace; };
    std::printf("{\"run\":\"%s\",\"nanites\":%zu,\"slots\":%zu,\"ribbon\":%zu,\"hero_cap\":%zu,"
                "\"first_lock_s\":%.3f,\"all_locked_s\":%.3f,\"all_raised_s\":%.3f,\"raised_letters\":%u,\"panels\":%u,\"locked_at_8_4\":%.4f,\"locked_at_10\":%.4f,\"moored_at_10\":%.4f,"
                "\"worst_visible_overlaps\":%u,\"worst_overlap_ratio\":%.4f,\"worst_overlap_time_s\":%.3f,\"min_visible_gap\":%.3f,"
                "\"max_heroes\":%u,\"max_migrations_per_tick\":%u,\"hero_yields\":%llu,\"goal_swaps\":%llu,\"claims\":%llu,\"stuck\":%llu,\"recovered\":%llu,\"pushes\":%llu,\"reclaims\":%llu,\"max_stuck_now\":%u,\"stuck_at_end\":%u,\"climber_ticks\":%llu,\"diagnostics_ok\":%s,\"locked_at_end\":%u,\"unsupported_locks\":%llu,\"off_surface\":%llu,\"dock_clicks\":%llu,\"peak_clicks_per_100ms\":%u,"
                "\"tick_us_p50\":%.1f,\"tick_us_p95\":%.1f,\"tick_us_max\":%.1f,"
                "\"mean_us\":{\"gather\":%.1f,\"signal\":%.1f,\"behaviour\":%.1f,\"resolve\":%.1f,\"lod\":%.1f,\"metrics\":%.1f},"
                "\"outline\":{\"corners\":%zu,\"rms_before\":%.3f,\"rms_after\":%.3f,\"max_morph\":%.3f,\"min_area_ratio\":%.3f},"
                "\"pace\":%.4f,\"presentation\":{\"length_s\":%.3f,\"all_locked_s\":%.3f,\"all_raised_s\":%.3f},"
                "\"state_hash\":\"%016llx\"}\n",
                label.c_str(), options.settings.nanites, s.slots, s.ribbon, options.settings.heroCap, s.firstLock, s.allLocked, s.allRaised, static_cast<unsigned>(s.raisedMask), s.panels, s.lockedAt84,
                s.lockedAt10, s.mooredAt10, s.worstOverlaps, s.worstOverlapRatio, s.worstOverlapTime, s.minVisibleGap, s.maxHeroes,
                s.maxMigrations, static_cast<unsigned long long>(s.yields), static_cast<unsigned long long>(s.swaps), static_cast<unsigned long long>(s.claims), static_cast<unsigned long long>(s.stuck), static_cast<unsigned long long>(s.recovered), static_cast<unsigned long long>(s.pushes), static_cast<unsigned long long>(s.reclaims), s.maxStuckNow, s.stuckAtEnd, static_cast<unsigned long long>(s.climberTicks), s.diagnosticsOk ? "true" : "false", s.lockedAtEnd, static_cast<unsigned long long>(s.unsupported), static_cast<unsigned long long>(s.offSurface), static_cast<unsigned long long>(s.clicks),
                s.peakClicksPer100ms, percentile(s.tickMicros, 0.5F), percentile(s.tickMicros, 0.95F),
                percentile(s.tickMicros, 1.0F), s.systemMicros[0] / ticks, s.systemMicros[1] / ticks, s.systemMicros[2] / ticks,
                s.systemMicros[3] / ticks, s.systemMicros[4] / ticks, s.systemMicros[5] / ticks,
                s.wordmark.boundaryCorners, s.wordmark.outlineRmsBefore, s.wordmark.outlineRmsAfter, s.wordmark.maxMorph,
                s.wordmark.minAreaRatio, pace, ticks * static_cast<double>(options.settings.tickSeconds), present(s.allLocked),
                present(s.allRaised), static_cast<unsigned long long>(s.hash));
}

bool parse(int argc, char** argv, Options& o)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view a = argv[i];
        auto next = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
        if (a == "--verify") o.verify = true;
        else if (a == "--build")
        {
            const char* v = next();
            if (!v) return false;
            const std::string_view m = v;
            if (m == "lying") o.settings.build = Build::Lying;
            else if (m == "climb") o.settings.build = Build::Climb;
            else if (m == "raise") o.settings.build = Build::Raise;
            else if (m == "hybrid") o.settings.build = Build::Hybrid;
            else if (m == "scaffold") o.settings.build = Build::Scaffold;
            else return false;
        }
        else if (a == "--rows") o.settings.wordmark.tile = 0.0F;
        else if (a == "--raise-letters") { const char* v = next(); if (!v) return false; o.settings.hybridRaise = static_cast<std::uint8_t>(std::strtoul(v, nullptr, 0)); }
        else if (a == "--morph") { const char* v = next(); if (!v) return false; o.settings.wordmark.morph = std::strtof(v, nullptr); }
        else if (a == "--claim-cap") { const char* v = next(); if (!v) return false; o.settings.claimCap = static_cast<std::uint32_t>(std::atoi(v)); }
        else if (a == "--count") { const char* v = next(); if (!v) return false; o.settings.nanites = std::strtoul(v, nullptr, 10); }
        else if (a == "--hero-cap") { const char* v = next(); if (!v) return false; o.settings.heroCap = std::strtoul(v, nullptr, 10); }
        else if (a == "--layers") { const char* v = next(); if (!v) return false; o.settings.wordmark.layers = std::max(1, std::atoi(v)); }
        else if (a == "--pace") { const char* v = next(); if (!v) return false; o.settings.pace = std::strtof(v, nullptr); }
        else if (a == "--seconds") { const char* v = next(); if (!v) return false; o.seconds = std::strtof(v, nullptr); }
        else if (a == "--frames") { const char* v = next(); if (!v) return false; o.framesDir = v; }
        else if (a == "--every") { const char* v = next(); if (!v) return false; o.every = std::max(1, std::atoi(v)); }
        else if (a == "--metrics") { const char* v = next(); if (!v) return false; o.metricsPath = v; }
        else if (a == "--events-jsonl") { const char* v = next(); if (!v) return false; o.eventsPath = v; }
        else if (a == "--health-json") { const char* v = next(); if (!v) return false; o.healthPath = v; }
        else if (a == "--lod")
        {
            const char* v = next();
            if (!v) return false;
            const std::string_view m = v;
            if (m == "camera") o.settings.lod = LodMode::Camera;
            else if (m == "hero") o.settings.lod = LodMode::AllHero;
            else if (m == "near") o.settings.lod = LodMode::AllNear;
            else if (m == "far") o.settings.lod = LodMode::AllFar;
            else return false;
        }
        else return false;
    }
    return true;
}

int fail(const char* what)
{
    std::printf("{\"verify\":\"FAIL\",\"check\":\"%s\"}\n", what);
    return 1;
}
} // namespace

int main(int argc, char** argv)
{
    Options options;
    if (!parse(argc, argv, options))
    {
        std::fprintf(stderr,
                     "usage: crucible_swarm_intro [--verify] [--build lying|climb|raise|hybrid|scaffold] [--raise-letters MASK] [--rows] [--morph F] [--layers N] [--count N] [--hero-cap N] [--lod camera|hero|near|far]\n"
                     "                            [--seconds S] [--pace P] [--frames DIR] [--every TICKS] [--metrics FILE]\n"
                     "                            [--events-jsonl FILE] [--health-json FILE] [--claim-cap N]\n");
        return 2;
    }
    try
    {
        const Build build = options.settings.build;
        // Climbing builds use the fifteen-second sequence (user direction), in choreography
        // seconds: at pace 2/3 a viewer sees 22.5 s.
        const bool climbs = build == Build::Climb || build == Build::Hybrid || build == Build::Scaffold;
        const float choreography = climbs ? 15.0F : 12.0F;
        if (options.settings.pace <= 0.0F) return fail("pace is positive");
        if (options.seconds <= 0.0F) options.seconds = choreography / options.settings.pace;
        if (options.verify && climbs) options.seconds = std::max(options.seconds, choreography / options.settings.pace);
        const RunSummary main = run(options, true);
        print(label(options.settings), options, main);
        if (!options.verify) return 0;

        // Acceptance per build; thresholds and their history are recorded in README.md.
        if (main.unsupported != 0) return fail("every lock has a locked support");
        if (!main.diagnosticsOk) return fail("Sub0Log diagnostics export is valid, complete and undamaged");
        if (main.stuckAtEnd != 0) return fail("every stuck episode is resolved by the end of the run");
        if (options.settings.wordmark.tile > 0.0F && options.settings.wordmark.morph > 0.0F)
        {
            // Morphing smooths the outline without degenerating any tile.
            if (main.wordmark.minAreaRatio < 0.499F) return fail("no morphed tile below half its area");
            if (main.wordmark.outlineRmsAfter > 0.6F * main.wordmark.outlineRmsBefore) return fail("morphing cuts outline error by 40% or more");
        }
        if (main.maxHeroes > options.settings.heroCap) return fail("hero tier respects its cap");
        if (main.mooredAt10 < 1.0F) return fail("ribbon fully moored by 10.0s");
        switch (build)
        {
        case Build::Lying:
        case Build::Raise:
            if (main.lockedAt84 < 0.99F) return fail("title >=99% locked by 8.4s (shot04 exit)");
            if (main.lockedAt10 < 1.0F) return fail("title fully locked by 10.0s");
            if (build == Build::Raise && (main.allRaised < 0.0F || main.allRaised > 10.0F)) return fail("every letter upright by 10.0s (shot05 hold)");
            if (main.worstOverlapRatio > 0.03F) return fail("visible mover contacts (<0.8d) at most 3% per tick");
            if (main.minVisibleGap < 0.5F) return fail("no visible mover passes through another (gap >= 0.5d)");
            break;
        case Build::Hybrid:
        case Build::Scaffold:
            if (main.allRaised < 0.0F || main.allRaised > 13.5F) return fail("every drawbridge panel upright by 13.5s");
            [[fallthrough]];
        case Build::Climb:
            // The fifteen-second sequence (user direction): title complete by 12.5 s leaves a
            // one-second hold and the menu handoff by 15 s.
            if (main.allLocked < 0.0F || main.allLocked > 12.5F) return fail("climbed title complete by 12.5s");
            if (main.climberTicks == 0 || main.offSurface * 200 > main.climberTicks) return fail("climbers stay attached to the built face (<=0.5% of climber-ticks off)");
            // Tightened with lane branching (S0h): was 10% and 0.3d while lane exits funnelled.
            if (main.worstOverlapRatio > 0.03F) return fail("visible ground contacts at most 3% per tick");
            if (main.minVisibleGap < 0.5F) return fail("no visible ground mover passes through another (gap >= 0.5d)");
            break;
        }

        const RunSummary again = run(options, false);
        if (again.hash != main.hash) return fail("identical runs produce identical state");

        // LOD changes cost and texture, never the outcome: a uniform far tier completes too.
        Options far = options;
        far.settings.lod = LodMode::AllFar;
        const RunSummary farRun = run(far, false);
        print(label(far.settings), far, farRun);
        if (farRun.allLocked < 0.0F || farRun.allLocked > (build == Build::Climb || build == Build::Hybrid || build == Build::Scaffold ? 14.0F : 10.0F)) return fail("uniform far tier completes the title");
        if (build == Build::Lying)
        {
            Options hero = options;
            hero.settings.lod = LodMode::AllHero;
            const RunSummary heroRun = run(hero, false);
            print(label(hero.settings), hero, heroRun);
            // Full deliberation everywhere is slower (measured): it must still finish. 11.0 s for
            // 959 row slots, scaled to 12.5 s when tessellation raised the count to 1104.
            if (heroRun.allLocked < 0.0F || heroRun.allLocked > 12.5F) return fail("uniform hero tier completes the title by 12.5s");
        }
        std::printf("{\"verify\":\"PASS\"}\n");
        return 0;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 3;
    }
}
