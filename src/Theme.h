#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The Ducker's own look (specs/27-the-ducker-vst.md, mockup-the-ducker.html): brass and yellow on dark metal.
namespace ducker::theme
{
    inline const juce::Colour metal      { 0xff23211e };
    inline const juce::Colour well       { 0xff121110 };
    inline const juce::Colour brass      { 0xffc9a35a };
    inline const juce::Colour brassHi    { 0xfff0cf86 };
    inline const juce::Colour brassLo    { 0xff6e5528 };
    inline const juce::Colour cream      { 0xffead9b4 };
    inline const juce::Colour dimText    { 0xffbfae8c };
    inline const juce::Colour offText    { 0xff9e9077 };
    inline const juce::Colour yellow     { 0xfff2c21b };
    inline const juce::Colour orange     { 0xffe57b12 };
    inline const juce::Colour curveLine  { 0xffe9c27a };

    constexpr int width = 1350, height = 600;     // the layout size; drawn at baseScale for "100 %"
    constexpr float baseScale = 0.75f;            // the user found 1350 x 600 too big: "100 %" is 1012 x 450 (2026-10-05)
    constexpr float minScale = baseScale, maxScale = baseScale * 2.0f;

    // Condensed labels (the mock-up used Oswald; Bahnschrift ships with Windows 10 and 11).
    inline juce::Font label (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions ("Bahnschrift", size, bold ? juce::Font::bold : juce::Font::plain)).withHorizontalScale (0.86f);
    }

    // Text with a 1 px black shadow under it, as all the panel labels have.
    inline void shadowText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just, juce::Colour colour)
    {
        g.setColour (juce::Colours::black);
        g.drawText (text, area.translated (0.0f, 1.0f), just, false);
        g.setColour (colour);
        g.drawText (text, area, just, false);
    }

    // A small down triangle after a pill's text (the menu sign).
    inline void dropArrow (juce::Graphics& g, float x, float cy, juce::Colour c)
    {
        juce::Path p;
        p.addTriangle (x, cy - 2.5f, x + 8.0f, cy - 2.5f, x + 4.0f, cy + 2.5f);
        g.setColour (c);
        g.fillPath (p);
    }
}
