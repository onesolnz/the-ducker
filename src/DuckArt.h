#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ducker
{
    // Everything that animates the duck art, in one place (spec answer 6): the head nods and the full-body duck bobs by how
    // hard the sound is being ducked right now. A frame-by-frame animation can later replace what these two views draw
    // without touching the rest of the window: the only input is setDuck().
    class DuckArt
    {
    public:
        DuckArt();

        juce::Component& head() { return headView; }
        juce::Component& body() { return bodyView; }

        // duck: 0 = full volume, 1 = silent. bob: the Bob switch under the body.
        void setDuck (float duck, bool bob);

    private:
        // An image fitted into the view, drawn with a transform around a pivot (a fraction of the view).
        struct View : juce::Component
        {
            juce::Image image;
            juce::AffineTransform motion;
            void paint (juce::Graphics&) override;
        };

        View headView, bodyView;
        float shownDuck = -1.0f;
        bool shownBob = true;
    };
}
