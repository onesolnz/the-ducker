#include "RealtimeGuard.h"

#include <atomic>

#if defined (_MSC_VER) && defined (_DEBUG)
 #define DUCKER_GUARD_ACTIVE 1
 #include <crtdbg.h>
#else
 #define DUCKER_GUARD_ACTIVE 0
#endif

namespace ducker::realtime
{
    namespace
    {
        thread_local bool inAudioScope = false;
        std::atomic<long> violations { 0 };

       #if DUCKER_GUARD_ACTIVE
        int __cdecl allocHook (int allocType, void*, size_t, int, long, const unsigned char*, int)
        {
            if (allocType == _HOOK_ALLOC && inAudioScope)
                violations.fetch_add (1, std::memory_order_relaxed);

            return 1;   // always allow the allocation; we only count it
        }
       #endif
    }

    AudioThreadScope::AudioThreadScope() noexcept : previous (inAudioScope) { inAudioScope = true; }
    AudioThreadScope::~AudioThreadScope() noexcept { inAudioScope = previous; }

    bool isAvailable() noexcept { return DUCKER_GUARD_ACTIVE != 0; }

    void install() noexcept
    {
       #if DUCKER_GUARD_ACTIVE
        static bool installed = false;

        if (! installed)
        {
            _CrtSetAllocHook (allocHook);
            installed = true;
        }
       #endif
    }

    long violationCount() noexcept { return violations.load (std::memory_order_relaxed); }
    void resetViolations() noexcept { violations.store (0, std::memory_order_relaxed); }
}
