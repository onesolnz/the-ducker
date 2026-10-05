#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace ducker
{
    // One breakpoint of the volume curve: tick 0 to kCycleTicks across one pass, value 1 = full volume, 0 = silent.
    // `bend` (-1 to 1) bends the line to the next point between a slow start and a slow finish (the same maths as Ingot's automation).
    struct CurvePoint
    {
        std::int64_t tick = 0;
        double value = 1.0;
        double bend = 0.0;
        bool operator== (const CurvePoint& o) const { return tick == o.tick && value == o.value && bend == o.bend; }
    };

    using Curve = std::vector<CurvePoint>;

    constexpr int kCycleTicks = 960;

    // The curve's volume (0 to 1) at a phase 0 to 1 of one pass.
    float curveAt (const Curve& points, double phase);

    // Sorted, clamped to the pass, first point at 0 and last at kCycleTicks (a curve always spans the whole pass).
    Curve tidyCurve (Curve points);

    struct Shape
    {
        const char* name;
        Curve points;
    };

    // The 13 factory shapes (also the factory presets), as in Ingot's Ducker.
    const std::vector<Shape>& factoryShapes();

    // The curve as a table the audio thread reads. setCurve (message thread) fills a table the audio thread is not reading,
    // then switches to it, so the audio thread never waits or sees half a curve.
    class CurveTable
    {
    public:
        static constexpr int kSize = 1024;
        using Table = std::array<float, kSize + 1>;

        CurveTable();
        void setCurve (const Curve& points);

        // Audio thread: the table to read for this block.
        const Table& current() const noexcept { return tables[(size_t) active.load (std::memory_order_acquire)]; }

        // Linear read of a table at a phase 0 to 1.
        static float read (const Table& t, double phase) noexcept
        {
            const double at = (phase < 0.0 ? 0.0 : (phase > 1.0 ? 1.0 : phase)) * kSize;
            const int i = at >= kSize ? kSize - 1 : (int) at;
            return t[(size_t) i] + (t[(size_t) i + 1] - t[(size_t) i]) * (float) (at - i);
        }

    private:
        static constexpr int kTables = 3;
        std::array<Table, kTables> tables {};
        std::atomic<int> active { 0 };
        int next = 1;
    };
}
