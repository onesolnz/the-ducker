#include <juce_core/juce_core.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include <onesol/TestMain.h>

#include "PluginProcessor.h"

// Runs every juce::UnitTest ("--only <text>"), or "--snapshot <file.png> [--trigger 0|1|2]": draws the plug-in window to a PNG
// (to check the layout without a host).
int main (int argc, char* argv[])
{
    return onesol::testing::run (argc, argv, [] (const juce::String& snapshot, const juce::StringArray& args)
    {
        juce::String trigger;
        for (int i = 0; i + 1 < args.size(); ++i)
            if (args[i] == "--trigger")
                trigger = args[i + 1];

        juce::ScopedJuceInitialiser_GUI gui;
        ducker::DuckerProcessor proc;
        if (trigger.isNotEmpty())
            if (auto* p = proc.params.getParameter (ducker::DuckerProcessor::triggerId))
                p->setValueNotifyingHost (p->convertTo0to1 (trigger.getFloatValue()));
        std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        juce::File file (juce::File::getCurrentWorkingDirectory().getChildFile (snapshot));
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat().writeImageToStream (image, out);
        std::printf ("wrote %s\n", file.getFullPathName().toRawUTF8());
        return 0;
    });
}
