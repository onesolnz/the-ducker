#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ducker
{
    // Everything that animates the duck art, in one place (spec answer 6): the head nods and the full-body duck bobs by how
    // hard the sound is being ducked right now. A frame-by-frame animation can later replace what these two views draw
    // without touching the rest of the window: the only input is setDuck().
    //
    // SPRITE TEST (branch sprite-test, 2026-10-06): the full-body duck plays one of three loops from assets/duck_loops.png
    // (80 frames of 375 x 463, 9 per row, feet on y = 458, 12 frames a second), picked by the Duck knob, with the coded
    // bob on top. The sheet is cut from the user's tier1/2/3 sheets (every 2nd frame): calm = tier1 0-38 ping-pong,
    // moving = tier2 0-70 loop, fists up = tier3 23-69 ping-pong.
    class DuckArt
    {
    public:
        DuckArt();

        juce::Component& head() { return headView; }
        juce::Component& body() { return bodyView; }

        // duck: 0 = full volume, 1 = silent. bob: the Bob switch under the body. duckKnob: the Duck knob, 0-100, picks
        // the loop (switching straight away). Called on the window's timer.
        void setDuck (float duck, bool bob, float duckKnob);

        struct Loop { int first, count; bool pingPong; };
        static constexpr int kSpriteFrames = 80, kSpriteColumns = 9, kSpriteW = 375, kSpriteH = 463;
        static constexpr double kSpriteFps = 12.0;
        static constexpr Loop kLoops[3] = { { 0, 20, true }, { 20, 36, false }, { 56, 24, true } };

        // the loop for a Duck knob value: 0-33 calm, 34-66 moving, 67-100 fists up
        static int loopFor (float duckKnob);
        // the sheet frame shown for a loop after this many frames of the clock
        static int frameAt (int loop, long long tick);

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
