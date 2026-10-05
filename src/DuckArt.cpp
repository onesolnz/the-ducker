#include "DuckArt.h"

#include "BinaryData.h"

namespace ducker
{
    DuckArt::DuckArt()
    {
        headView.image = juce::ImageCache::getFromMemory (BinaryData::duckerhead_png, BinaryData::duckerhead_pngSize);
        bodyView.image = juce::ImageCache::getFromMemory (BinaryData::duckerbody_png, BinaryData::duckerbody_pngSize);
        for (auto* v : { &headView, &bodyView })
        {
            v->setInterceptsMouseClicks (false, false);
            v->setPaintingIsUnclipped (true);       // the bob moves the body a little outside its box
        }
        headView.setTitle ("Duck head");
        bodyView.setTitle ("The Ducker duck");
    }

    void DuckArt::setDuck (float duck, bool bob)
    {
        duck = juce::jlimit (0.0f, 1.0f, duck);
        if (std::abs (duck - shownDuck) < 0.002f && bob == shownBob)
            return;
        shownDuck = duck;
        shownBob = bob;

        // head: drops a little, tips forward (a squash stands in for the mock-up's 3D tilt) and turns, around its neck
        {
            const float w = (float) headView.getWidth(), h = (float) headView.getHeight();
            const float px = w * 0.5f, py = h * 0.88f;
            const float tilt = std::cos (juce::degreesToRadians (duck * 24.0f));
            headView.motion = juce::AffineTransform::translation (-px, -py)
                                  .scaled (1.0f, tilt)
                                  .rotated (juce::degreesToRadians (-duck * 6.0f))
                                  .translated (px, py + duck * 7.0f);
            headView.repaint();
        }
        // body: sinks and squashes a touch, around its feet
        {
            const float w = (float) bodyView.getWidth(), h = (float) bodyView.getHeight();
            bodyView.motion = bob ? juce::AffineTransform::translation (-w * 0.5f, -h)
                                        .scaled (1.0f, 1.0f - duck * 0.04f)
                                        .translated (w * 0.5f, h + duck * 10.0f)
                                  : juce::AffineTransform();
            bodyView.repaint();
        }
    }

    void DuckArt::View::paint (juce::Graphics& g)
    {
        if (! image.isValid())
            return;
        const auto fit = juce::RectanglePlacement (juce::RectanglePlacement::centred)
                             .getTransformToFit (image.getBounds().toFloat(), getLocalBounds().toFloat());
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImageTransformed (image, fit.followedBy (motion));
    }
}
