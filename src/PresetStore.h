#pragma once

#include <juce_core/juce_core.h>

#include "Curve.h"

namespace ducker
{
    // A saved preset: the curve and the knobs (not the Trigger, which depends on how the track is routed).
    struct UserPreset
    {
        juce::String name;
        Curve curve;
        float duck = 100.0f, mix = 100.0f, smooth = 2.0f, offset = 0.0f, delay = 0.0f;
        int rate = 0;
    };

    // The user's presets, shared by every instance in every DAW: %APPDATA%\The Ducker\presets.xml.
    namespace presetStore
    {
        juce::File defaultFile();
        std::vector<UserPreset> load (const juce::File& file);
        bool save (const juce::File& file, const std::vector<UserPreset>& presets);
    }
}
