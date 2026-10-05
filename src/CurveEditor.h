#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Curve.h"
#include "DuckerEngine.h"

namespace ducker
{
    // The curve over one pass (mock-up column 2): drag a point, double-click adds or removes one, Alt-drag a line to bend it.
    // The first and last points stay on the edges. Behind the curve: the sound that came in and went out, folded onto the
    // pass, and a playhead line.
    class CurveEditor : public juce::Component
    {
    public:
        CurveEditor (DuckerEngine& engine);

        void setCurve (const Curve& c);                       // from the plug-in (no callback)
        std::function<void (const Curve&)> onEdit;            // every change the user makes

        void refreshLive();                                   // timer: playhead and the live bars

        void paint (juce::Graphics&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;

        static constexpr float padX = 12.0f, padY = 14.0f;

    private:
        float tx (double tick) const;
        float ty (double value) const;
        double fromX (float x) const;
        double fromY (float y) const;
        int hitPoint (juce::Point<float> p) const;
        int segmentAt (juce::Point<float> p) const;
        void changed();

        DuckerEngine& engine;
        Curve pts;
        int hover = -1, dragPoint = -1, bendSegment = -1;
        float bendStartY = 0.0f;
        double bendStart = 0.0;
        float phase = -1.0f;
        std::array<float, DuckerEngine::kLiveBuckets> liveIn {}, liveOut {};
    };
}
