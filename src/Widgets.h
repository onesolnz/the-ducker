#pragma once

#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>

#include <onesol/Widgets.h>

#include "Curve.h"

namespace ducker
{
    // The knob, the steel buttons and the meter are shared (Shared/include/onesol/Widgets.h).
    using onesol::Knob;
    using onesol::Meter;
    using onesol::Pill;

    // One of the 13 shape buttons: a mini picture of the curve.
    class ShapeButton : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        ShapeButton (const Shape& shape);
        void setOn (bool on);
        std::function<void()> onClick;

        void paint (juce::Graphics&) override;
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override { repaint(); }
        void mouseDown (const juce::MouseEvent&) override { repaint(); }
        void mouseUp (const juce::MouseEvent&) override;

    private:
        juce::Path line;
        bool on = false;
    };
}
