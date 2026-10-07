#include "Widgets.h"

#include "Theme.h"

namespace ducker
{
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
}
