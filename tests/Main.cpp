#include <juce_core/juce_core.h>

#include "RealtimeGuard.h"

namespace
{
    // The test runner logs through juce::Logger, which on Windows goes to the debugger only.
    struct ConsoleLogger : juce::Logger
    {
        void logMessage (const juce::String& message) override { std::printf ("%s\n", message.toRawUTF8()); std::fflush (stdout); }
    };
}

// Runs every juce::UnitTest; "--only <text>" runs the tests whose name contains the text.
int main (int argc, char* argv[])
{
    ducker::realtime::install();
    ConsoleLogger logger;
    juce::Logger::setCurrentLogger (&logger);

    juce::String only;
    for (int i = 1; i + 1 < argc; ++i)
        if (juce::String (argv[i]) == "--only")
            only = argv[i + 1];

    juce::Array<juce::UnitTest*> tests;
    for (auto* t : juce::UnitTest::getAllTests())
        if (only.isEmpty() || t->getName().containsIgnoreCase (only))
            tests.add (t);

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTests (tests);

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;
    std::printf ("\n%s: %d failure(s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures);
    juce::Logger::setCurrentLogger (nullptr);
    return failures == 0 ? 0 : 1;
}
