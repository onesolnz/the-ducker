#pragma once

#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Curve.h"

namespace ducker
{
    // A brass knob for one parameter, drawn in code until the user's knob art arrives (spec answer 2: one image the code rotates).
    // Drag up/down (Shift = fine), mouse wheel, double-click resets, arrow keys when focused. Big knobs show their name above in
    // large letters and need a longer drag for the full range.
    class Knob : public juce::Component
    {
    public:
        Knob (juce::RangedAudioParameter& param, juce::String name, int knobSize, std::function<juce::String (float)> format);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        bool keyPressed (const juce::KeyPress&) override;

        bool isBig() const { return knobSize > 50; }

    private:
        void setNormalised (float v, bool asGesture);

        juce::RangedAudioParameter& param;
        juce::ParameterAttachment attachment;
        juce::String name;
        int knobSize;
        std::function<juce::String (float)> format;
        float value = 0.0f;            // normalised 0 to 1
        float dragStart = 0.0f;
        bool dragging = false;
    };

    // A clickable button in one of the mock-up's styles. Pill: the rounded preset/save pills. Square: the rate and trigger
    // buttons. Title: the cream buttons in the title strip. `withArrow` adds the menu triangle after the text.
    class Pill : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        enum class Style { pill, square, title };

        Pill (juce::String text, Style style, bool withArrow = false);
        void setText (const juce::String& t);
        void setOn (bool on);
        int idealWidth() const;

        std::function<void()> onClick;
        std::function<void()> onRightClick;

        void paint (juce::Graphics&) override;
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override { repaint(); }
        void mouseUp (const juce::MouseEvent&) override;

    private:
        juce::String text;
        Style style;
        bool arrow, on = false;
    };

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
        void mouseUp (const juce::MouseEvent&) override;

    private:
        juce::Path line;
        bool on = false;
    };

    // The DUCK METER: IN and OUT bars (-48 to 0 dB) with falling peak marks.
    class Meter : public juce::Component
    {
    public:
        void setLevels (float inLevel, float outLevel, float inPeakMark, float outPeakMark);   // 0 to 1 of the scale
        void paint (juce::Graphics&) override;

        static constexpr int barWidth = 30, gap = 12;

    private:
        float in = 0.0f, out = 0.0f, inMark = 0.0f, outMark = 0.0f;
    };
}
