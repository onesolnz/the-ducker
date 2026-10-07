cmake_minimum_required(VERSION 3.22)

project(TheDucker VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# Shared OneSol Audio code (steel look, knob, buttons, meter, test main...), compiled into each target below.
include(${CMAKE_CURRENT_SOURCE_DIR}/../Shared/OneSolShared.cmake)

# The spec is in the Ingot repo: specs/27-the-ducker-vst.md (mock-up: specs/mockup-the-ducker.html).
juce_add_plugin(TheDucker
    COMPANY_NAME "OneSol Audio"
    BUNDLE_ID "com.onesolproductions.theducker"
    PLUGIN_MANUFACTURER_CODE Osp1
    PLUGIN_CODE Dck1
    FORMATS VST3 Standalone
    PRODUCT_NAME "The Ducker"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES "Fx" "Dynamics"
    COPY_PLUGIN_AFTER_BUILD TRUE
    # Same folder as Progression: Ableton lists it as a custom VST3 folder, and copying there needs no admin rights.
    VST3_COPY_DIR "$ENV{USERPROFILE}/VST3"
)

juce_add_binary_data(TheDuckerArt
    SOURCES
        assets/ducker-logo.png
        assets/ducker-body.png
        assets/body.png
        assets/head_spritesheet.png
)

set(DUCKER_CORE_SOURCES
    src/Curve.cpp
    src/DuckerEngine.cpp
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/Widgets.cpp
    src/CurveEditor.cpp
    src/DuckArt.cpp
    src/PresetStore.cpp
)

target_sources(TheDucker PRIVATE ${DUCKER_CORE_SOURCES})
target_include_directories(TheDucker PRIVATE src)
onesol_use_shared(TheDucker)

set(DUCKER_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_compile_definitions(TheDucker PUBLIC ${DUCKER_DEFINITIONS})

target_link_libraries(TheDucker
    PRIVATE
        TheDuckerArt
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Offline tests (juce::UnitTest), run: build/TheDuckerTests_artefacts/Debug/TheDuckerTests.exe
juce_add_console_app(TheDuckerTests PRODUCT_NAME "TheDuckerTests")
target_sources(TheDuckerTests PRIVATE ${DUCKER_CORE_SOURCES} tests/Main.cpp tests/EngineTests.cpp tests/ProcessorTests.cpp)
target_include_directories(TheDuckerTests PRIVATE src)
onesol_use_shared(TheDuckerTests)
target_compile_definitions(TheDuckerTests PRIVATE ${DUCKER_DEFINITIONS} JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(TheDuckerTests
    PRIVATE
        TheDuckerArt
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
