#include "CurveEditor.h"

#include "Theme.h"

namespace ducker
{
    CurveEditor::CurveEditor (DuckerEngine& e) : engine (e)
    {
        setTitle ("Curve");
    }

    void CurveEditor::setCurve (const Curve& c)
    {
        if (dragPoint >= 0 || bendSegment >= 0)
            return;                                         // the user is editing; their curve wins
        if (c == pts)
            return;
        pts = c;
        hover = -1;
        repaint();
    }

    float CurveEditor::tx (double tick) const { return padX + (float) (tick / kCycleTicks) * ((float) getWidth() - padX * 2.0f); }
    float CurveEditor::ty (double v) const    { return padY + (float) (1.0 - v) * ((float) getHeight() - padY * 2.0f); }
    double CurveEditor::fromX (float x) const { return (double) (x - padX) / ((float) getWidth() - padX * 2.0f) * kCycleTicks; }
    double CurveEditor::fromY (float y) const { return 1.0 - (double) (y - padY) / ((float) getHeight() - padY * 2.0f); }

    int CurveEditor::hitPoint (juce::Point<float> p) const
    {
        int best = -1;
        float bestD = 11.0f;
        for (int i = 0; i < (int) pts.size(); ++i)
        {
            const float d = p.getDistanceFrom ({ tx ((double) pts[(size_t) i].tick), ty (pts[(size_t) i].value) });
            if (d < bestD)
            {
                bestD = d;
                best = i;
            }
        }
        return best;
    }

    int CurveEditor::segmentAt (juce::Point<float> p) const
    {
        const double t = fromX (p.x);
        for (int i = 0; i + 1 < (int) pts.size(); ++i)
            if (t >= (double) pts[(size_t) i].tick && t <= (double) pts[(size_t) i + 1].tick)
                return i;
        return -1;
    }

    void CurveEditor::changed()
    {
        repaint();
        if (onEdit)
            onEdit (pts);
    }

    void CurveEditor::refreshLive()
    {
        const float ph = engine.getPhase();
        bool any = ph != phase;
        phase = ph;
        for (int i = 0; i < DuckerEngine::kLiveBuckets; ++i)
        {
            const float in = engine.getLiveIn (i), out = engine.getLiveOut (i);
            any = any || in != liveIn[(size_t) i] || out != liveOut[(size_t) i];
            liveIn[(size_t) i] = in;
            liveOut[(size_t) i] = out;
        }
        float loud = 0.0f;
        for (int i = 0; i < DuckerEngine::kLiveBuckets; ++i)
            loud = juce::jmax (loud, liveIn[(size_t) i], liveOut[(size_t) i]);
        const float scale = loud > waveScale ? loud : juce::jmax (1.0e-4f, waveScale + (loud - waveScale) * 0.04f);
        any = any || scale != waveScale;
        waveScale = scale;
        if (any)
            repaint();
    }

    void CurveEditor::mouseMove (const juce::MouseEvent& e)
    {
        const int h = hitPoint (e.position);
        if (h != hover)
        {
            hover = h;
            repaint();
        }
        setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor
                               : (e.mods.isAltDown() ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::CrosshairCursor));
    }

    void CurveEditor::mouseExit (const juce::MouseEvent&)
    {
        if (hover != -1)
        {
            hover = -1;
            repaint();
        }
    }

    void CurveEditor::mouseDown (const juce::MouseEvent& e)
    {
        if (! e.mods.isLeftButtonDown())
            return;
        if (e.mods.isAltDown())
        {
            bendSegment = segmentAt (e.position);
            if (bendSegment >= 0)
            {
                bendStartY = e.position.y;
                bendStart = pts[(size_t) bendSegment].bend;
            }
            return;
        }
        dragPoint = hitPoint (e.position);
    }

    void CurveEditor::mouseDrag (const juce::MouseEvent& e)
    {
        if (bendSegment >= 0)
        {
            const auto& a = pts[(size_t) bendSegment];
            const auto& b = pts[(size_t) bendSegment + 1];
            const bool rising = b.value >= a.value;
            pts[(size_t) bendSegment].bend = juce::jlimit (-1.0, 1.0, bendStart + (double) (e.position.y - bendStartY) / 120.0 * (rising ? 1.0 : -1.0));
            changed();
            return;
        }
        if (dragPoint >= 0)
        {
            auto& p = pts[(size_t) dragPoint];
            const int n = (int) pts.size();
            if (dragPoint > 0 && dragPoint < n - 1)
                p.tick = juce::jlimit (pts[(size_t) dragPoint - 1].tick + 1, pts[(size_t) dragPoint + 1].tick - 1, (std::int64_t) std::llround (fromX (e.position.x)));
            p.value = juce::jlimit (0.0, 1.0, fromY (e.position.y));
            changed();
        }
    }

    void CurveEditor::mouseUp (const juce::MouseEvent&)
    {
        dragPoint = -1;
        bendSegment = -1;
    }

    void CurveEditor::mouseDoubleClick (const juce::MouseEvent& e)
    {
        const int i = hitPoint (e.position);
        if (i > 0 && i < (int) pts.size() - 1)
            pts.erase (pts.begin() + i);
        else if (i < 0)
        {
            const auto t = (std::int64_t) juce::jlimit (1.0, (double) kCycleTicks - 1.0, std::round (fromX (e.position.x)));
            for (auto& p : pts)
                if (p.tick == t)
                    return;
            pts.push_back ({ t, juce::jlimit (0.0, 1.0, fromY (e.position.y)), 0.0 });
            std::stable_sort (pts.begin(), pts.end(), [] (const CurvePoint& a, const CurvePoint& b) { return a.tick < b.tick; });
        }
        else
            return;
        hover = -1;
        changed();
    }

    void CurveEditor::paint (juce::Graphics& g)
    {
        const float w = (float) getWidth(), h = (float) getHeight();
        const float iw = w - padX * 2.0f, ih = h - padY * 2.0f;

        // grid: 8 columns (beat quarters stronger), 4 rows
        for (int i = 0; i <= 8; ++i)
        {
            const float x = std::round (padX + iw * (float) i / 8.0f) + 0.5f;
            g.setColour (theme::brass.withAlpha (i % 4 == 0 ? 0.35f : (i % 2 == 0 ? 0.2f : 0.09f)));
            g.drawVerticalLine ((int) x, padY, padY + ih);
        }
        g.setColour (theme::brass.withAlpha (0.16f));
        for (int i = 0; i <= 4; ++i)
            g.drawHorizontalLine ((int) std::round (padY + ih * (float) i / 4.0f), padX, padX + iw);

        // the sound folded onto the pass: a waveform mirrored around the middle line, in (faint) and out (yellow)
        const float bw = iw / (float) DuckerEngine::kLiveBuckets, mid = padY + ih * 0.5f;
        const auto half = [&] (float level) { return juce::jmin (1.0f, level / waveScale) * ih * 0.5f * 0.9f; };
        g.setColour (theme::brass.withAlpha (0.22f));
        g.drawHorizontalLine ((int) std::round (mid), padX, padX + iw);
        for (int i = 0; i < DuckerEngine::kLiveBuckets; ++i)
        {
            const float x = padX + (float) i * bw;
            if (liveIn[(size_t) i] > 0.0f)
            {
                const float hIn = half (liveIn[(size_t) i]);
                g.setColour (theme::cream.withAlpha (0.09f));
                g.fillRect (x, mid - hIn, bw + 0.3f, hIn * 2.0f);
            }
            if (liveOut[(size_t) i] > 0.0f)
            {
                const float hOut = half (liveOut[(size_t) i]);
                g.setColour (theme::yellow.withAlpha (0.22f));
                g.fillRect (x, mid - hOut, bw + 0.3f, hOut * 2.0f);
            }
        }

        // curve fill and line
        juce::Path line, fill;
        for (int x = 0; x <= (int) iw; ++x)
        {
            const float y = ty (curveAt (pts, (double) x / iw));
            x == 0 ? line.startNewSubPath (padX, y) : line.lineTo (padX + (float) x, y);
        }
        fill = line;
        fill.lineTo (padX + iw, ty (0.0));
        fill.lineTo (padX, ty (0.0));
        fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (theme::brass.withAlpha (0.22f), 0.0f, padY, theme::brass.withAlpha (0.04f), 0.0f, padY + ih, false));
        g.fillPath (fill);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.strokePath (line, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (theme::curveLine);
        g.strokePath (line, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // points
        for (int i = 0; i < (int) pts.size(); ++i)
        {
            const bool on = i == hover || i == dragPoint;
            const float r = on ? 7.0f : 5.5f;
            const float x = tx ((double) pts[(size_t) i].tick), y = ty (pts[(size_t) i].value);
            g.setColour (on ? juce::Colour (0xfffff1c9) : theme::brassHi);
            g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colour (0xff2a1d07));
            g.drawEllipse (x - r, y - r, r * 2.0f, r * 2.0f, 2.0f);
        }

        // playhead
        if (phase >= 0.0f && phase < 1.0f)
        {
            g.setColour (juce::Colour (0xfffff1c9).withAlpha (0.75f));
            const float x = std::round (padX + phase * iw) + 0.5f;
            g.drawLine (x, padY, x, padY + ih, 1.5f);
        }
    }
}
