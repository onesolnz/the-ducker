#include "Widgets.h"

#include "Theme.h"

namespace ducker
{
    // ---------- Knob ----------

    Knob::Knob (juce::RangedAudioParameter& p, juce::String n, int size, std::function<juce::String (float)> f)
        : param (p),
          attachment (p, [this] (float plain) { value = param.convertTo0to1 (plain); repaint(); }),
          name (std::move (n)), knobSize (size), format (std::move (f))
    {
        setWantsKeyboardFocus (true);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        setTitle (name);
        attachment.sendInitialUpdate();
    }

    void Knob::setNormalised (float v, bool asGesture)
    {
        // snap through the parameter's own steps so the shown value is what the host stores
        const float plain = param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, v));
        if (asGesture)
            attachment.setValueAsPartOfGesture (plain);
        else
            attachment.setValueAsCompleteGesture (plain);
    }

    void Knob::mouseDown (const juce::MouseEvent&)
    {
        dragging = true;
        dragStart = value;
        attachment.beginGesture();
        grabKeyboardFocus();
    }

    void Knob::mouseDrag (const juce::MouseEvent& e)
    {
        const float pixels = e.mods.isShiftDown() ? 800.0f : (isBig() ? 260.0f : 180.0f);
        // the drag distance is in this component's (unscaled) pixels, so the feel is the same at every window size
        setNormalised (dragStart - (float) e.getDistanceFromDragStartY() / pixels, true);
    }

    void Knob::mouseUp (const juce::MouseEvent&)
    {
        if (dragging)
            attachment.endGesture();
        dragging = false;
    }

    void Knob::mouseDoubleClick (const juce::MouseEvent&)
    {
        setNormalised (param.getDefaultValue(), false);
    }

    void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
    {
        const float dir = w.deltaY > 0.0f ? 1.0f : (w.deltaY < 0.0f ? -1.0f : 0.0f);
        if (dir != 0.0f)
            setNormalised (value + dir / 50.0f, false);
    }

    bool Knob::keyPressed (const juce::KeyPress& k)
    {
        const float step = k.getModifiers().isShiftDown() ? 0.01f : 0.05f;
        const int code = k.getKeyCode();
        if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey) { setNormalised (value + step, false); return true; }
        if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey) { setNormalised (value - step, false); return true; }
        return false;
    }

    void Knob::paint (juce::Graphics& g)
    {
        const float s = (float) knobSize / 50.0f;                         // the drawing below is in a 50 x 50 box, scaled
        const float top = isBig() ? 44.0f : 0.0f;
        const float x0 = ((float) getWidth() - (float) knobSize) * 0.5f;
        const float cx = x0 + 25.0f * s, cy = top + 25.0f * s;
        const float f = value;

        if (isBig())
        {
            // the name above in big yellow letters with a black outline
            auto font = theme::label (40.0f, true).withExtraKerningFactor (0.04f);
            juce::GlyphArrangement ga;
            ga.addLineOfText (font, name, 0.0f, 0.0f);
            juce::Path text;
            ga.createPath (text);
            const auto b = text.getBounds();
            text.applyTransform (juce::AffineTransform::translation ((float) getWidth() * 0.5f - b.getCentreX(), 36.0f - b.getBottom()));
            g.setColour (juce::Colours::black);
            g.fillPath (text, juce::AffineTransform::translation (3.0f, 4.0f));
            g.strokePath (text, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
            g.setColour (theme::cream);
            g.fillPath (text);
        }

        // gear-edged steel ring: square teeth (flat tops, straight radial sides, flat gaps), grained, with a raised edge lit top left
        juce::Path ring;
        const int teeth = 14;
        const float pitch = juce::MathConstants<float>::twoPi / (float) teeth, rIn = 21.2f * s, rOut = 24.0f * s;
        for (int i = 0; i < teeth; ++i)
            for (auto [u, r] : { std::pair<float, float> { 0.0f, rIn }, { 0.25f, rIn }, { 0.25f, rOut }, { 0.75f, rOut }, { 0.75f, rIn } })
            {
                const float a = ((float) i + u) * pitch;
                const juce::Point<float> p (cx + std::cos (a) * r, cy + std::sin (a) * r);
                i == 0 && u == 0.0f ? ring.startNewSubPath (p) : ring.lineTo (p);
            }
        ring.closeSubPath();
        const juce::Rectangle<float> box (x0 + 5.0f * s, top + 3.0f * s, 40.0f * s, 44.0f * s);
        g.setGradientFill (theme::steelGradient (box));
        g.fillPath (ring);
        theme::grainFill (g, ring, 0.4f);
        {
            // faint brushed rings across the teeth (seeded, so the same every paint)
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (ring);
            juce::Random rnd (11);
            for (float r = 20.5f; r < 24.4f; r += 0.22f)
            {
                const bool light = rnd.nextFloat() < 0.5f;
                const float a = rnd.nextFloat() * (light ? 0.16f : 0.2f), w = 0.2f + rnd.nextFloat() * 0.15f;
                g.setColour ((light ? juce::Colour (0xfffff4d6) : juce::Colours::black).withAlpha (a));
                g.drawEllipse (cx - r * s, cy - r * s, 2.0f * r * s, 2.0f * r * s, w * s);
            }
        }
        g.setColour (juce::Colours::black);
        g.strokePath (ring, juce::PathStrokeType (1.2f * s));
        {
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (ring);
            juce::ColourGradient bevel (juce::Colours::white.withAlpha (0.65f), box.getX(), box.getY(), juce::Colours::black.withAlpha (0.6f), box.getRight(), box.getBottom(), false);
            bevel.addColour (0.45, juce::Colours::white.withAlpha (0.0f));
            bevel.addColour (0.55, juce::Colours::black.withAlpha (0.0f));
            g.setGradientFill (bevel);
            g.strokePath (ring, juce::PathStrokeType (2.6f * s));
        }

        // value arc on a dark track
        const float a0 = juce::MathConstants<float>::pi * 0.75f, a1 = juce::MathConstants<float>::pi * 2.25f;
        g.setColour (juce::Colour (0xff1a1612));
        g.fillEllipse (cx - 18.0f * s, cy - 18.0f * s, 36.0f * s, 36.0f * s);
        if (f > 0.0f)
        {
            juce::Path arc;
            arc.addCentredArc (cx, cy, 18.0f * s, 18.0f * s, 0.0f, a0 + juce::MathConstants<float>::halfPi, a0 + (a1 - a0) * f + juce::MathConstants<float>::halfPi, true);
            g.setColour (theme::yellow.withAlpha (0.85f));
            g.strokePath (arc, juce::PathStrokeType (2.2f * s));
        }

        // dark cap
        juce::ColourGradient cap (juce::Colour (0xff4a463f), cx - 4.0f * s, cy - 5.0f * s, juce::Colour (0xff151311), cx - 4.0f * s + 15.0f * s, cy - 5.0f * s + 15.0f * s, true);
        g.setGradientFill (cap);
        g.fillEllipse (cx - 14.5f * s, cy - 14.5f * s, 29.0f * s, 29.0f * s);
        {
            juce::Path capPath;
            capPath.addEllipse (cx - 14.5f * s, cy - 14.5f * s, 29.0f * s, 29.0f * s);
            theme::grainFill (g, capPath, 0.3f);
        }
        // a raised steel rim round the cap, lit top left
        {
            juce::ColourGradient rimGrad (juce::Colour (0xffe4e4e2), cx - 11.0f * s, cy - 11.0f * s, juce::Colour (0xff262625), cx + 11.0f * s, cy + 11.0f * s, false);
            rimGrad.addColour (0.5, juce::Colour (0xff7a7a79));
            g.setGradientFill (rimGrad);
            g.drawEllipse (cx - 14.6f * s, cy - 14.6f * s, 29.2f * s, 29.2f * s, 1.8f * s);
            g.setColour (juce::Colours::black);
            g.drawEllipse (cx - 13.5f * s, cy - 13.5f * s, 27.0f * s, 27.0f * s, 0.8f * s);
        }

        // pointer
        const float a = a0 + (a1 - a0) * f;
        g.setColour (juce::Colour (0xfff5e6c3));
        g.drawLine (cx + std::cos (a) * 4.0f * s, cy + std::sin (a) * 4.0f * s, cx + std::cos (a) * 13.0f * s, cy + std::sin (a) * 13.0f * s, 2.6f * s);

        // value and name
        const float textTop = top + (float) knobSize + 2.0f;
        g.setFont (theme::label (isBig() ? 22.0f : 15.0f, true));
        theme::shadowText (g, format (param.convertFrom0to1 (value)), { 0.0f, textTop, (float) getWidth(), isBig() ? 26.0f : 18.0f }, juce::Justification::centred, theme::cream);
        if (! isBig())
        {
            g.setFont (theme::label (14.0f));
            theme::shadowText (g, name, { 0.0f, textTop + 18.0f, (float) getWidth(), 16.0f }, juce::Justification::centred, theme::dimText);
        }
    }

    // ---------- Pill ----------

    Pill::Pill (juce::String t, Style s, bool withArrow) : text (std::move (t)), style (s), arrow (withArrow)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTitle (text);
    }

    void Pill::setText (const juce::String& t)
    {
        if (t != text)
        {
            text = t;
            setTitle (text);
            repaint();
        }
    }

    void Pill::setOn (bool o)
    {
        if (o != on)
        {
            on = o;
            repaint();
        }
    }

    int Pill::idealWidth() const
    {
        const auto font = theme::label (style == Style::title ? 15.0f : 16.0f, true);
        return juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, text)) + (style == Style::pill ? 30 : 20) + (arrow ? 14 : 0);
    }

    void Pill::mouseUp (const juce::MouseEvent& e)
    {
        if (! getLocalBounds().contains (e.getPosition()))
            return;
        if (e.mods.isPopupMenu())
        {
            if (onRightClick)
                onRightClick();
        }
        else if (onClick)
            onClick();
    }

    void Pill::paint (juce::Graphics& g)
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        const bool hover = isMouseOver();
        juce::Colour textColour = theme::cream;

        if (style == Style::title)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3dfa6), 0.0f, r.getY(), juce::Colour (0xffd5b46a), 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (juce::Colour (0xff5c5c5b));
            g.drawRoundedRectangle (r, 6.0f, 1.5f);
            textColour = juce::Colour (0xff1a1408);
        }
        else
        {
            // raised steel rim, 4 px wide; on and hover show on the face, never on the rim
            const float radius = style == Style::pill ? (r.getHeight() - 2.0f) * 0.5f : 5.0f;
            theme::drawRaisedButton (g, r.reduced (1.0f), radius, 4.0f, on, hover, hover && isMouseButtonDown());
            textColour = on ? theme::brassHi : (style == Style::square ? theme::offText : theme::cream);
        }

        const auto font = theme::label (style == Style::title ? 15.0f : 16.0f, true);
        g.setFont (font);
        const float textW = juce::GlyphArrangement::getStringWidth (font, text);
        const float total = textW + (arrow ? 14.0f : 0.0f);
        const float x = r.getCentreX() - total * 0.5f;
        g.setColour (textColour);
        g.drawText (text, juce::Rectangle<float> (x, r.getY(), textW + 2.0f, r.getHeight()), juce::Justification::centredLeft, false);
        if (arrow)
            theme::dropArrow (g, x + textW + 6.0f, r.getCentreY(), textColour);
    }

    // ---------- ShapeButton ----------

    ShapeButton::ShapeButton (const Shape& shape)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTitle (juce::String ("Shape: ") + shape.name);
        setTooltip (shape.name);
        for (int x = 0; x <= 36; ++x)
        {
            const float y = 3.0f + (1.0f - curveAt (shape.points, x / 36.0)) * 20.0f;
            x == 0 ? line.startNewSubPath ((float) x, y) : line.lineTo ((float) x, y);
        }
    }

    void ShapeButton::setOn (bool o)
    {
        if (o != on)
        {
            on = o;
            repaint();
        }
    }

    void ShapeButton::mouseUp (const juce::MouseEvent& e)
    {
        if (getLocalBounds().contains (e.getPosition()) && ! e.mods.isPopupMenu() && onClick)
            onClick();
    }

    void ShapeButton::paint (juce::Graphics& g)
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        const bool hover = isMouseOver();
        theme::drawRaisedButton (g, r.reduced (1.0f), 5.0f, 4.0f, on, hover, hover && isMouseButtonDown());
        g.setColour (juce::Colour (0xffe7b04a));
        g.strokePath (line, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::translation (r.getCentreX() - 18.0f, r.getCentreY() - 13.0f));
    }

    // ---------- Meter ----------

    void Meter::setLevels (float inLevel, float outLevel, float inPeakMark, float outPeakMark)
    {
        if (inLevel != in || outLevel != out || inPeakMark != inMark || outPeakMark != outMark)
        {
            in = inLevel; out = outLevel; inMark = inPeakMark; outMark = outPeakMark;
            repaint();
        }
    }

    void Meter::paint (juce::Graphics& g)
    {
        const float h = (float) getHeight();
        auto bar = [&] (float x, float level, float mark)
        {
            juce::Rectangle<float> r (x, 0.0f, (float) barWidth, h);
            g.setColour (juce::Colour (0xff3a3226));
            g.drawRoundedRectangle (r.expanded (0.5f), 3.0f, 1.0f);
            g.setColour (juce::Colour (0xff0b0a09));
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (juce::Colours::black);
            g.drawRoundedRectangle (r.reduced (1.0f), 3.0f, 2.0f);

            const float inner = h - 6.0f;
            const float fillH = inner * juce::jlimit (0.0f, 1.0f, level);
            if (fillH > 0.5f)
            {
                juce::Rectangle<float> f (x + 3.0f, h - 3.0f - fillH, (float) barWidth - 6.0f, fillH);
                juce::ColourGradient grad (theme::orange, 0.0f, h - 3.0f, juce::Colour (0xffffe27a), 0.0f, 3.0f, false);
                grad.addColour (0.7, theme::yellow);
                g.setGradientFill (grad);
                g.fillRoundedRectangle (f, 2.0f);
            }
            const float markY = h - 3.0f - inner * juce::jlimit (0.0f, 1.0f, mark) - 3.0f;
            g.setColour (theme::brassHi);
            g.fillRect (x + 3.0f, markY, (float) barWidth - 6.0f, 3.0f);
        };
        bar (0.0f, in, inMark);
        bar ((float) (barWidth + gap), out, outMark);
    }
}
