# 27 - The Ducker (VST3 plug-in)

Status: BUILT 2026-10-05 (steps 1-5) in `D:\Claude Projects\The Ducker VST` (GitHub `onesolnz/the-ducker`); window waiting for the user's check in Live. Labels use Bahnschrift (ships with Windows) in place of the mock-up's Oswald. Mock-up: `mockup-the-ducker.html` (interactive; open it through the `mockups` preview server, `.claude/serve.ps1`, http://localhost:8765/specs/mockup-the-ducker.html).
Art: `assets/Ducker Logo.png` (full-body duck), `assets/Ducker Text.png` (logo), `assets/Icon Head.png` (head); cropped copies in `assets/ducker/`.

## What it is
A volume ducker as a stand-alone VST3 plug-in for other DAWs (tested in Ableton Live first). It plays a drawn volume curve, either on the beat or each time a
sidechain hit or a MIDI note arrives, so a pad or bass pumps with the kick. It only changes the VOLUME: no compressor, no detector shaping the gain, no change
of tone. That is the point of it (the user's reason: a compressor changes the sound, a ducker only turns it down).

It is a separate product from Ingot's native Ducker (`25-native-fx-plan.md`, `engine/DuckerDevice`). The two share the idea and the curve maths, not the look:
- The Ducker VST has its own look (brass and yellow, the cartoon duck, the big DUCK knob, a wide window).
- Ingot's native Ducker keeps Ingot's forge theme and card style like the Limiter and the Synth; it never borrows this skin.
- Features can travel to the native Ducker later in Ingot's style (for example the Audio trigger once Ingot has track-to-track routing).

## Decisions (the user, 2026-10-05)
1. Wide window (about 2.25 : 1), not tall. Columns left to right: DUCK knob, curve, rate/shapes/trigger, logo over the duck, head and meter.
2. A big DUCK knob replaces Depth (same job: how far the volume drops). Mix, Smooth, Offset are small knobs under it.
3. The text logo sits above the full-body duck.
4. Trigger: **Beat | Audio | MIDI**, all three in the first version. The user uses MIDI most when the timing must be unique.
5. Audio and MIDI only TRIGGER the curve (start it from the top); they never shape the gain. Not compression.
6. MIDI reacts to the START of each note only. Note length, velocity and pitch do nothing.
7. No threshold knob: the user always triggers from a clean single sound (a kick on its own), so hit detection is automatic and fixed.
8. In Audio and MIDI the Offset knob becomes **Delay** (0 to 100 ms after each hit).
9. Routing is the host's job. In Live: the Sidechain box on the plug-in's device for Audio, a MIDI track's "MIDI To" for MIDI. No track picker inside the plug-in.
10. A status line under the Trigger buttons says what it is listening to, and says where to route a source when nothing arrives.
11. The user will make custom knob and pill art later; until then knobs and pills are drawn in code as in the mock-up.

## Window
- Layout drawn at 1350 x 600; 100% shows it at 0.75, so 1012 x 450 (changed 2026-10-05: the user found 1350 x 600 too big). The title-bar size box offers 100, 125, 150, 175, 200 % (200 % = 1350 x 600 x 1.5; the whole window scales; art supplied at 2x stays sharp). Fixed sizes only for now; drag-resizing (fixed ratio, the menu kept for quick jumps) to be revisited after the user has used it for a while.
  The size is saved with the plug-in's state.
- Columns (positions as in the mock-up, at 100%):
  1. x 30-290: "DUCK" label, the DUCK knob (230 px), its value; Mix, Smooth, Offset/Delay (50 px knobs) in a row under it.
  2. x 300-684: preset pill, "+ Save current" pill; the curve editor (384 x 334); "on the beat / next 1/4" labels and the editing hint under it.
  3. x 712-930: "Every" (or "Length") with 1/4, 1/8, 1/16; Shapes (13 shape buttons, 4 per row); "Trigger" with Beat, Audio, MIDI; the hit lamp and the status line.
  4. x 944-1234: the logo (220 x 107) above the full-body duck (290 x 320).
  5. x 1250-1340: the duck head (86 x 88), "DUCK METER", IN and OUT bars, the gain reduction in dB under them.
- Background: a metal panel image (the user's art when supplied; until then a generated texture), four screws.

## Controls and parameters
Every parameter is a host-automatable VST3 parameter (Live can map and automate them).

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Duck | `duck` | 0 to 100 % | 100 % | How far the curve's dips go. 0 = no ducking (bit-exact pass-through). |
| Mix | `mix` | 0 to 100 % | 100 % | Dry/wet. 0 = bit-exact pass-through. |
| Smooth | `smooth` | 0 to 50 ms | 2 ms | One-pole smoothing of the gain so a hard drop does not click. |
| Offset | `offset` | -50 to +50 % of the cycle | 0 % | Beat mode only: slides the curve against the beat grid. Hidden in Audio/MIDI. |
| Delay | `delay` | 0 to 100 ms | 0 ms | Audio/MIDI only: the curve starts this long after each hit. Hidden in Beat. Both values are kept when switching modes. |
| Rate / Length | `rate` | 1/4, 1/8, 1/16 | 1/4 | Beat: the curve repeats every 1/4, 1/8 or 1/16 note. Audio/MIDI: the length of one pass. The label reads "Every" or "Length". |
| Trigger | `trigger` | Beat, Audio, MIDI | Beat | |

Knobs: drag up/down (Shift = fine), mouse wheel, double-click resets, arrow keys when focused. The DUCK knob takes a longer drag for the full range (260 px against 180).

The curve: the same editor as Ingot's Ducker (drag a point, double-click adds or removes, Alt-drag a line to bend it, first and last points stay on the edges).
Shapes: the 13 factory shapes of the mock-up (Kick start, Gentle pump, Hard chop, Sidechain-ish, Flat, Ramp, Swell, Triangle, Gate, Double, Stutter, Soft dip, Late pump);
a shape click loads the curve only, the knobs stay.

## How it sounds (engine)
- Gain per sample: `wet = smooth(1 - duck * (1 - curve(phase)))`, then `g = (1 - mix) + mix * wet` (as in Ingot's Ducker). Both channels get the same gain.
- No latency, no look-ahead, no oversampling, nothing allocated on the audio thread. Curve table built on the message thread and swapped in (as in `DuckerDevice`).
- **Beat:** phase from the host playhead (PPQ position, tempo), `phase = frac(ppq / cycle - offset)`; it stays in step through seeks, loops and tempo changes.
  Host stopped: pass-through at full volume (the status line says "Waiting for the host to play.").
- **Audio:** the plug-in has a second stereo input bus named "Sidechain" (optional; the main sound still passes when it is not connected). Each detected hit starts
  the curve from phase 0 at that sample (+ Delay). A pass lasts the Length (in beats at the host tempo; 120 BPM if the host gives none). After one pass the curve
  holds its END value until the next hit; before the first hit the volume is full. A new hit during a pass restarts it.
  Works while the host is stopped too (if Live is feeding the sidechain).
- **Hit detection (fixed, no controls):** sidechain summed to mono, a fast envelope (about 1 ms attack, 10 ms release) against a slow one (about 100 ms); a hit is the fast
  envelope rising more than about 6 dB above the slow one and above -50 dBFS; after a hit it re-arms only once the fast envelope has fallen back near the slow one and at least
  30 ms have passed, so a kick's tail or ringing never counts twice. The exact numbers are tuned against test kicks (see Tests); they are constants, not parameters.
- **MIDI:** the plug-in accepts MIDI input. Every note-on (velocity > 0, any channel, any pitch) starts the curve at the event's sample position (+ Delay). Note-offs,
  note length, velocity, CCs and clock are ignored. Same pass/hold rules as Audio.
- **Delay** is sample-accurate: the start is scheduled that many samples after the hit, even across block boundaries.

## The status line and the lamp
Under the Trigger buttons; the lamp (Audio and MIDI only) flashes for about 90 ms on each hit.

| Mode | Normal | Nothing arriving for 2 bars while the host plays (orange) |
|---|---|---|
| Beat | "Following the host's beat." / "Waiting for the host to play." | (none) |
| Audio | "Listening to the sidechain." | "Nothing on the sidechain. Pick a source in Live's Sidechain box." |
| MIDI | "Listening to MIDI." | "No MIDI arriving. Set a MIDI track's MIDI To to this track." |

The orange line clears on the next hit. While the host is stopped it never turns orange (nothing is expected).

## Behind the curve
The sound that came in (faint cream) and went out (yellow), folded onto one pass, drawn as a waveform mirrored around a middle line, so the ducking shows as the yellow pinching in. Auto-scaled: the loudest recent part fills about 90 % of the height (a louder sound rescales at once, a quieter one over about a second). Added 2026-10-05 at the user's request.

## Meter and the duck
- DUCK METER: IN and OUT peak bars (-48 to 0 dB) with falling peak marks, and the current gain reduction in dB underneath (both channels duck together, so one figure).
- The head nods by how hard the sound is being ducked right now (after Duck, Mix and Smooth). Still when nothing is ducking.
- The full-body duck bobs with the ducking when the "Bob" pill under it is on (default on). Animation runs on the UI timer from values the audio thread publishes; it never touches audio.

## Presets
- Factory: the 13 shapes as presets (curve only, knobs at defaults).
- "+ Save current" saves the curve, Duck, Mix, Smooth, Offset, Delay and Rate under a name. The Trigger mode is NOT saved in a preset (it depends on how the track is routed).
- User presets live in `%APPDATA%\The Ducker\presets.xml` (shared by every instance and every DAW). The preset pill lists Factory then Yours; when a user preset is loaded the menu also offers Rename and Delete for it (built 2026-10-05: menu items rather than a right-click, since a host menu cannot take a right-click on an item).
- The plug-in's state (saved in the Live set) holds everything: curve, all parameters, preset name and an edited flag, window size, Body bobs.

## Project setup
- A separate project and git repo, like Progression: `D:\Claude Projects\The Ducker VST` (private GitHub repo `onesolnz/the-ducker`), JUCE 9.0.2 as a submodule, CMake,
  `juce_add_plugin` with FORMATS VST3 (Standalone too, for quick checks). Windows only. Plug-in name "The Ducker"; IS_SYNTH off, NEEDS_MIDI_INPUT on, a sidechain bus.
- The curve DSP is COPIED from Ingot (`DuckerDevice` table and curve maths, `model::evaluateAutomation`'s bend), not linked, so the plug-in builds on its own.
- Art goes in the plug-in project's `assets/` and is compiled in as JUCE BinaryData.
- Tests: a console test target with `juce::UnitTest`, as in Ingot.

## Tests
- Duck 0 and Mix 0 are bit-exact pass-through, in every mode.
- Beat: the curve lands on the beat for any block size, restarts after a seek and a loop jump, follows a tempo change; Offset shifts it by the right amount.
- MIDI: a note-on at sample N of a block starts the curve at exactly N (+ Delay); a long note and a short note give the same result; note-offs and velocity 0 do nothing;
  a note during a pass restarts it; after a pass the gain holds the curve's end value.
- Audio: a test kick at several levels (-30 to 0 dBFS) gives exactly one hit per kick, within 2 ms of its onset; a kick with a long tail and a kick plus reverb give one
  hit each; silence and low noise give none; 1/16 kicks at 180 BPM are all caught.
- Delay crosses block boundaries correctly; switching modes keeps Offset and Delay.
- Smooth: a hard drop with Smooth 2 ms has no step larger than the one-pole allows.
- State: save, load and an unknown or missing attribute falls back to the default; presets file round trip.
- No allocation on the audio thread (Debug real-time guard, as in Ingot).
- By hand in Live: sidechain from a kick track through the Sidechain box; MIDI from a MIDI track through MIDI To; automation of Duck; several instances; the status line in each case.

## Build steps (one review point each)
1. Project, empty plug-in that loads in Live with the sidechain bus and MIDI input, parameters, state; tests target.
2. Engine: curve, Beat mode, Duck/Mix/Smooth/Offset, then MIDI trigger, then Audio trigger with hit detection and Delay. Tests for each. (Checkable in Live with the generic editor.)
3. The window: layout, curve editor, shapes, knobs and pills drawn in code, rate, trigger, status line, lamp, meter, size box.
4. Art: logo, head (nod), body (bob), background; then the user's custom knobs and pills when they are ready.
5. Presets (factory and user file).

## Licences
JUCE 9.0.2 under its personal terms is fine while the plug-in is for the user's own use (see `licences.md`). The VST3 SDK comes with JUCE. If The Ducker is ever given or sold
to other people, the JUCE licence tier and the VST3 SDK licence must be checked first (and the duck art's own rights).

## Paused 2026-10-05 (the user is drawing the art)
- Built and checked in Live: engine, window, presets, trigger buttons fix, mirrored waveform, window sizes.
- Coming from the user: metal panel, knob image (rotated by code), pill art, the duck as a frame set: transparent PNGs, same canvas, duck aligned, 16 to 24 POSES from rest (frame 0) to fully ducked (last), about 450 x 500 px; the plug-in picks the frame by how much it is ducking right now (blending neighbours), not by a clock. Optional separate idle loop (time-based).
- Under consideration (not decided): drop the small head above the meter and keep only the big duck, since the head, the duck, the waveform and the meter all moving feels busy.
- 2026-10-06 sprite test (Ducker repo branch sprite-test, not merged): the user's first sheet as a 24 fps idle loop (71 frames, 244 x 454) with the coded bob on top. The user: the bob works with it; keep it on the branch until the final art.
- 2026-10-06 three loops (same branch): the user made two more sheets (tier2 = more idle motion, tier3 = fists up; 375 x 463 cells). A frame comparison showed the three sheets cannot be joined without a visible jump (their closest frames differ 5-14x a normal step), so each loop stays inside its own sheet. The user's choice: the DUCK KNOB picks the loop, switching straight away (also under automation): 0-33 % calm (tier1 0-38 every 2nd frame, ping-pong), 34-66 % moving (tier2 0-70 every 2nd, loop), 67-100 % fists up (tier3 23-69 every 2nd, ping-pong; tier3 frames 3-7 are ghosted and left out). All at 12 fps, the coded bob still on top (it hides the jump when the knob crosses a third). Full size: one sheet `assets/duck_loops.png`, 80 frames of 375 x 463, feet on y = 458, 14.6 MB (the plug-in about 22 MB, about 55 MB decoded once per process). The tier sheets stay in `assets/` as source art; the sheet was made by `tools/make-duck-loops.ps1` (Ducker repo) from the frame lists above.
- 2026-10-06 next idea (the user): drop the calm loop; moving for Duck 0-66, fists up for 67-100, with a smoother switch. Trying it first in `specs/mockup-the-ducker-loops.html` (Instant / Crossfade with a length / Closest pair: wait for the look-alike frame, then crossfade). Measured: the closest pair is only about 15 % closer than an average pair (26 against 30.5), about 8x a normal step of the moving loop, and waits up to about 2 s.
- 2026-10-06 final for now (the user, after trying the mockup `specs/mockup-the-ducker-loops.html`): TWO loops. Duck 0 % = idle (tier1 0-38 every 2nd, ping-pong); 2 % and up = fists up (tier3 23-69 every 2nd, ping-pong); 1 % keeps what is showing (dead zone, so automation around 0 does not flick). The switch is a ONE-frame crossfade (1/12 s). The moving (tier2) loop is dropped; the blending clip is on hold (the user could not get a generation they liked in Seedance) and is not needed with a one-frame fade. Sheet `assets/duck_loops.png` = 44 frames, 8.2 MB; the plug-in 15.8 MB. Built (Release) and installed; checked by tests only, the user has not yet tried it in Live.
- Parked: drag resize (revisit after use).

## Answers (the user, 2026-10-05)
1. **Body bobs:** a switch in the window: a small "Bob" pill under the full-body duck (bottom right of column 4), saved with the state, default on.
2. **Custom knob art:** one knob image that the code rotates (the pointer is part of the image). Pills: plain images (off/on) stretched by 9-slice, or drawn in code until then.
3. **Names:** for now "The Ducker" by "OneSol Productions" (as Progression), manufacturer code `Osp1`, plug-in code `Dck1`. The user may rename later.
4. **Project:** `D:\Claude Projects\The Ducker VST`, its own private repo.
5. **When:** build now.
6. **Later (backlog, not v1):** the user may draw a full frame-by-frame animation of the duck to replace the coded nod and bob. Keep the animation code in one place
   (a `DuckArt` component fed the current duck amount) so a sprite sheet can replace it without touching the rest.
