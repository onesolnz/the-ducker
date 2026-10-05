#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DuckerEngine.h"

namespace ducker
{
    // The Ducker plug-in: main stereo (or mono) in/out, an optional "Sidechain" input bus (Audio trigger) and MIDI in (MIDI trigger).
    // Parameters are host-automatable; the curve, the preset name, the window size and the Bob switch live in the plug-in's state.
    class DuckerProcessor : public juce::AudioProcessor
    {
    public:
        static constexpr const char* duckId = "duck", *mixId = "mix", *smoothId = "smooth", *offsetId = "offset",
                                    *delayId = "delay", *rateId = "rate", *triggerId = "trigger";

        DuckerProcessor();

        const juce::String getName() const override { return "The Ducker"; }
        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void releaseResources() override {}
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        using AudioProcessor::processBlock;
        bool isBusesLayoutSupported (const BusesLayout&) const override;

        bool acceptsMidi() const override  { return true; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock&) override;
        void setStateInformation (const void*, int) override;

        // The curve (message thread).
        void setCurve (Curve c);
        const Curve& getCurve() const { return curve; }

        // Window-side state kept with the plug-in (message thread).
        juce::String presetName { "Kick start" };
        bool presetEdited = false;
        float windowScale = 0.75f;          // drawing scale of the 1350 x 600 layout (0.75 = "100 %", 1.5 = "200 %")
        bool bob = true;

        juce::AudioProcessorValueTreeState params;
        DuckerEngine engine;

        // What the host's transport did in the last block, for the status line (any thread).
        std::atomic<bool> hostPlaying { false };
        std::atomic<double> hostPpq { 0.0 }, hostBpm { 120.0 };

        static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    private:
        Settings readSettings() const noexcept;

        Curve curve;
        std::array<int, 512> noteOns {};
        std::atomic<float>* duck = nullptr, *mix = nullptr, *smooth = nullptr, *offset = nullptr, *delay = nullptr, *rate = nullptr, *trigger = nullptr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuckerProcessor)
    };
}
