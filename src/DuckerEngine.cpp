#include "DuckerEngine.h"

#include <algorithm>
#include <cmath>

namespace ducker
{
    // ---------- HitDetector ----------

    void HitDetector::prepare (double sampleRate)
    {
        const auto samples = [sampleRate] (double ms) { return std::max (1, (int) std::lround (ms * 0.001 * sampleRate)); };
        history.assign ((size_t) samples (kLookBackMs), 0.0f);
        holdOffSamples = samples (kHoldOffMs);
        attack = (float) std::exp (-1.0 / (kAttackMs * 0.001 * sampleRate));
        release = (float) std::exp (-1.0 / (kReleaseMs * 0.001 * sampleRate));
        reset();
    }

    void HitDetector::reset() noexcept
    {
        std::fill (history.begin(), history.end(), 0.0f);
        writePos = 0;
        holdOff = 0;
        env = 0.0f;
    }

    bool HitDetector::process (float x) noexcept
    {
        x = std::abs (x);
        env = x > env ? x + (env - x) * attack : x + (env - x) * release;
        if (history.empty())
            return false;

        const float before = history[(size_t) writePos];        // the envelope kLookBackMs ago (the oldest kept)
        history[(size_t) writePos] = env;
        writePos = (writePos + 1) % (int) history.size();

        if (holdOff > 0)
        {
            --holdOff;
            return false;
        }
        if (env > kFloor && env > kRise * before)
        {
            holdOff = holdOffSamples;
            return true;
        }
        return false;
    }

    // ---------- DuckerEngine ----------

    void DuckerEngine::prepare (double rate)
    {
        sampleRate = rate > 0.0 ? rate : 44100.0;
        detector.prepare (sampleRate);
        reset();
    }

    void DuckerEngine::reset() noexcept
    {
        detector.reset();
        gainState = 1.0f;
        started = false;
        passPhase = 0.0;
        numPending = 0;
        shownDuck.store (0.0f);
        shownPhase.store (-1.0f);
    }

    void DuckerEngine::process (float* left, float* right, int numSamples, const float* scLeft, const float* scRight,
                                const int* noteOns, int numNoteOns, const Settings& s, const Transport& t) noexcept
    {
        if (numSamples <= 0)
            return;

        const auto& tab = table.current();
        const float duck = std::clamp (s.duck, 0.0f, 1.0f), mix = std::clamp (s.mix, 0.0f, 1.0f);
        const float coef = s.smoothMs > 0.0f ? (float) std::exp (-1.0 / (s.smoothMs * 0.001 * sampleRate)) : 0.0f;
        const double cycle = cycleBeats (s.rate);
        const double beatsPerSample = (t.bpm > 0.0 ? t.bpm : 120.0) / (60.0 * sampleRate);
        const double passPerSample = beatsPerSample / cycle;
        const int delaySamples = (int) std::lround (std::clamp (s.delayMs, 0.0f, 100.0f) * 0.001 * sampleRate);
        const bool beatRunning = s.trigger == Trigger::beat && t.playing && t.hasPosition;
        const bool listenAudio = s.trigger == Trigger::audio && scLeft != nullptr;
        const bool listenMidi = s.trigger == Trigger::midi;

        int nextNote = 0, newHits = 0;
        float blockIn = 0.0f, blockOut = 0.0f;
        double phase = -1.0;
        for (int i = 0; i < numSamples; ++i)
        {
            // starts that have waited out the Delay
            for (int k = 0; k < numPending; ++k)
                if (--pending[(size_t) k] <= 0)
                {
                    startPass();
                    pending[(size_t) k--] = pending[(size_t) --numPending];
                }

            bool hit = false;
            if (listenMidi)
                while (nextNote < numNoteOns && noteOns[nextNote] <= i)
                {
                    hit = true;
                    ++nextNote;
                }
            if (listenAudio)
            {
                const float r = scRight != nullptr ? std::abs (scRight[i]) : 0.0f;
                hit = detector.process (std::max (std::abs (scLeft[i]), r));
            }
            if (hit)
            {
                ++newHits;
                if (delaySamples == 0)
                    startPass();
                else if (numPending < kMaxPending)
                    pending[(size_t) numPending++] = delaySamples;
            }

            float target = 1.0f;
            if (beatRunning)
            {
                phase = (t.ppq + beatsPerSample * i) / cycle - s.offset;
                phase = std::max (0.0, phase - std::floor (phase + 1.0e-6));     // a beat reached by rounding error still counts as the beat
                target = 1.0f - duck * (1.0f - CurveTable::read (tab, phase));
            }
            else if (s.trigger != Trigger::beat && started)
            {
                phase = std::min (passPhase, 1.0);                                  // after one pass: hold the curve's end value
                target = 1.0f - duck * (1.0f - CurveTable::read (tab, phase));
                passPhase += passPerSample;
            }

            gainState = target + (gainState - target) * coef;
            const float g = 1.0f - mix * (1.0f - gainState);                       // exactly 1 when Mix is 0 or nothing is ducking
            const float in = std::max (std::abs (left[i]), std::abs (right[i]));
            left[i] *= g;
            if (right != left)
                right[i] *= g;
            blockIn = std::max (blockIn, in);
            blockOut = std::max (blockOut, in * g);
        }

        const float g = 1.0f - mix * (1.0f - gainState);
        shownDuck.store (1.0f - g, std::memory_order_relaxed);
        shownPhase.store ((float) phase, std::memory_order_relaxed);
        if (newHits > 0)
            hits.fetch_add (newHits, std::memory_order_relaxed);
        if (blockIn > inPeak.load (std::memory_order_relaxed))
            inPeak.store (blockIn, std::memory_order_relaxed);
        if (blockOut > outPeak.load (std::memory_order_relaxed))
            outPeak.store (blockOut, std::memory_order_relaxed);
    }
}
