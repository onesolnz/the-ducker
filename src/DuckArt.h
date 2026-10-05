#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ducker
{
    // Everything that animates the duck art, in one place (spec answer 6): the head nods and the full-body duck bobs by how
    // hard the sound is being ducked right now. A frame-by-frame animation can later replace what these two views draw
    // without touching the rest of the window: the only input is setDuck().
    //
    // SPRITE TEST (branch sprite-test, 2026-10-06): the full-body duck plays the user's first sprite sheet as an idle loop
    // (71 frames of 244 x 454, 9 per row, 24 frames a second), with the coded bob on top.
    class DuckArt
    {
    public:
        DuckArt();

        juce::Component& head() { return headView; }
        juce::Component& body() { return bodyView; }

        // duck: 0 = full volume, 1 = silent. bob: the Bob switch under the body. Called on the window's timer.
        void setDuck (float duck, bool bob);

        static constexpr int kSpriteFrames = 71, kSpriteColumns = 9, kSpriteW = 244, kSpriteH = 454;
        static constexpr double kSpriteFps = 24.0;

    private:
        // An image fitted into the view, drawn with a transform around a pivot (a fraction of the view).
        struct View : juce::Component
        {
            juce::Image image;
            juce::AffineTransform motion;
            void paint (juce::Graphics&) override;
        };

        View headView, bodyView;
        std::vector<juce::Image> spriteFrames;              // views into the sheet (no copies)
        int shownFrame = -1;
        double startMs = 0.0;
        float shownDuck = -1.0f;
        bool shownBob = true;
    };
}
