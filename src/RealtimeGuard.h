#pragma once

// Debug-build check for the audio-thread rule (an Ingot standing rule): no memory allocation on the audio thread.
// Code that runs a block wraps itself in an AudioThreadScope; in a Debug build every allocation made inside a scope
// is counted (through the C runtime's debug allocation hook, so both operator new and malloc are seen).
// In a Release build the guard does nothing and isAvailable() returns false.
namespace ducker::realtime
{
    class AudioThreadScope
    {
    public:
        AudioThreadScope() noexcept;
        ~AudioThreadScope() noexcept;

        AudioThreadScope (const AudioThreadScope&) = delete;
        AudioThreadScope& operator= (const AudioThreadScope&) = delete;

    private:
        bool previous;
    };

    // True in builds where allocations are actually being watched.
    bool isAvailable() noexcept;

    // Turns the watch on (installs the hook). Safe to call more than once.
    void install() noexcept;

    // Number of allocations seen inside audio-thread scopes since the last reset.
    long violationCount() noexcept;
    void resetViolations() noexcept;
}
