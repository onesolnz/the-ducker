#include "PluginProcessor.h"

#include "RealtimeGuard.h"

namespace ducker
{
    juce::AudioProcessorValueTreeState::ParameterLayout DuckerProcessor::createLayout()
    {
        using Float = juce::AudioParameterFloat;
        using Attr = juce::AudioParameterFloatAttributes;
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add (std::make_unique<Float> (juce::ParameterID { duckId, 1 }, "Duck", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, Attr().withLabel ("%")));
        layout.add (std::make_unique<Float> (juce::ParameterID { mixId, 1 }, "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, Attr().withLabel ("%")));
        layout.add (std::make_unique<Float> (juce::ParameterID { smoothId, 1 }, "Smooth", juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 2.0f, Attr().withLabel ("ms")));
        layout.add (std::make_unique<Float> (juce::ParameterID { offsetId, 1 }, "Offset", juce::NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f, Attr().withLabel ("%")));
        layout.add (std::make_unique<Float> (juce::ParameterID { delayId, 1 }, "Delay", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f, Attr().withLabel ("ms")));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { rateId, 1 }, "Rate", juce::StringArray { "1/4", "1/8", "1/16" }, 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { triggerId, 1 }, "Trigger", juce::StringArray { "Beat", "Audio", "MIDI" }, 0));
        return layout;
    }

    DuckerProcessor::DuckerProcessor()
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                           .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
          params (*this, nullptr, "PARAMS", createLayout())
    {
        duck = params.getRawParameterValue (duckId);
        mix = params.getRawParameterValue (mixId);
        smooth = params.getRawParameterValue (smoothId);
        offset = params.getRawParameterValue (offsetId);
        delay = params.getRawParameterValue (delayId);
        rate = params.getRawParameterValue (rateId);
        trigger = params.getRawParameterValue (triggerId);
        setCurve (factoryShapes().front().points);
    }

    bool DuckerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
    {
        const auto& in = layouts.getMainInputChannelSet();
        if (in != layouts.getMainOutputChannelSet() || (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()))
            return false;
        if (layouts.inputBuses.size() > 1)
        {
            const auto& sc = layouts.inputBuses[1];
            return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
        }
        return true;
    }

    void DuckerProcessor::prepareToPlay (double sampleRate, int)
    {
        engine.prepare (sampleRate);
    }

    Settings DuckerProcessor::readSettings() const noexcept
    {
        Settings s;
        s.duck = duck->load() * 0.01f;
        s.mix = mix->load() * 0.01f;
        s.smoothMs = smooth->load();
        s.offset = offset->load() * 0.01f;
        s.delayMs = delay->load();
        s.rate = juce::jlimit (0, 2, (int) std::lround (rate->load()));
        s.trigger = (Trigger) juce::jlimit (0, 2, (int) std::lround (trigger->load()));
        return s;
    }

    void DuckerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        realtime::AudioThreadScope audioThread;
        juce::ScopedNoDenormals noDenormals;

        const int n = buffer.getNumSamples();
        auto main = getBusBuffer (buffer, true, 0);
        if (n <= 0 || main.getNumChannels() == 0)
            return;

        const float* scLeft = nullptr, *scRight = nullptr;
        if (auto* scBus = getBus (true, 1); scBus != nullptr && scBus->isEnabled())
        {
            auto sc = getBusBuffer (buffer, true, 1);
            if (sc.getNumChannels() > 0)
            {
                scLeft = sc.getReadPointer (0);
                scRight = sc.getNumChannels() > 1 ? sc.getReadPointer (1) : nullptr;
            }
        }

        // note starts only: note-offs, velocity 0, length, pitch and CCs do nothing
        int numNotes = 0;
        for (const auto meta : midi)
            if (meta.getMessage().isNoteOn() && numNotes < (int) noteOns.size())
                noteOns[(size_t) numNotes++] = juce::jlimit (0, n - 1, meta.samplePosition);
        midi.clear();

        Transport t;
        if (auto* head = getPlayHead())
            if (auto pos = head->getPosition(); pos.hasValue())
            {
                t.playing = pos->getIsPlaying();
                if (auto bpm = pos->getBpm(); bpm.hasValue() && *bpm > 0.0)
                    t.bpm = *bpm;
                if (auto ppq = pos->getPpqPosition(); ppq.hasValue())
                {
                    t.ppq = *ppq;
                    t.hasPosition = true;
                }
            }
        hostPlaying.store (t.playing, std::memory_order_relaxed);
        hostPpq.store (t.ppq, std::memory_order_relaxed);
        hostBpm.store (t.bpm, std::memory_order_relaxed);

        float* left = main.getWritePointer (0);
        float* right = main.getNumChannels() > 1 ? main.getWritePointer (1) : left;
        engine.process (left, right, n, scLeft, scRight, noteOns.data(), numNotes, readSettings(), t);
    }

    void DuckerProcessor::setCurve (Curve c)
    {
        curve = tidyCurve (std::move (c));
        engine.setCurve (curve);
    }

    juce::AudioProcessorEditor* DuckerProcessor::createEditor()
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }

    void DuckerProcessor::getStateInformation (juce::MemoryBlock& destData)
    {
        juce::XmlElement xml ("THEDUCKER");
        xml.setAttribute ("version", 1);
        xml.setAttribute ("preset", presetName);
        xml.setAttribute ("edited", presetEdited);
        xml.setAttribute ("scale", (double) windowScale);
        xml.setAttribute ("bob", bob);
        if (auto p = params.copyState().createXml())
            xml.addChildElement (p.release());
        auto* c = xml.createNewChildElement ("CURVE");
        for (auto& p : curve)
        {
            auto* e = c->createNewChildElement ("POINT");
            e->setAttribute ("tick", (int) p.tick);
            e->setAttribute ("value", p.value);
            e->setAttribute ("bend", p.bend);
        }
        copyXmlToBinary (xml, destData);
    }

    void DuckerProcessor::setStateInformation (const void* data, int sizeInBytes)
    {
        auto xml = getXmlFromBinary (data, sizeInBytes);
        if (xml == nullptr || ! xml->hasTagName ("THEDUCKER"))
            return;

        presetName = xml->getStringAttribute ("preset", "Kick start");
        presetEdited = xml->getBoolAttribute ("edited", false);
        windowScale = juce::jlimit (0.75f, 1.5f, (float) xml->getDoubleAttribute ("scale", 1.0));
        bob = xml->getBoolAttribute ("bob", true);

        // every parameter the state does not mention goes back to its default
        for (auto* p : getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
                ranged->setValueNotifyingHost (ranged->getDefaultValue());
        if (auto* p = xml->getChildByName (params.state.getType()))
            for (auto* e : p->getChildWithTagNameIterator ("PARAM"))
                if (auto* param = params.getParameter (e->getStringAttribute ("id")))
                    param->setValueNotifyingHost (param->convertTo0to1 ((float) e->getDoubleAttribute ("value", param->convertFrom0to1 (param->getDefaultValue()))));

        Curve points;
        if (auto* c = xml->getChildByName ("CURVE"))
            for (auto* e : c->getChildWithTagNameIterator ("POINT"))
                points.push_back ({ e->getIntAttribute ("tick"), e->getDoubleAttribute ("value", 1.0), e->getDoubleAttribute ("bend") });
        setCurve (points.empty() ? factoryShapes().front().points : points);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ducker::DuckerProcessor();
}
