# The Ducker

A volume-only ducker VST3 (Beat, Audio sidechain or MIDI trigger). Spec and mock-up live in the Ingot repo:
`specs/27-the-ducker-vst.md`, `specs/mockup-the-ducker.html`.

Build: `cmake -S . -B build`, then `cmake --build build --config Release --target TheDucker_VST3`
(copies to `%USERPROFILE%\VST3`). Tests: `--config Debug --target TheDuckerTests`, run
`build/TheDuckerTests_artefacts/Debug/TheDuckerTests.exe [--only <text>]`.
