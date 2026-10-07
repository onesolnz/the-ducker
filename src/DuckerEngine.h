#pragma once

#include <array>
#include <atomic>
#include <vector>

#include <onesol/HitDetector.h>

#include "Curve.h"

namespace ducker
{
    enum class Trigger { beat = 0, audio = 1, midi = 2 };

    using onesol::HitDetector;      // the Audio trigger's hit finder is shared (Shared/include/onesol/HitDetector.h)

    // Everything the knobs set, read once per block.
    struct Settings
    {
        float duck = 1.0f;          // 0 to 1
        float mix = 1.0f;           // 0 to 1
        float smoothMs = 2.0f;
        float offset = 0.0f;        // -0.5 to 0.5 of a cycle (Beat)
        float delayMs = 0.0f;       // 0 to 100 (Audio, MIDI)
        int rate = 0;               // 0 = 1/4, 1 = 1/8, 2 = 1/16
        Trigger trigger = Trigger::beat;
    };

    // What the host says about time for this block.
    struct Transport
    {
        bool playing = false;
        bool hasPosition = false;
        double ppq = 0.0;           // quarter notes at the block's first sample
        double bpm = 120.0;         // the host's tempo, or 120 if it gives none
    };

    // The Ducker's sound (specs/27-the-ducker-vst.md, "How it sounds"): a volume curve played on the beat (Beat) or from the
    // top on each sidechain hit (Audio) or note start (MIDI). It only ever changes the volume. Nothing here allocates or locks
    // once prepare() has run.
    class DuckerEngine
    {
    public:
        static constexpr int kMaxPending = 32;      // curve starts waiting out the Delay
        static constexpr int kLiveBuckets = 160;    // the sound folded onto one pass, for the bars behind the curve

        void prepare (double sampleRate);           // message thread
        void reset() noexcept;

        void setCurve (const Curve& c) { table.setCurve (c); }

        // left/right: the main signal, changed in place (right may equal left for mono). scLeft/scRight: the sidechain, or nullptr.
        // noteOns: the sample positions of this block's note starts, in order (MIDI trigger).
        void process (float* left, float* right, int numSamples, const float* scLeft, const float* scRight,
                      const int* noteOns, int numNoteOns, const Settings& s, const Transport& t) noexcept;

        static double cycleBeats (int rate) noexcept { return rate == 1 ? 0.5 : (rate == 2 ? 0.25 : 1.0); }

        // For the window (any thread).
        float getDuckAmount() const noexcept { return shownDuck.load (std::memory_order_relaxed); }   // 0 = full volume, 1 = silent
        float getPhase() const noexcept { return shownPhase.load (std::memory_order_relaxed); }       // -1 = no pass running
        int getHitCount() const noexcept { return hits.load (std::memory_order_relaxed); }           // goes up by one per hit or note
        // Highest input / output sample since the last call (the meters take them on their timer).
        float takeInPeak() noexcept { return inPeak.exchange (0.0f, std::memory_order_relaxed); }
        float takeOutPeak() noexcept { return outPeak.exchange (0.0f, std::memory_order_relaxed); }
        // The peak that came in and the peak that left in each slice of the pass, from its latest pass (any thread).
        float getLiveIn (int bucket) const noexcept  { return liveIn[(size_t) bucket].load (std::memory_order_relaxed); }
        float getLiveOut (int bucket) const noexcept { return liveOut[(size_t) bucket].load (std::memory_order_relaxed); }

    private:
        void startPass() noexcept { started = true; passPhase = 0.0; }

        CurveTable table;
        HitDetector detector;
        double sampleRate = 44100.0;
        float gainState = 1.0f;

        // Audio / MIDI pass
        bool started = false;
        double passPhase = 0.0;
        std::array<int, kMaxPending> pending {};    // samples left until each waiting start
        int numPending = 0;

        std::atomic<float> shownDuck { 0.0f }, shownPhase { -1.0f }, inPeak { 0.0f }, outPeak { 0.0f };
        std::atomic<int> hits { 0 };
        std::array<std::atomic<float>, kLiveBuckets> liveIn {}, liveOut {};
        int liveBucket = -1;
        float bucketIn = 0.0f, bucketOut = 0.0f;
    };
}
