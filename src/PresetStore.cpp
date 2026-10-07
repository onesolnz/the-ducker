#include "PresetStore.h"

#include <onesol/PresetFile.h>

namespace ducker::presetStore
{
    juce::File defaultFile()
    {
        return onesol::presetFile::inUserData ("The Ducker");
    }

    std::vector<UserPreset> load (const juce::File& file)
    {
        std::vector<UserPreset> list;
        auto xml = onesol::presetFile::read (file, "PRESETS");
        if (xml == nullptr)
            return list;
        for (auto* e : xml->getChildWithTagNameIterator ("PRESET"))
        {
            UserPreset p;
            p.name = e->getStringAttribute ("name").trim();
            if (p.name.isEmpty())
                continue;
            p.duck = (float) e->getDoubleAttribute ("duck", 100.0);
            p.mix = (float) e->getDoubleAttribute ("mix", 100.0);
            p.smooth = (float) e->getDoubleAttribute ("smooth", 2.0);
            p.offset = (float) e->getDoubleAttribute ("offset", 0.0);
            p.delay = (float) e->getDoubleAttribute ("delay", 0.0);
            p.rate = juce::jlimit (0, 2, e->getIntAttribute ("rate", 0));
            for (auto* pt : e->getChildWithTagNameIterator ("POINT"))
                p.curve.push_back ({ pt->getIntAttribute ("tick"), pt->getDoubleAttribute ("value", 1.0), pt->getDoubleAttribute ("bend") });
            p.curve = tidyCurve (p.curve);
            list.push_back (std::move (p));
        }
        return list;
    }

    bool save (const juce::File& file, const std::vector<UserPreset>& presets)
    {
        juce::XmlElement xml ("PRESETS");
        for (auto& p : presets)
        {
            auto* e = xml.createNewChildElement ("PRESET");
            e->setAttribute ("name", p.name);
            e->setAttribute ("duck", (double) p.duck);
            e->setAttribute ("mix", (double) p.mix);
            e->setAttribute ("smooth", (double) p.smooth);
            e->setAttribute ("offset", (double) p.offset);
            e->setAttribute ("delay", (double) p.delay);
            e->setAttribute ("rate", p.rate);
            for (auto& pt : p.curve)
            {
                auto* c = e->createNewChildElement ("POINT");
                c->setAttribute ("tick", (int) pt.tick);
                c->setAttribute ("value", pt.value);
                c->setAttribute ("bend", pt.bend);
            }
        }
        return onesol::presetFile::write (file, xml);
    }
}
