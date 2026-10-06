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

    // ---------- steel, grain, frames: the shared metal look of the buttons, pills, knobs and section frames ----------

    // One small tile of translucent black and white speckle with faint horizontal brushing, laid over metal as a fake texture.
    inline const juce::Image& grainTile()
    {
        static const juce::Image tile = []
        {
            juce::Image im (juce::Image::ARGB, 128, 128, true);
            juce::Random rnd (5);
            std::array<float, 128> row {};
            for (auto& r : row)
                r = rnd.nextFloat();
            juce::Image::BitmapData bd (im, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < 128; ++y)
                for (int x = 0; x < 128; ++x)
                {
                    const float n = rnd.nextFloat();
                    const float a = std::abs (n - 0.5f) * 0.5f + std::abs (row[(size_t) y] - 0.5f) * 0.25f;
                    bd.setPixelColour (x, y, (n > 0.5f ? juce::Colours::white : juce::Colours::black).withAlpha (a));
                }
            return im;
        }();
        return tile;
    }

    // Lays the grain over a shape (a clip is set by the caller or by the path).
    inline void grainFill (juce::Graphics& g, const juce::Path& shape, float opacity)
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (shape);
        g.setTiledImageFill (grainTile(), 0, 0, opacity);
        g.fillAll();
    }

    // A steel bevel, lit from the top left (light top and left edges, dark bottom and right). Pressed flips the light.
    inline juce::ColourGradient steelGradient (juce::Rectangle<float> r, bool pressed = false)
    {
        const juce::Colour hi (0xffd4d4d2), mid1 (0xff9a9a98), mid2 (0xff5c5c5b), lo (0xff2c2c2b);
        juce::ColourGradient cg (pressed ? lo : hi, r.getX(), r.getY(), pressed ? hi : lo, r.getRight(), r.getBottom(), false);
        cg.addColour (0.30, pressed ? mid2 : mid1);
        cg.addColour (0.55, pressed ? mid1 : mid2);
        return cg;
    }

    // A raised button: black keyline and drop shadow, a wide grained steel rim, a dark face (lit warm brass when `lit`).
    inline void drawRaisedButton (juce::Graphics& g, juce::Rectangle<float> r, float radius, float rim, bool lit, bool hover, bool pressed)
    {
        juce::Path outer, face;
        outer.addRoundedRectangle (r, radius);
        const auto inner = r.reduced (rim);
        const float innerRadius = juce::jmax (2.0f, radius - rim * 0.5f);
        face.addRoundedRectangle (inner, innerRadius);

        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f).expanded (1.0f), radius + 1.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (r.expanded (1.0f), radius + 1.0f);

        g.setGradientFill (steelGradient (r, pressed));
        g.fillPath (outer);
        grainFill (g, outer, 1.0f);

        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.fillRoundedRectangle (inner.expanded (1.0f), innerRadius + 1.0f);
        if (lit)
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a4720), inner.getCentreX(), inner.getY() + inner.getHeight() * 0.35f,
                                                     juce::Colour (0xff2b2210), inner.getX(), inner.getBottom(), true));
        else if (pressed)
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c1a17), 0.0f, inner.getY(), juce::Colour (0xff2a2723), 0.0f, inner.getBottom(), false));
        else
            g.setGradientFill (hover ? juce::ColourGradient (juce::Colour (0xff3a3631), 0.0f, inner.getY(), juce::Colour (0xff24211d), 0.0f, inner.getBottom(), false)
                                     : juce::ColourGradient (juce::Colour (0xff2e2b27), 0.0f, inner.getY(), juce::Colour (0xff1c1a17), 0.0f, inner.getBottom(), false));
        g.fillPath (face);
        grainFill (g, face, 0.3f);
        // the face is a little sunk into the rim: a shadow along its top
        {
            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (face);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (lit ? 0.3f : 0.5f), 0.0f, inner.getY(), juce::Colours::transparentBlack, 0.0f, inner.getY() + 5.0f, false));
            g.fillRect (inner);
            if (lit)
            {
                g.setColour (theme::brassHi.withAlpha (0.35f));
                g.drawRoundedRectangle (inner.reduced (1.0f), innerRadius, 2.0f);
            }
        }
    }

    // A section frame: a brass ring with black keylines round a slightly darkened panel, and a rivet in each corner.
    inline void drawSectionFrame (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r.expanded (4.5f), 11.0f, 1.0f);
        g.setColour (juce::Colour (0xff8c6b2c));
        g.drawRoundedRectangle (r.expanded (2.5f), 10.0f, 3.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r.expanded (0.5f), 8.5f, 1.0f);
        g.setColour (juce::Colour (0xfff0cf86).withAlpha (0.28f));
        g.drawRoundedRectangle (r.expanded (5.5f), 11.5f, 1.0f);
        {
            juce::Graphics::ScopedSaveState s (g);
            juce::Path p;
            p.addRoundedRectangle (r, 8.0f);
            g.reduceClipRegion (p);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.55f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 12.0f, false));
            g.fillRect (r);
        }
        for (auto c : { r.getTopLeft() + juce::Point<float> (9.0f, 9.0f), r.getTopRight() + juce::Point<float> (-9.0f, 9.0f),
                        r.getBottomLeft() + juce::Point<float> (9.0f, -9.0f), r.getBottomRight() + juce::Point<float> (-9.0f, -9.0f) })
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a826f), c.x - 1.0f, c.y - 1.0f, juce::Colour (0xff1a1814), c.x + 3.0f, c.y + 3.0f, true));
            g.fillEllipse (c.x - 3.5f, c.y - 3.5f, 7.0f, 7.0f);
        }
    }

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
