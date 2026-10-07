#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <onesol/Theme.h>

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

    // The steel look itself is shared (Shared/include/onesol/Theme.h); this is only what makes it The Ducker's.
    using onesol::theme::drawRaisedButton;
    using onesol::theme::drawSectionFrame;
    using onesol::theme::dropArrow;
    using onesol::theme::grainFill;
    using onesol::theme::label;
    using onesol::theme::shadowText;
    using onesol::theme::steelGradient;

    inline onesol::Palette palette()
    {
        onesol::Palette p;
        p.cream = cream;
        p.dimText = dimText;
        p.offText = offText;
        p.accent = yellow;
        p.accentHi = brassHi;
        p.warn = orange;
        p.meterTop = juce::Colour (0xffffe27a);
        p.meterBorder = juce::Colour (0xff3a3226);
        p.litCentre = juce::Colour (0xff5a4720);
        p.litEdge = juce::Colour (0xff2b2210);
        p.frameRing = juce::Colour (0xff8c6b2c);
        p.frameHi = juce::Colour (0xfff0cf86);
        p.pointer = juce::Colour (0xfff5e6c3);
        p.titleTop = juce::Colour (0xfff3dfa6);
        p.titleBottom = juce::Colour (0xffd5b46a);
        p.titleText = juce::Colour (0xff1a1408);
        return p;
    }
}
