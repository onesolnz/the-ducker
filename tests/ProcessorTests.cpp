#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

using namespace ducker;

namespace
{
    class FakeHead : public juce::AudioPlayHead
    {
    public:
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setPpqPosition (ppq);
            info.setBpm (120.0);
            info.setIsPlaying (true);
            return info;
        }
        double ppq = 0.0;
    };

    void set (DuckerProcessor& p, const char* id, float plain)
    {
        auto* param = p.params.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (plain));
    }

    float get (DuckerProcessor& p, const char* id)
    {
        auto* param = p.params.getParameter (id);
        return param->convertFrom0to1 (param->getValue());
    }
}

// The plug-in shell around the engine: buses, MIDI handling, state.
class ProcessorTests : public juce::UnitTest
{
public:
    ProcessorTests() : juce::UnitTest ("Processor", "TheDucker") {}

    void runTest() override
    {
        beginTest ("Buses: stereo or mono main, optional sidechain");
        {
            DuckerProcessor p;
            expectEquals (p.getBusCount (true), 2);
            expect (p.getBus (true, 1)->getName() == "Sidechain");
            juce::AudioProcessor::BusesLayout stereo, mono, mixed;
            stereo.inputBuses = { juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo() };
            stereo.outputBuses = { juce::AudioChannelSet::stereo() };
            mono.inputBuses = { juce::AudioChannelSet::mono(), juce::AudioChannelSet::disabled() };
            mono.outputBuses = { juce::AudioChannelSet::mono() };
            mixed.inputBuses = { juce::AudioChannelSet::mono(), juce::AudioChannelSet::disabled() };
            mixed.outputBuses = { juce::AudioChannelSet::stereo() };
            expect (p.checkBusesLayoutSupported (stereo));
            expect (p.checkBusesLayoutSupported (mono));
            expect (! p.checkBusesLayoutSupported (mixed));
        }

        beginTest ("MIDI: only note starts trigger; note-offs and velocity 0 do nothing");
        {
            const Curve ramp { { 0, 0.0, 0.0 }, { 960, 1.0, 0.0 } };
            auto play = [&] (juce::MidiBuffer midi) {
                DuckerProcessor p;
                FakeHead head;
                p.setPlayHead (&head);
                p.setRateAndBufferSizeDetails (44100.0, 512);
                p.prepareToPlay (44100.0, 512);
                p.setCurve (ramp);
                set (p, DuckerProcessor::triggerId, 2.0f);
                set (p, DuckerProcessor::smoothId, 0.0f);
                juce::AudioBuffer<float> buf (2, 512);
                for (int c = 0; c < 2; ++c)
                    juce::FloatVectorOperations::fill (buf.getWritePointer (c), 1.0f, 512);
                p.processBlock (buf, midi);
                expect (midi.isEmpty(), "no MIDI passes out");
                return std::vector<float> (buf.getReadPointer (0), buf.getReadPointer (0) + 512);
            };
            juce::MidiBuffer offs;
            offs.addEvent (juce::MidiMessage::noteOff (1, 60), 100);
            offs.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 0), 200);
            offs.addEvent (juce::MidiMessage::controllerEvent (1, 1, 64), 300);
            auto g = play (offs);
            expect (*std::min_element (g.begin(), g.end()) == 1.0f, "note-off, velocity 0 and a CC do nothing");

            juce::MidiBuffer on;
            on.addEvent (juce::MidiMessage::noteOn (5, 30, (juce::uint8) 1), 250);
            g = play (on);
            expect (g[249] == 1.0f);
            expectWithinAbsoluteError (g[250], 0.0f, 0.001f, "any channel, pitch and velocity");
        }

        beginTest ("State: everything round-trips; missing values fall back to defaults");
        {
            DuckerProcessor a;
            set (a, DuckerProcessor::duckId, 40.0f);
            set (a, DuckerProcessor::delayId, 25.0f);
            set (a, DuckerProcessor::rateId, 2.0f);
            set (a, DuckerProcessor::triggerId, 1.0f);
            a.setCurve (factoryShapes()[7].points);
            a.presetName = "Mine";
            a.presetEdited = true;
            a.windowScale = 1.25f;
            a.bob = false;
            juce::MemoryBlock mb;
            a.getStateInformation (mb);

            DuckerProcessor b;
            b.setStateInformation (mb.getData(), (int) mb.getSize());
            expectWithinAbsoluteError (get (b, DuckerProcessor::duckId), 40.0f, 0.01f);
            expectWithinAbsoluteError (get (b, DuckerProcessor::delayId), 25.0f, 0.01f);
            expectWithinAbsoluteError (get (b, DuckerProcessor::rateId), 2.0f, 0.01f);
            expectWithinAbsoluteError (get (b, DuckerProcessor::triggerId), 1.0f, 0.01f);
            expect (b.getCurve() == tidyCurve (factoryShapes()[7].points));
            expect (b.presetName == "Mine" && b.presetEdited && b.windowScale == 1.25f && ! b.bob);

            juce::XmlElement bare ("THEDUCKER");
            juce::MemoryBlock mb2;
            juce::AudioProcessor::copyXmlToBinary (bare, mb2);
            b.setStateInformation (mb2.getData(), (int) mb2.getSize());
            expectWithinAbsoluteError (get (b, DuckerProcessor::duckId), 100.0f, 0.01f);
            expectWithinAbsoluteError (get (b, DuckerProcessor::triggerId), 0.0f, 0.01f);
            expect (b.getCurve() == tidyCurve (factoryShapes().front().points));
            expect (b.bob && b.windowScale == 1.0f && ! b.presetEdited);
        }

        beginTest ("Curves are tidied: sorted, clamped, spanning the whole pass");
        {
            const auto c = tidyCurve ({ { 500, 2.0, 0.0 }, { -10, 0.2, 3.0 } });
            expectEquals ((int) c.size(), 3);
            expect (c.front().tick == 0 && c.front().bend == 1.0 && c[1].value == 1.0 && c.back().tick == kCycleTicks);
        }
    }
};

static ProcessorTests processorTests;
