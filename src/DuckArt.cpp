#include "DuckArt.h"

#include "BinaryData.h"

namespace ducker
{
    DuckArt::DuckArt()
    {
        // the sheet is decoded once per process (ImageCache) and each frame is a view into it
        const auto sheet = juce::ImageCache::getFromMemory (BinaryData::head_spritesheet_png, BinaryData::head_spritesheet_pngSize);
        const auto body = juce::ImageCache::getFromMemory (BinaryData::body_png, BinaryData::body_pngSize);
        if (sheet.isValid() && body.isValid())
        {
            for (int i = 0; i < kHeadFrames; ++i)
                headFrames.push_back (sheet.getClippedImage ({ (i % kHeadColumns) * kHeadCell, (i / kHeadColumns) * kHeadCell, kHeadCell, kHeadCell }));
            bodyView.image = body;
            bodyView.headFrame = headFrames[(size_t) kRestFrame];
            bodyView.layered = true;
        }
        else
            bodyView.image = juce::ImageCache::getFromMemory (BinaryData::duckerbody_png, BinaryData::duckerbody_pngSize);

        bodyView.setInterceptsMouseClicks (false, false);
        bodyView.setPaintingIsUnclipped (true);     // the bob moves the body a little outside its box
        bodyView.setTitle ("The Ducker duck");
        startMs = juce::Time::getMillisecondCounterHiRes();
    }

    int DuckArt::nodFrame (double cycle, float duckKnob)
    {
        // 28 steps make one forwards-and-back pass over the 15 frames; the last frame (the head down) is on the beat
        constexpr int period = 2 * kHeadFrames - 2;
        const double c = cycle - std::floor (cycle);
        const int p = ((int) std::floor (c * period) + kDownFrame) % period;
        const int moving = p < kHeadFrames ? p : period - p;
        const float k = juce::jlimit (0.0f, 1.0f, duckKnob * 0.01f);
        return juce::jlimit (0, kHeadFrames - 1, juce::roundToInt ((float) kRestFrame + (float) (moving - kRestFrame) * k));
    }

    void DuckArt::setDuck (float duck, bool bob, float duckKnob, bool hostPlaying, double ppq)
    {
        duck = juce::jlimit (0.0f, 1.0f, duck);

        // the nod follows the host's beat while it plays, and runs free at kFreeFps when it is stopped
        if (! headFrames.empty())
        {
            const double now = juce::Time::getMillisecondCounterHiRes();
            const double cycle = hostPlaying ? ppq / kBeatsPerNod : (now - startMs) * 0.001 * kFreeFps / (2 * kHeadFrames - 2);
            const int frame = nodFrame (cycle, duckKnob);
            if (frame != shownFrame)
            {
                shownFrame = frame;
                bodyView.headFrame = headFrames[(size_t) frame];
                bodyView.repaint();
            }
        }

        if (std::abs (duck - shownDuck) < 0.002f && bob == shownBob)
            return;
        shownDuck = duck;
        shownBob = bob;

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
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        const auto placement = juce::RectanglePlacement (juce::RectanglePlacement::xMid | juce::RectanglePlacement::yBottom);
        if (layered)
        {
            // fitted like the old sprite cell, so the feet sit on the bottom edge and the bob pivots around them
            const auto fit = placement.getTransformToFit (juce::Rectangle<float> (0.0f, 0.0f, kCellW, kCellH), getLocalBounds().toFloat());
            const auto base = juce::AffineTransform::scale (kBodyScale).translated (kBodyX, kBodyY).followedBy (fit).followedBy (motion);
            g.drawImageTransformed (image, base);
            if (headFrame.isValid())
                g.drawImageTransformed (headFrame, juce::AffineTransform::translation (kHeadX, kHeadY).followedBy (base));
            return;
        }
        // fitted to the box with the feet on its bottom edge, so the bob pivots around them
        const auto fit = placement.getTransformToFit (image.getBounds().toFloat(), getLocalBounds().toFloat());
        g.drawImageTransformed (image, fit.followedBy (motion));
    }
}
