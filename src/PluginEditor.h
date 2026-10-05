#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "CurveEditor.h"
#include "DuckArt.h"
#include "PluginProcessor.h"
#include "PresetStore.h"
#include "Widgets.h"

namespace ducker
{
    // The Ducker's window (specs/27-the-ducker-vst.md "Window", mockup-the-ducker.html): 1350 x 600 at 100 %, five columns.
    // Everything is laid out at 100 % on one panel that is scaled as a whole for 75 to 150 %.
    class DuckerEditor : public juce::AudioProcessorEditor, private juce::Timer
    {
    public:
        explicit DuckerEditor (DuckerProcessor&);
        ~DuckerEditor() override;

        void resized() override;
        void paint (juce::Graphics&) override {}

    private:
        struct Panel : juce::Component
        {
            explicit Panel (DuckerEditor& e) : editor (e) {}
            void paint (juce::Graphics& g) override { editor.paintPanel (g); }
            DuckerEditor& editor;
        };

        struct LookAndFeel : juce::LookAndFeel_V4
        {
            LookAndFeel();
            juce::Font getPopupMenuFont() override;
            juce::Font getAlertWindowTitleFont() override;
            juce::Font getAlertWindowMessageFont() override;
            juce::Font getAlertWindowFont() override;
        };

        void timerCallback() override;
        void paintPanel (juce::Graphics&);
        void makeTexture();

        void setScale (float s);
        void refreshPresetUi();
        void refreshTriggerUi();
        int triggerIndex() const;
        int rateIndex() const;

        void showPresetMenu();
        void loadFactory (int index);
        void loadUser (const UserPreset& p);
        void askName (const juce::String& title, const juce::String& initial, std::function<void (juce::String)> done);
        void saveCurrent (const juce::String& name);
        void setParam (const char* id, float plain);

        DuckerProcessor& proc;
        LookAndFeel lnf;
        Panel panel { *this };
        juce::Image texture, logo;
        juce::TooltipWindow tooltips { this, 600 };

        Pill sizePill { "100%", Pill::Style::title, true };
        Pill presetPill { "", Pill::Style::pill, true };
        Pill savePill { "+ Save current", Pill::Style::pill };
        Pill bobPill { "Bob", Pill::Style::square };
        std::array<std::unique_ptr<Pill>, 3> rateButtons, triggerButtons;
        std::vector<std::unique_ptr<ShapeButton>> shapeButtons;
        std::unique_ptr<Knob> duckKnob, mixKnob, smoothKnob, offsetKnob, delayKnob;
        CurveEditor curve;
        Meter meter;
        DuckArt art;
        std::unique_ptr<juce::ParameterAttachment> rateWatch, triggerWatch;

        // status line, lamp and meter state (timer)
        int lastHits = 0;
        double lampUntil = 0.0, lastTick = 0.0, silentSeconds = 0.0;
        bool statusWarn = false;
        juce::String status;
        float inMark = 0.0f, outMark = 0.0f, shownGr = 0.0f;
        bool shownPlaying = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuckerEditor)
    };
}
