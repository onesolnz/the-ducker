#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ducker
{
    // Everything that animates the duck art, in one place (spec answer 6): the head nods and the full-body duck bobs by how
    // hard the sound is being ducked right now. The only inputs are setDuck()'s.
    //
    // SPRITE TEST (branch sprite-test, 2026-10-06): the full-body duck is layered art (assets/layout.json): one body
    // (582 x 966) and 15 head frames (464 x 464, 5 x 3 in head_spritesheet.png) drawn on it at (-1, 5), the head tipped
    // from -3 to 18 degrees in 1.5 degree steps. The head nods once per beat, ping-pong, locked to the host's beat so the
    // deepest frame lands on the beat (a stopped host plays it at 12 frames a second). The Duck knob sets the size of the
    // nod: 0 % holds the rest frame (0 degrees, no nod), 100 % is the full range. The coded bob stays on top.
    class DuckArt
    {
    public:
        DuckArt();

        juce::Component& head() { return headView; }
        juce::Component& body() { return bodyView; }

        // duck: 0 = full volume, 1 = silent. bob: the Bob switch under the body. duckKnob: the Duck knob, 0-100, the
        // size of the nod. hostPlaying / ppq: the host's transport and position in quarter notes. Called on the window's timer.
        void setDuck (float duck, bool bob, float duckKnob, bool hostPlaying, double ppq);

        static constexpr int kHeadFrames = 15, kHeadColumns = 5, kHeadCell = 464, kRestFrame = 2, kDownFrame = 14;
        static constexpr double kBeatsPerNod = 1.0, kFreeFps = 12.0;
        // the body canvas (582 x 966) placed in the 375 x 463 cell the old full-body sprites used, so the duck keeps its
        // size and its feet line
        static constexpr float kCellW = 375.0f, kCellH = 463.0f, kBodyScale = 0.462f, kBodyX = 53.6f, kBodyY = 19.8f;
        static constexpr float kHeadX = -1.0f, kHeadY = 5.0f;

        // the head frame for a spot in the nod and a Duck knob value. cycle: position in beats divided by kBeatsPerNod;
        // a whole number is on the beat and shows the deepest frame. The frames play forwards then back, and the knob
        // scales how far from the rest frame they go.
        static int nodFrame (double cycle, float duckKnob);

    private:
        // An image fitted into the view, drawn with a transform around a pivot (a fraction of the view).
        // layered: the body, with the head frame drawn on it (both placed by the constants above).
        struct View : juce::Component
        {
            juce::Image image, headFrame;
            bool layered = false;
            juce::AffineTransform motion;
            void paint (juce::Graphics&) override;
        };

        View headView, bodyView;
        std::vector<juce::Image> headFrames;                // views into the sheet (no copies)
        int shownFrame = -1;
        double startMs = 0.0;
        float shownDuck = -1.0f;
        bool shownBob = true;
    };
}
