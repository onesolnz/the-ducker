#include "Curve.h"

#include <algorithm>
#include <cmath>

namespace ducker
{
    float curveAt (const Curve& points, double phase)
    {
        if (points.empty())
            return 1.0f;
        const double tick = std::clamp (phase, 0.0, 1.0) * kCycleTicks;
        if (tick <= (double) points.front().tick) return (float) points.front().value;
        if (tick >= (double) points.back().tick) return (float) points.back().value;

        const auto next = std::upper_bound (points.begin(), points.end(), tick, [] (double t, const CurvePoint& p) { return t < (double) p.tick; });
        const auto& b = *next;
        const auto& a = *(next - 1);
        const double span = (double) (b.tick - a.tick);
        double t = span > 0.0 ? (tick - (double) a.tick) / span : 1.0;
        if (a.bend != 0.0)
            t = std::pow (t, std::pow (4.0, std::clamp (a.bend, -1.0, 1.0)));
        return (float) std::clamp (a.value + (b.value - a.value) * t, 0.0, 1.0);
    }

    Curve tidyCurve (Curve points)
    {
        for (auto& p : points)
        {
            p.tick = std::clamp<std::int64_t> (p.tick, 0, kCycleTicks);
            p.value = std::clamp (p.value, 0.0, 1.0);
            p.bend = std::clamp (p.bend, -1.0, 1.0);
        }
        std::stable_sort (points.begin(), points.end(), [] (const CurvePoint& a, const CurvePoint& b) { return a.tick < b.tick; });
        if (points.empty())
            return { { 0, 1.0, 0.0 }, { kCycleTicks, 1.0, 0.0 } };
        if (points.front().tick != 0)
            points.insert (points.begin(), { 0, points.front().value, 0.0 });
        if (points.back().tick != kCycleTicks)
            points.push_back ({ kCycleTicks, points.back().value, 0.0 });
        return points;
    }

    const std::vector<Shape>& factoryShapes()
    {
        static const std::vector<Shape> list
        {
            { "Kick start",    { { 0, 0.0, -0.45 }, { 840, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Gentle pump",   { { 0, 0.45, -0.3 }, { 700, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Hard chop",     { { 0, 0.0, 0.0 }, { 360, 0.0, 0.0 }, { 400, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Sidechain-ish", { { 0, 0.1, -0.5 }, { 600, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Flat",          { { 0, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Ramp",          { { 0, 0.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Swell",         { { 0, 0.0, 0.6 }, { 960, 1.0, 0.0 } } },
            { "Triangle",      { { 0, 1.0, 0.0 }, { 480, 0.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Gate",          { { 0, 0.0, 0.0 }, { 432, 0.0, 0.0 }, { 480, 1.0, 0.0 }, { 912, 1.0, 0.0 }, { 960, 0.0, 0.0 } } },
            { "Double",        { { 0, 0.0, -0.4 }, { 240, 1.0, 0.0 }, { 480, 0.0, -0.4 }, { 720, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Stutter",       { { 0, 0.0, 0.0 }, { 115, 0.0, 0.0 }, { 144, 1.0, 0.0 }, { 336, 1.0, 0.0 }, { 365, 0.0, 0.0 }, { 480, 0.0, 0.0 }, { 509, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Soft dip",      { { 0, 1.0, 0.0 }, { 192, 0.35, 0.0 }, { 672, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
            { "Late pump",     { { 0, 1.0, 0.0 }, { 288, 1.0, 0.0 }, { 336, 0.0, -0.4 }, { 816, 1.0, 0.0 }, { 960, 1.0, 0.0 } } },
        };
        return list;
    }

    CurveTable::CurveTable()
    {
        for (auto& t : tables)
            t.fill (1.0f);
    }

    void CurveTable::setCurve (const Curve& points)
    {
        const int slot = next;
        next = (next + 1) % kTables;
        if (next == active.load())
            next = (next + 1) % kTables;
        auto& t = tables[(size_t) slot];
        for (int i = 0; i <= kSize; ++i)
            t[(size_t) i] = curveAt (points, (double) i / kSize);
        active.store (slot, std::memory_order_release);
    }
}
