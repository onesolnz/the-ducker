#include "DuckArt.h"

#include "BinaryData.h"

namespace ducker
{
    DuckArt::DuckArt()
    {
        headView.image = juce::ImageCache::getFromMemory (BinaryData::duckerhead_png, BinaryData::duckerhead_pngSize);

        // the sheet is decoded once per process (ImageCache) and each frame is a view into it
        const auto sheet = juce::ImageCache::getFromMemory (BinaryData::duck_loops_png, BinaryData::duck_loops_pngSize);
        if (sheet.isValid())
            for (int i = 0; i < kSpriteFrames; ++i)
                spriteFrames.push_back (sheet.getClippedImage ({ (i % kSpriteColumns) * kSpriteW, (i / kSpriteColumns) * kSpriteH, kSpriteW, kSpriteH }));
        bodyView.image = spriteFrames.empty() ? juce::ImageCache::getFromMemory (BinaryData::duckerbody_png, BinaryData::duckerbody_pngSize)
                                              : spriteFrames.front();

        for (auto* v : { &headView, &bodyView })
        {
            v->setInterceptsMouseClicks (false, false);
            v->setPaintingIsUnclipped (true);       // the bob moves the body a little outside its box
        }
        headView.setTitle ("Duck head");
        bodyView.setTitle ("The Ducker duck");
        startMs = juce::Time::getMillisecondCounterHiRes();
    }

    int DuckArt::loopFor (float duckKnob)
    {
        return duckKnob < 100.0f / 3.0f ? 0 : (duckKnob < 200.0f / 3.0f ? 1 : 2);
    }

    int DuckArt::frameAt (int loop, long long tick)
    {
        const auto& l = kLoops[juce::jlimit (0, 2, loop)];
        if (! l.pingPong)
            return l.first + (int) (tick % l.count);
        // forwards then back, without showing either end twice
        const int period = 2 * l.count - 2;
        const int p = (int) (tick % period);
        return l.first + (p < l.count ? p : period - p);
    }

    void DuckArt::setDuck (float duck, bool bob, float duckKnob)
    {
        duck = juce::jlimit (0.0f, 1.0f, duck);

        // the loop runs on its own clock; the Duck knob picks which one, and a new pick shows straight away
        if (! spriteFrames.empty())
        {
            const double seconds = (juce::Time::getMillisecondCounterHiRes() - startMs) * 0.001;
            const int frame = frameAt (loopFor (duckKnob), (long long) (seconds * kSpriteFps));
            if (frame != shownFrame)
            {
                shownFrame = frame;
                bodyView.image = spriteFrames[(size_t) frame];
                bodyView.repaint();
            }
        }

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
        // fitted to the box with the feet on its bottom edge, so the bob pivots around them
        const auto fit = juce::RectanglePlacement (juce::RectanglePlacement::xMid | juce::RectanglePlacement::yBottom)
                             .getTransformToFit (image.getBounds().toFloat(), getLocalBounds().toFloat());
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImageTransformed (image, fit.followedBy (motion));
    }
}
