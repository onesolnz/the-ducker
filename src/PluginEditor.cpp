#include "PluginEditor.h"

#include "BinaryData.h"
#include "Theme.h"

namespace ducker
{
    namespace
    {
        const char* rateNames[] = { "1/4", "1/8", "1/16" };
        const char* triggerNames[] = { "Beat", "Audio", "MIDI" };

        // Status line texts (spec "The status line and the lamp").
        struct TriggerText { const char* every; const char* start; const char* next; const char* ok; const char* none; };
        const TriggerText triggerText[] = {
            { "Every",  "on the beat", "next ",   "Following the host's beat.", "" },
            { "Length", "on the hit",  "length ", "Listening to the sidechain.", "Nothing on the sidechain. Pick a source in Live's Sidechain box." },
            { "Length", "on the note", "length ", "Listening to MIDI.", "No MIDI arriving. Set a MIDI track's MIDI To to this track." },
        };

        float meterFraction (float amplitude)
        {
            const float db = amplitude > 1.0e-5f ? 20.0f * std::log10 (amplitude) : -100.0f;
            return juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        }

        juce::String percent (float v) { return juce::String (juce::roundToInt (v)) + "%"; }
    }

    // ---------- look and feel for menus and the name box ----------

    DuckerEditor::LookAndFeel::LookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1d1b18));
        setColour (juce::PopupMenu::textColourId, theme::cream);
        setColour (juce::PopupMenu::headerTextColourId, juce::Colour (0xff8d8473));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2e2920));
        setColour (juce::PopupMenu::highlightedTextColourId, theme::brassHi);
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1d1b18));
        setColour (juce::AlertWindow::textColourId, theme::cream);
        setColour (juce::AlertWindow::outlineColourId, theme::brassLo);
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0e0d0c));
        setColour (juce::TextEditor::textColourId, theme::cream);
        setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff3a3226));
        setColour (juce::TextEditor::focusedOutlineColourId, theme::brass);
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2e2920));
        setColour (juce::TextButton::textColourOffId, theme::brassHi);
        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff1d1b18));
        setColour (juce::TooltipWindow::textColourId, theme::cream);
        setColour (juce::TooltipWindow::outlineColourId, theme::brassLo);
    }

    juce::Font DuckerEditor::LookAndFeel::getPopupMenuFont()        { return theme::label (17.0f); }
    juce::Font DuckerEditor::LookAndFeel::getAlertWindowTitleFont() { return theme::label (19.0f, true); }
    juce::Font DuckerEditor::LookAndFeel::getAlertWindowMessageFont() { return theme::label (16.0f); }
    juce::Font DuckerEditor::LookAndFeel::getAlertWindowFont()      { return theme::label (16.0f); }

    // ---------- the window ----------

    DuckerEditor::DuckerEditor (DuckerProcessor& p) : AudioProcessorEditor (p), proc (p), curve (p.engine)
    {
        setLookAndFeel (&lnf);
        logo = juce::ImageCache::getFromMemory (BinaryData::duckerlogo_png, BinaryData::duckerlogo_pngSize);
        makeTexture();
        addAndMakeVisible (panel);

        auto& params = proc.params;
        auto param = [&params] (const char* id) -> juce::RangedAudioParameter& { return *params.getParameter (id); };

        // column 1: DUCK with Mix, Smooth, Offset/Delay under it
        duckKnob = std::make_unique<Knob> (param (DuckerProcessor::duckId), "DUCK", 230, percent);
        mixKnob = std::make_unique<Knob> (param (DuckerProcessor::mixId), "Mix", 50, percent);
        smoothKnob = std::make_unique<Knob> (param (DuckerProcessor::smoothId), "Smooth", 50, [] (float v) { return juce::String (v, 1) + "ms"; });
        offsetKnob = std::make_unique<Knob> (param (DuckerProcessor::offsetId), "Offset", 50, [] (float v) { return (v > 0.0f ? "+" : "") + percent (v); });
        delayKnob = std::make_unique<Knob> (param (DuckerProcessor::delayId), "Delay", 50, [] (float v) { return juce::String (juce::roundToInt (v)) + "ms"; });
        duckKnob->setBounds (19, 68, 260, 304);
        mixKnob->setBounds (52 - 7, 436, 64, 90);
        smoothKnob->setBounds (124 - 7, 436, 64, 90);
        offsetKnob->setBounds (196 - 7, 436, 64, 90);
        delayKnob->setBounds (196 - 7, 436, 64, 90);
        for (auto* k : { duckKnob.get(), mixKnob.get(), smoothKnob.get(), offsetKnob.get(), delayKnob.get() })
            panel.addAndMakeVisible (*k);

        // column 2: presets and the curve
        presetPill.onClick = [this] { showPresetMenu(); };
        savePill.onClick = [this] { askName ("Save preset", proc.presetEdited ? juce::String() : proc.presetName + " copy", [this] (juce::String n) { saveCurrent (n); }); };
        panel.addAndMakeVisible (presetPill);
        panel.addAndMakeVisible (savePill);
        curve.setBounds (302, 108, 380, 330);
        curve.setCurve (proc.getCurve());
        curve.onEdit = [this] (const Curve& c)
        {
            proc.setCurve (c);
            proc.presetEdited = true;
            refreshPresetUi();
        };
        panel.addAndMakeVisible (curve);

        // column 3: rate, shapes, trigger
        for (int i = 0; i < 3; ++i)
        {
            rateButtons[(size_t) i] = std::make_unique<Pill> (rateNames[i], Pill::Style::square);
            rateButtons[(size_t) i]->setBounds (760 + i * 54, 106, 50, 40);
            rateButtons[(size_t) i]->onClick = [this, i] { rateWatch->setValueAsCompleteGesture ((float) i); };
            panel.addAndMakeVisible (*rateButtons[(size_t) i]);

            triggerButtons[(size_t) i] = std::make_unique<Pill> (triggerNames[i], Pill::Style::square);
            triggerButtons[(size_t) i]->setBounds (712 + i * 70, 428, 64, 40);
            triggerButtons[(size_t) i]->onClick = [this, i] { triggerWatch->setValueAsCompleteGesture ((float) i); };
            panel.addAndMakeVisible (*triggerButtons[(size_t) i]);
        }
        const auto& shapes = factoryShapes();
        for (int i = 0; i < (int) shapes.size(); ++i)
        {
            auto b = std::make_unique<ShapeButton> (shapes[(size_t) i]);
            b->setBounds (712 + (i % 4) * 54, 198 + (i / 4) * 48, 48, 40);
            b->onClick = [this, i]
            {
                // a shape loads the curve only; the knobs stay
                const auto& s = factoryShapes()[(size_t) i];
                proc.setCurve (s.points);
                proc.presetName = s.name;
                proc.presetEdited = false;
                curve.setCurve (proc.getCurve());
                refreshPresetUi();
            };
            panel.addAndMakeVisible (*b);
            shapeButtons.push_back (std::move (b));
        }
        rateWatch = std::make_unique<juce::ParameterAttachment> (param (DuckerProcessor::rateId), [this] (float v) { shownRate = juce::jlimit (0, 2, juce::roundToInt (v)); refreshTriggerUi(); });
        triggerWatch = std::make_unique<juce::ParameterAttachment> (param (DuckerProcessor::triggerId), [this] (float v) { shownTrigger = juce::jlimit (0, 2, juce::roundToInt (v)); refreshTriggerUi(); });

        // column 4: the duck and its Bob switch; column 5: head and meter
        art.body().setBounds (944, 190, 290, 320);
        panel.addAndMakeVisible (art.body());
        bobPill.setBounds (1166, 530, 64, 34);
        bobPill.setOn (proc.bob);
        bobPill.setTooltip ("The duck bobs with the ducking");
        bobPill.onClick = [this]
        {
            proc.bob = ! proc.bob;
            bobPill.setOn (proc.bob);
        };
        panel.addAndMakeVisible (bobPill);
        art.head().setBounds (1256, 42, 86, 88);
        panel.addAndMakeVisible (art.head());
        meter.setBounds (1262, 166, Meter::barWidth * 2 + Meter::gap, 360);
        meter.setInterceptsMouseClicks (false, false);
        panel.addAndMakeVisible (meter);

        // title strip: window size
        sizePill.onClick = [this]
        {
            juce::PopupMenu m;
            for (int pct : { 75, 100, 125, 150 })
                m.addItem (pct, juce::String (pct) + "%", true, juce::roundToInt (proc.windowScale * 100.0f) == pct);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sizePill), [this] (int r) { if (r > 0) setScale ((float) r / 100.0f); });
        };
        panel.addAndMakeVisible (sizePill);

        refreshPresetUi();
        rateWatch->sendInitialUpdate();
        triggerWatch->sendInitialUpdate();
        art.setDuck (0.0f, proc.bob);
        lastHits = proc.engine.getHitCount();
        lastTick = juce::Time::getMillisecondCounterHiRes();
        setScale (proc.windowScale);
        timerCallback();                 // the status line and meter show at once
        startTimerHz (30);
    }

    DuckerEditor::~DuckerEditor()
    {
        stopTimer();
        setLookAndFeel (nullptr);
    }

    void DuckerEditor::setScale (float s)
    {
        proc.windowScale = juce::jlimit (0.75f, 1.5f, s);
        sizePill.setText (juce::String (juce::roundToInt (proc.windowScale * 100.0f)) + "%");
        sizePill.setBounds (theme::width - 10 - sizePill.idealWidth(), 5, sizePill.idealWidth(), 26);
        panel.setTransform (juce::AffineTransform::scale (proc.windowScale));
        setSize (juce::roundToInt (theme::width * proc.windowScale), juce::roundToInt (theme::height * proc.windowScale));
    }

    void DuckerEditor::resized()
    {
        panel.setBounds (0, 0, theme::width, theme::height);
    }

    // The attachments pass the new value before the plug-in's stored value has caught up, so the window keeps its own copy.
    int DuckerEditor::triggerIndex() const { return shownTrigger; }
    int DuckerEditor::rateIndex() const    { return shownRate; }

    void DuckerEditor::refreshTriggerUi()
    {
        const int trig = triggerIndex(), rate = rateIndex();
        for (int i = 0; i < 3; ++i)
        {
            rateButtons[(size_t) i]->setOn (i == rate);
            triggerButtons[(size_t) i]->setOn (i == trig);
        }
        offsetKnob->setVisible (trig == 0);          // Offset slides the curve against the beat grid: Beat only
        delayKnob->setVisible (trig != 0);           // Delay waits after each hit: Audio and MIDI
        silentSeconds = 0.0;
        panel.repaint();
    }

    void DuckerEditor::refreshPresetUi()
    {
        presetPill.setText (proc.presetName + (proc.presetEdited ? " *" : ""));
        const int w = juce::jmin (300, presetPill.idealWidth());
        presetPill.setBounds (300, 58, w, 34);
        savePill.setBounds (juce::jmax (456, 300 + w + 12), 58, savePill.idealWidth(), 34);
        const auto& shapes = factoryShapes();
        for (size_t i = 0; i < shapes.size(); ++i)
            shapeButtons[i]->setOn (! proc.presetEdited && proc.presetName == shapes[i].name);
    }

    void DuckerEditor::setParam (const char* id, float plain)
    {
        if (auto* p = proc.params.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (plain));
            p->endChangeGesture();
        }
    }

    void DuckerEditor::loadFactory (int index)
    {
        // a factory preset: its curve with the knobs at their defaults (the Trigger stays)
        const auto& s = factoryShapes()[(size_t) index];
        for (const char* id : { DuckerProcessor::duckId, DuckerProcessor::mixId, DuckerProcessor::smoothId, DuckerProcessor::offsetId, DuckerProcessor::delayId, DuckerProcessor::rateId })
            if (auto* p = proc.params.getParameter (id))
                setParam (id, p->convertFrom0to1 (p->getDefaultValue()));
        proc.setCurve (s.points);
        proc.presetName = s.name;
        proc.presetEdited = false;
        curve.setCurve (proc.getCurve());
        refreshPresetUi();
    }

    void DuckerEditor::loadUser (const UserPreset& p)
    {
        setParam (DuckerProcessor::duckId, p.duck);
        setParam (DuckerProcessor::mixId, p.mix);
        setParam (DuckerProcessor::smoothId, p.smooth);
        setParam (DuckerProcessor::offsetId, p.offset);
        setParam (DuckerProcessor::delayId, p.delay);
        setParam (DuckerProcessor::rateId, (float) p.rate);
        proc.setCurve (p.curve);
        proc.presetName = p.name;
        proc.presetEdited = false;
        curve.setCurve (proc.getCurve());
        refreshPresetUi();
    }

    void DuckerEditor::showPresetMenu()
    {
        const auto user = presetStore::load (presetStore::defaultFile());     // read every time: other instances may have saved
        const auto& shapes = factoryShapes();
        juce::PopupMenu m;
        m.addSectionHeader ("Factory");
        for (int i = 0; i < (int) shapes.size(); ++i)
            m.addItem (1 + i, shapes[(size_t) i].name, true, ! proc.presetEdited && proc.presetName == shapes[(size_t) i].name);
        m.addSectionHeader ("Yours");
        if (user.empty())
            m.addItem (-1, "(none saved yet)", false);
        bool currentIsUser = false;
        for (int i = 0; i < (int) user.size(); ++i)
        {
            const bool current = proc.presetName == user[(size_t) i].name;
            currentIsUser = currentIsUser || current;
            m.addItem (100 + i, user[(size_t) i].name, true, current && ! proc.presetEdited);
        }
        if (currentIsUser)
        {
            m.addSeparator();
            m.addItem (500, "Rename \"" + proc.presetName + "\"...");
            m.addItem (501, "Delete \"" + proc.presetName + "\"");
        }

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetPill),
                         [this, user, shapes = (int) shapes.size()] (int r)
        {
            if (r >= 1 && r <= shapes)
                loadFactory (r - 1);
            else if (r >= 100 && r < 100 + (int) user.size())
                loadUser (user[(size_t) (r - 100)]);
            else if (r == 500)
            {
                const auto old = proc.presetName;
                askName ("Rename preset", old, [this, old] (juce::String name)
                {
                    auto list = presetStore::load (presetStore::defaultFile());
                    list.erase (std::remove_if (list.begin(), list.end(), [&] (const UserPreset& u) { return u.name == name && name != old; }), list.end());
                    for (auto& u : list)
                        if (u.name == old)
                            u.name = name;
                    presetStore::save (presetStore::defaultFile(), list);
                    proc.presetName = name;
                    refreshPresetUi();
                });
            }
            else if (r == 501)
            {
                const auto name = proc.presetName;
                auto list = presetStore::load (presetStore::defaultFile());
                list.erase (std::remove_if (list.begin(), list.end(), [&] (const UserPreset& u) { return u.name == name; }), list.end());
                presetStore::save (presetStore::defaultFile(), list);
                proc.presetEdited = true;                // the sound stays; it is no longer a saved preset
                refreshPresetUi();
            }
        });
    }

    void DuckerEditor::askName (const juce::String& title, const juce::String& initial, std::function<void (juce::String)> done)
    {
        auto* w = new juce::AlertWindow (title, {}, juce::MessageBoxIconType::NoIcon, this);
        w->setLookAndFeel (&lnf);
        w->addTextEditor ("name", initial, "Name:");
        if (auto* ed = w->getTextEditor ("name"))
        {
            ed->setInputRestrictions (40);
            ed->selectAll();
        }
        w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        juce::Component::SafePointer<DuckerEditor> safe (this);
        w->enterModalState (true, juce::ModalCallbackFunction::create ([w, safe, done] (int result)
        {
            const auto name = w->getTextEditorContents ("name").trim();
            if (result == 1 && name.isNotEmpty() && safe != nullptr)
                done (name);
        }), true);
    }

    void DuckerEditor::saveCurrent (const juce::String& name)
    {
        UserPreset p;
        p.name = name;
        p.curve = proc.getCurve();
        auto value = [this] (const char* id) { return proc.params.getRawParameterValue (id)->load(); };
        p.duck = value (DuckerProcessor::duckId);
        p.mix = value (DuckerProcessor::mixId);
        p.smooth = value (DuckerProcessor::smoothId);
        p.offset = value (DuckerProcessor::offsetId);
        p.delay = value (DuckerProcessor::delayId);
        p.rate = rateIndex();

        auto list = presetStore::load (presetStore::defaultFile());
        list.erase (std::remove_if (list.begin(), list.end(), [&] (const UserPreset& u) { return u.name == name; }), list.end());
        list.push_back (p);
        if (! presetStore::save (presetStore::defaultFile(), list))
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "The Ducker", "Could not save to " + presetStore::defaultFile().getFullPathName());
            return;
        }
        proc.presetName = name;
        proc.presetEdited = false;
        refreshPresetUi();
    }

    // ---------- live parts ----------

    void DuckerEditor::timerCallback()
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        const double dt = juce::jlimit (0.0, 0.2, (now - lastTick) * 0.001);
        lastTick = now;
        auto& engine = proc.engine;

        curve.refreshLive();

        // the curve may have been changed by the host (a loaded state)
        if (curve.isEnabled() && ! curve.isMouseButtonDown())
            curve.setCurve (proc.getCurve());

        // meter
        const float in = meterFraction (engine.takeInPeak()), out = meterFraction (engine.takeOutPeak());
        inMark = juce::jmax (inMark - (float) dt * 0.25f, in);
        outMark = juce::jmax (outMark - (float) dt * 0.25f, out);
        meter.setLevels (in, out, inMark, outMark);

        // duck art and the gain reduction readout
        const float duck = engine.getDuckAmount();
        art.setDuck (duck, proc.bob);

        // lamp and status line
        const int trig = triggerIndex();
        const bool playing = proc.hostPlaying.load();
        const int hits = engine.getHitCount();
        if (hits != lastHits)
        {
            lastHits = hits;
            lampUntil = now + 90.0;
            silentSeconds = 0.0;
        }
        if (playing && trig != 0)
            silentSeconds += dt;
        else
            silentSeconds = 0.0;
        const double twoBars = 8.0 * 60.0 / juce::jmax (20.0, proc.hostBpm.load());
        const bool warn = trig != 0 && silentSeconds > twoBars;
        const auto& tt = triggerText[trig];
        const juce::String text = warn ? tt.none : (trig == 0 && ! playing ? "Waiting for the host to play." : tt.ok);

        if (text != status || warn != statusWarn || std::abs (duck - shownGr) > 0.001f || playing != shownPlaying || (lampUntil > now - 60.0))
        {
            status = text;
            statusWarn = warn;
            shownGr = duck;
            shownPlaying = playing;
            panel.repaint (700, 470, 240, 60);
            panel.repaint (1244, 526, 106, 50);
        }
    }

    // ---------- the panel ----------

    void DuckerEditor::makeTexture()
    {
        // a stand-in metal panel until the user's art (spec "Window"): dark gradient, grain, scratches and smudges
        const int w = theme::width, h = theme::height;
        texture = juce::Image (juce::Image::RGB, w, h, true);
        {
            juce::Graphics g (texture);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2f2c28), w * 0.45f, h * 0.4f, juce::Colour (0xff151413), w * 0.45f + h * 0.75f, h * 0.4f, true));
            g.fillAll();
        }
        juce::Random rnd (7);
        {
            juce::Image::BitmapData bd (texture, juce::Image::BitmapData::readWrite);
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                {
                    const auto c = bd.getPixelColour (x, y);
                    const int n = (int) ((rnd.nextFloat() - 0.5f) * 18.0f);
                    bd.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, c.getRed() + n),
                                                           (juce::uint8) juce::jlimit (0, 255, c.getGreen() + n),
                                                           (juce::uint8) juce::jlimit (0, 255, c.getBlue() + (int) (n * 0.9f))));
                }
        }
        juce::Graphics g (texture);
        for (int i = 0; i < 260; ++i)
        {
            const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * h, len = 10.0f + rnd.nextFloat() * 70.0f, a = rnd.nextFloat() * juce::MathConstants<float>::pi;
            g.setColour (rnd.nextBool() ? juce::Colour (0xfffff0d2).withAlpha (0.05f) : juce::Colours::black.withAlpha (0.25f));
            g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len * 0.3f, rnd.nextFloat() < 0.8f ? 0.6f : 1.2f);
        }
        for (int i = 0; i < 40; ++i)
        {
            const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * h, r = 8.0f + rnd.nextFloat() * 40.0f;
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.22f), x, y, juce::Colours::transparentBlack, x + r, y, true));
            g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
        }
        // the line before the meter column
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawLine (1242.0f, 44.0f, 1242.0f, 590.0f, 3.0f);
        g.setColour (theme::brass.withAlpha (0.25f));
        g.drawLine (1244.0f, 44.0f, 1244.0f, 590.0f, 1.0f);
    }

    void DuckerEditor::paintPanel (juce::Graphics& g)
    {
        g.drawImageAt (texture, 0, 0);

        // title strip
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb8923e), 0.0f, 0.0f, juce::Colour (0xff8a6a2a), 0.0f, 36.0f, false));
        g.fillRect (0, 0, theme::width, 36);
        g.setColour (juce::Colours::black);
        g.fillRect (0, 36, theme::width, 2);

        // screws
        for (auto pos : { juce::Point<float> (17.0f, 53.0f), juce::Point<float> ((float) theme::width - 17.0f, 53.0f),
                          juce::Point<float> (17.0f, (float) theme::height - 17.0f), juce::Point<float> ((float) theme::width - 17.0f, (float) theme::height - 17.0f) })
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff77705f), pos.x - 2.0f, pos.y - 2.0f, juce::Colour (0xff2a2722), pos.x + 5.0f, pos.y + 5.0f, true));
            g.fillEllipse (pos.x - 7.0f, pos.y - 7.0f, 14.0f, 14.0f);
            g.setColour (juce::Colours::black);
            g.drawEllipse (pos.x - 7.0f, pos.y - 7.0f, 14.0f, 14.0f, 1.0f);
            g.drawLine (juce::Line<float> (pos.x - 4.0f, pos.y - 3.0f, pos.x + 4.0f, pos.y + 3.0f), 2.0f);
        }

        // the curve's well
        {
            const juce::Rectangle<float> well (300.0f, 106.0f, 384.0f, 334.0f);
            g.setColour (juce::Colour (0xff3a3226));
            g.drawRoundedRectangle (well.expanded (2.5f), 7.0f, 1.0f);
            g.setColour (juce::Colours::black);
            g.fillRoundedRectangle (well.expanded (2.0f), 7.0f);
            g.setColour (theme::well);
            g.fillRoundedRectangle (well, 6.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.7f), 0.0f, well.getY(), juce::Colours::transparentBlack, 0.0f, well.getY() + 8.0f, false));
            g.fillRoundedRectangle (well.withHeight (8.0f), 6.0f);
        }

        const int trig = triggerIndex(), rate = rateIndex();
        const auto& tt = triggerText[trig];

        // labels
        g.setFont (theme::label (15.0f));
        theme::shadowText (g, tt.start, { 302.0f, 446.0f, 190.0f, 20.0f }, juce::Justification::centredLeft, theme::dimText);
        theme::shadowText (g, juce::String (tt.next) + rateNames[rate], { 492.0f, 446.0f, 190.0f, 20.0f }, juce::Justification::centredRight, theme::dimText);
        theme::shadowText (g, juce::String::fromUTF8 ("Drag a point \xc2\xb7 double-click adds or removes"), { 302.0f, 474.0f, 380.0f, 20.0f }, juce::Justification::centredLeft, theme::dimText);
        theme::shadowText (g, "Alt-drag a line to bend it", { 302.0f, 494.0f, 380.0f, 20.0f }, juce::Justification::centredLeft, theme::dimText);

        g.setFont (theme::label (17.0f, true));
        theme::shadowText (g, tt.every, { 712.0f, 106.0f, 48.0f, 40.0f }, juce::Justification::centredLeft, theme::cream);
        theme::shadowText (g, "Shapes", { 712.0f, 166.0f, 120.0f, 24.0f }, juce::Justification::centredLeft, theme::cream);
        theme::shadowText (g, "Trigger", { 712.0f, 398.0f, 120.0f, 24.0f }, juce::Justification::centredLeft, theme::cream);

        // lamp and status line
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (trig != 0)
        {
            const juce::Rectangle<float> lamp (714.0f, 488.0f, 16.0f, 16.0f);
            if (now < lampUntil)
            {
                g.setColour (theme::yellow.withAlpha (0.45f));
                g.fillEllipse (lamp.expanded (5.0f));
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff3b0), lamp.getX() + 5.0f, lamp.getY() + 5.0f, theme::orange, lamp.getRight(), lamp.getBottom(), true));
            }
            else
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a3a1a), lamp.getX() + 5.0f, lamp.getY() + 5.0f, juce::Colour (0xff1a1408), lamp.getRight(), lamp.getBottom(), true));
            g.fillEllipse (lamp);
            g.setColour (juce::Colours::black);
            g.drawEllipse (lamp, 1.5f);
        }
        g.setFont (theme::label (15.0f));
        const juce::Rectangle<int> statusArea (trig == 0 ? 712 : 738, 482, trig == 0 ? 210 : 190, 44);
        g.setColour (juce::Colours::black);
        g.drawFittedText (status, statusArea.translated (0, 1), juce::Justification::topLeft, 2, 1.0f);
        g.setColour (statusWarn ? theme::orange : theme::dimText);
        g.drawFittedText (status, statusArea, juce::Justification::topLeft, 2, 1.0f);

        // logo above the duck
        if (logo.isValid())
            g.drawImage (logo, juce::Rectangle<float> (976.0f, 62.0f, 220.0f, 107.0f), juce::RectanglePlacement::centred);

        // meter labels and the gain reduction
        g.setFont (theme::label (16.0f, true));
        theme::shadowText (g, "DUCK METER", { 1250.0f, 132.0f, 96.0f, 22.0f }, juce::Justification::centred, theme::cream);
        g.setFont (theme::label (13.0f, true));
        theme::shadowText (g, "IN", { 1256.0f, 530.0f, 42.0f, 16.0f }, juce::Justification::centred, theme::dimText);
        theme::shadowText (g, "OUT", { 1298.0f, 530.0f, 42.0f, 16.0f }, juce::Justification::centred, theme::dimText);
        const float gain = 1.0f - shownGr;
        const juce::String gr = shownGr < 0.001f ? "0.0 dB" : (gain <= 1.0e-5f ? "-inf dB" : juce::String (20.0f * std::log10 (gain), 1) + " dB");
        g.setFont (theme::label (15.0f, true));
        theme::shadowText (g, gr, { 1250.0f, 550.0f, 96.0f, 18.0f }, juce::Justification::centred, theme::brassHi);
    }
}
