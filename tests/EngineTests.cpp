#include <cmath>
#include <vector>

#include <juce_core/juce_core.h>

#include "DuckerEngine.h"
#include <onesol/RealtimeGuard.h>

using namespace ducker;

namespace
{
    constexpr double kRate = 44100.0;
    constexpr double kPi = 3.14159265358979323846;

    // A synthetic kick: a sine falling from 150 Hz to 45 Hz with an exponential decay (tau ms). `tailTau` adds a quieter, longer tail.
    void addKick (std::vector<float>& buf, int at, float level, double tauMs = 80.0, double tailTauMs = 0.0)
    {
        double phase = 0.0;
        for (int i = 0; at + i < (int) buf.size() && i < (int) (kRate * 1.5); ++i)
        {
            const double t = i / kRate;
            const double f = 45.0 + 105.0 * std::exp (-t / 0.03);
            phase += 2.0 * kPi * f / kRate;
            double env = std::exp (-t / (tauMs * 0.001));
            if (tailTauMs > 0.0)
                env = 0.75 * env + 0.25 * std::exp (-t / (tailTauMs * 0.001));
            buf[(size_t) (at + i)] += (float) (level * env * std::sin (phase));
        }
    }

    // Decaying noise after each kick, like a short reverb.
    void addReverb (std::vector<float>& buf, int at, float level, juce::Random& rnd)
    {
        for (int i = 0; at + i < (int) buf.size() && i < (int) (kRate * 0.8); ++i)
        {
            const double t = i / kRate;
            const double env = (1.0 - std::exp (-t / 0.004)) * std::exp (-t / 0.25);
            buf[(size_t) (at + i)] += (float) (level * 0.3 * env * (rnd.nextFloat() * 2.0f - 1.0f));
        }
    }

    std::vector<int> detect (const std::vector<float>& sc)
    {
        HitDetector d;
        d.prepare (kRate);
        std::vector<int> hits;
        for (int i = 0; i < (int) sc.size(); ++i)
            if (d.process (sc[(size_t) i]))
                hits.push_back (i);
        return hits;
    }

    // Plays a constant 1.0 through the engine in blocks; returns the gain per sample. noteAt: absolute note-on samples.
    std::vector<float> run (DuckerEngine& e, const Settings& s, int total, int block, Transport t, std::vector<int> noteAt = {},
                            const std::vector<float>* sc = nullptr, double seekAtBeat = -1.0, int seekAtSample = -1)
    {
        std::vector<float> out;
        std::vector<float> l ((size_t) block), r ((size_t) block);
        std::vector<int> notes;
        for (int start = 0; start < total; start += block)
        {
            const int n = std::min (block, total - start);
            if (seekAtSample >= 0 && start >= seekAtSample)
            {
                t.ppq = seekAtBeat;
                seekAtSample = -1;
            }
            std::fill (l.begin(), l.end(), 1.0f);
            std::fill (r.begin(), r.end(), 1.0f);
            notes.clear();
            for (int a : noteAt)
                if (a >= start && a < start + n)
                    notes.push_back (a - start);
            const float* scp = sc != nullptr ? sc->data() + start : nullptr;
            e.process (l.data(), r.data(), n, scp, scp, notes.data(), (int) notes.size(), s, t);
            for (int i = 0; i < n; ++i)
                out.push_back (l[(size_t) i]);
            t.ppq += n * t.bpm / (60.0 * kRate);
        }
        return out;
    }

    Transport playing (double bpm = 120.0)
    {
        Transport t;
        t.playing = t.hasPosition = true;
        t.bpm = bpm;
        return t;
    }

    Settings hard (Trigger trig)
    {
        Settings s;
        s.smoothMs = 0.0f;              // exact curve values, no rounding of corners
        s.trigger = trig;
        return s;
    }
}

// specs/27-the-ducker-vst.md, "How it sounds" and "Tests".
class EngineTests : public juce::UnitTest
{
public:
    EngineTests() : juce::UnitTest ("Engine", "TheDucker") {}

    void runTest() override
    {
        const Curve ramp { { 0, 0.0, 0.0 }, { 960, 1.0, 0.0 } };          // silent at the start, full at the end of a pass
        const Curve rampToHalf { { 0, 0.0, 0.0 }, { 960, 0.5, 0.0 } };    // ends at half volume, to see the hold

        beginTest ("Duck 0 and Mix 0 are bit-exact pass-through in every mode");
        {
            juce::Random rnd (7);
            std::vector<float> sc ((size_t) kRate * 2, 0.0f);
            for (int k = 0; k < 8; ++k)
                addKick (sc, k * 11025, 0.9f);
            for (auto trig : { Trigger::beat, Trigger::audio, Trigger::midi })
                for (int which = 0; which < 2; ++which)
                {
                    DuckerEngine e;
                    e.prepare (kRate);
                    e.setCurve (ramp);
                    Settings s;
                    s.trigger = trig;
                    (which == 0 ? s.duck : s.mix) = 0.0f;
                    auto t = playing();
                    bool exact = true;
                    std::vector<float> l (512), r (512);
                    for (int start = 0; start + 512 <= (int) sc.size(); start += 512)
                    {
                        for (int i = 0; i < 512; ++i)
                            l[(size_t) i] = r[(size_t) i] = rnd.nextFloat() * 2.0f - 1.0f;
                        auto keepL = l, keepR = r;
                        int note[] = { 100 };
                        e.process (l.data(), r.data(), 512, sc.data() + start, sc.data() + start, note, 1, s, t);
                        exact = exact && l == keepL && r == keepR;
                        t.ppq += 512 * t.bpm / (60.0 * kRate);
                    }
                    expect (exact, "mode " + juce::String ((int) trig) + (which == 0 ? " Duck 0" : " Mix 0"));
                }
        }

        beginTest ("Beat: the curve starts on every beat for any block size");
        for (int block : { 32, 441, 1000, 4096 })
        {
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (ramp);
            const auto g = run (e, hard (Trigger::beat), (int) kRate * 2, block, playing (120.0));     // a beat every 22050 samples
            for (int beat = 1; beat < 4; ++beat)
            {
                const int at = beat * 22050;
                expectWithinAbsoluteError (g[(size_t) at], 0.0f, 0.001f, "beat " + juce::String (beat) + ", block " + juce::String (block));
                expectGreaterThan (g[(size_t) at - 1], 0.99f);
                expectWithinAbsoluteError (g[(size_t) at + 11025], 0.5f, 0.002f);
            }
        }

        beginTest ("Beat: Offset, rate, seek and stop");
        {
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (ramp);
            auto s = hard (Trigger::beat);
            s.offset = 0.25f;                                          // a quarter of the cycle later
            auto g = run (e, s, 44100, 441, playing());
            expectWithinAbsoluteError (g[22050 + 5513], 0.0f, 0.002f, "offset moves the start a quarter beat later");

            s.offset = 0.0f;
            s.rate = 2;                                                // 1/16: a pass every 5512.5 samples
            e.reset();
            g = run (e, s, 44100, 441, playing());
            expectWithinAbsoluteError (g[11025], 0.0f, 0.002f);
            expectWithinAbsoluteError (g[11025 + 2756], 0.5f, 0.005f);

            s.rate = 0;
            e.reset();
            g = run (e, s, 44100, 441, playing(), {}, nullptr, 10.0, 441 * 30);     // seek to beat 10 at sample 13230
            expectWithinAbsoluteError (g[441 * 30], 0.0f, 0.001f, "a seek to a beat restarts the curve there");

            auto stopped = playing();
            stopped.playing = false;
            e.reset();
            g = run (e, s, 22050, 441, stopped);
            expect (*std::min_element (g.begin(), g.end()) == 1.0f, "stopped host: full volume");
        }

        beginTest ("MIDI: a note start begins the curve on its exact sample, in any block");
        for (int block : { 64, 441, 1024 })
        {
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (ramp);
            const int note = 5000;
            const auto g = run (e, hard (Trigger::midi), 30000, block, playing(), { note });
            expect (g[(size_t) note - 1] == 1.0f, "full volume before the first note");
            expectWithinAbsoluteError (g[(size_t) note], 0.0f, 0.001f, "block " + juce::String (block));
            expectWithinAbsoluteError (g[(size_t) note + 11025], 0.5f, 0.002f, "half way through a 1/4 pass at 120 BPM");
        }

        beginTest ("MIDI: a note during a pass restarts it; after a pass the end value holds");
        {
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (rampToHalf);
            const auto g = run (e, hard (Trigger::midi), 80000, 512, playing(), { 1000, 12000 });
            expectWithinAbsoluteError (g[12000], 0.0f, 0.001f, "restart");
            expectWithinAbsoluteError (g[12000 + 22050 + 10], 0.5f, 0.001f, "hold after one pass");
            expectWithinAbsoluteError (g[79999], 0.5f, 0.001f, "still holding");
        }

        beginTest ("MIDI: Length follows the rate and the host tempo, also while stopped");
        {
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (ramp);
            auto s = hard (Trigger::midi);
            s.rate = 1;                                                // 1/8 at 60 BPM: a pass is half a second
            auto t = playing (60.0);
            t.playing = false;
            const auto g = run (e, s, 40000, 256, t, { 0 });
            expectWithinAbsoluteError (g[11025], 0.5f, 0.002f);
            expectWithinAbsoluteError (g[22050 + 5], 1.0f, 0.001f);
        }

        beginTest ("Delay: starts later by exactly the delay, across block boundaries");
        for (int block : { 100, 441, 2048 })
        {
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (ramp);
            auto s = hard (Trigger::midi);
            s.delayMs = 10.0f;                                         // 441 samples
            const auto g = run (e, s, 30000, block, playing(), { 1000, 1100 });
            expect (g[1440] == 1.0f, "nothing before the delayed start, block " + juce::String (block));
            expectWithinAbsoluteError (g[1441], 0.0f, 0.001f, "first start");
            expectWithinAbsoluteError (g[1541], 0.0f, 0.001f, "the second note's start also waits its own delay");
        }

        beginTest ("Smooth rounds a hard drop");
        {
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (ramp);
            auto s = hard (Trigger::midi);
            s.smoothMs = 2.0f;
            const auto g = run (e, s, 4000, 256, playing(), { 1000 });
            const float maxStep = 1.0f - (float) std::exp (-1.0 / (0.002 * kRate));
            float worst = 0.0f;
            for (size_t i = 1; i < g.size(); ++i)
                worst = std::max (worst, std::abs (g[i] - g[i - 1]));
            expect (worst <= maxStep + 1.0e-5f, "largest step " + juce::String (worst));
            expectGreaterThan (g[1000], 0.9f);
        }

        beginTest ("Hit detection: one hit per kick, on time, at any level");
        for (float db : { -30.0f, -18.0f, -6.0f, 0.0f })
        {
            const float level = std::pow (10.0f, db / 20.0f);
            std::vector<float> sc ((size_t) kRate * 4, 0.0f);
            std::vector<int> at;
            for (int k = 0; k < 8; ++k)
                at.push_back (1000 + k * 22050);
            for (int a : at)
                addKick (sc, a, level);
            const auto hits = detect (sc);
            expectEquals ((int) hits.size(), (int) at.size(), "kicks at " + juce::String (db) + " dB");
            for (size_t k = 0; k < std::min (hits.size(), at.size()); ++k)
                expect (std::abs (hits[k] - at[k]) <= (int) (0.002 * kRate), "hit " + juce::String ((int) k) + " late by " + juce::String (hits[k] - at[k]));
        }

        beginTest ("Hit detection: long tails and reverb count once");
        {
            juce::Random rnd (3);
            std::vector<float> tail ((size_t) kRate * 4, 0.0f), wet ((size_t) kRate * 4, 0.0f);
            for (int k = 0; k < 8; ++k)
            {
                addKick (tail, 1000 + k * 22050, 0.8f, 60.0, 400.0);
                addKick (wet, 1000 + k * 22050, 0.8f);
                addReverb (wet, 1000 + k * 22050, 0.8f, rnd);
            }
            expectEquals ((int) detect (tail).size(), 8, "long tail");
            expectEquals ((int) detect (wet).size(), 8, "with reverb");
        }

        beginTest ("Hit detection: silence and low noise give nothing; fast 1/16 kicks are all caught");
        {
            juce::Random rnd (11);
            std::vector<float> noise ((size_t) kRate * 2);
            for (auto& x : noise)
                x = 0.001f * (rnd.nextFloat() * 2.0f - 1.0f);         // -60 dBFS
            expectEquals ((int) detect (std::vector<float> ((size_t) kRate, 0.0f)).size(), 0, "silence");
            expectEquals ((int) detect (noise).size(), 0, "low noise");

            std::vector<float> fast ((size_t) kRate * 3, 0.0f);
            const double step = kRate * 60.0 / 180.0 / 4.0;            // 1/16 at 180 BPM, about 83 ms
            int count = 0;
            for (double a = 500.0; a < kRate * 2.8; a += step, ++count)
                addKick (fast, (int) a, 0.8f, 40.0);
            expectEquals ((int) detect (fast).size(), count, "1/16 at 180 BPM");
        }

        beginTest ("Audio trigger: a sidechain kick starts the curve; no sidechain does nothing");
        {
            std::vector<float> sc (30000, 0.0f);
            addKick (sc, 4000, 0.8f);
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (ramp);
            auto g = run (e, hard (Trigger::audio), 30000, 441, playing(), {}, &sc);
            const auto low = std::min_element (g.begin(), g.end()) - g.begin();
            expect (std::abs ((int) low - 4000) <= 88, "curve starts at the kick (sample " + juce::String ((int) low) + ")");
            expectEquals (e.getHitCount(), 1);

            DuckerEngine none;
            none.prepare (kRate);
            none.setCurve (ramp);
            g = run (none, hard (Trigger::audio), 30000, 441, playing(), { 100 });
            expect (*std::min_element (g.begin(), g.end()) == 1.0f, "no sidechain and MIDI notes ignored in Audio mode");
        }

        beginTest ("Nothing allocated on the audio thread");
        if (onesol::realtime::isAvailable())
        {
            std::vector<float> sc (20000, 0.0f);
            addKick (sc, 2000, 0.8f);
            DuckerEngine e;
            e.prepare (kRate);
            e.setCurve (ramp);
            std::vector<float> l (512, 1.0f), r (512, 1.0f);
            int notes[] = { 3, 200 };
            onesol::realtime::resetViolations();
            {
                onesol::realtime::AudioThreadScope scope;
                auto s = Settings();
                s.delayMs = 20.0f;
                for (auto trig : { Trigger::beat, Trigger::audio, Trigger::midi })
                {
                    s.trigger = trig;
                    for (int start = 0; start + 512 <= 20000; start += 512)
                        e.process (l.data(), r.data(), 512, sc.data() + start, sc.data() + start, notes, 2, s, playing());
                }
            }
            expectEquals ((int) onesol::realtime::violationCount(), 0);
        }
        else
        {
            logMessage ("(release build: allocation guard not available)");
        }
    }
};

static EngineTests engineTests;
