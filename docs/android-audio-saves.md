# Android sound and save investigation

Callum reported: "On Android, the game is missing many sound effects. It also does not seem to be saving data." He also reported that manual saving through the original Linux game's save/load menu appeared to do nothing. Source: project conversation, 2026-10-18.

The existing save menu writes ten numbered files through `SaveConfigSlot`. In the baseline, `InitSaveData` was empty, so a clean installation never created their parent folder. The menu discarded the write result. Options use a separate `SONICR.INF` file, and time-attack ghosts use their own `GHOST` directory tree. These paths need separate checks.

## Test list

| Unit | Case, as the test name | Expected | Level |
|---|---|---|---|
| Save initialization | Fresh data creates writable save and ghost folders | A numbered slot and a per-mode, per-track ghost can be written | Native integration |
| Save initialization | Existing saves survive initialization | Existing bytes remain unchanged | Native integration |
| Numbered slots | Save and load round trip | All 554 words survive writing, reopening, and loading | Native integration |
| Numbered slots | Failed write leaves the cached slot unchanged | Failure result and previous cached data | Native integration |
| Numbered slots | Truncated slot is unavailable | No partial save is offered for loading | Native integration |
| Options | Settings survive background and process restart | Changed settings return on next launch | Android runtime |
| Sound loading | All mapped effects load from imported assets | Every mapped slot is available | Native and Android runtime |
| Sound playback | Plain, pitched, and looping voices produce audio | Nonzero PCM for each playback path | Native integration |
| Android lifecycle | Background and resume preserve sound playback | Effects play after resume | Android runtime |
| Save menu | Explicit save creates a slot and load restores it | Visible occupied slot and persisted data | Desktop and Android runtime |

SDL 2.32.10's Android `SDL_RWFromFile` resolves relative paths against Android's internal storage root, then tries APK assets. It does not use the engine's changed working directory. `Mix_LoadWAV` therefore searches beside `gamedata` instead of inside it. Music formats decoded through ordinary C files already use the correct directory. Standard WAV/OGG/MP3/FLAC music needs the same stream-opening correction as effects.

Additional cases: verify that audio loading bypasses Android's filename-based SDL resolver; missing clips remain unavailable; invalid slot indices fail without indexing outside the table; short reads retain default settings; failed saves keep the menu open; settings load after a process restart; slot initialization creates all four ghost-mode directory trees.

## How this part of the recomp works

The translated game still decides when to play effects and which save slot to use. SDL replaces the Windows audio device. Ordinary files replace the original PC file calls. Android's launcher installs the assets into a private writable directory, then the same C engine uses that directory as its working directory. Fixes should follow these existing paths.

## Implemented fixes

Audio loaders now open through the engine's C file path and pass the handle to SDL with `SDL_RWFromFP`. SDL_mixer owns and closes the successful stream. Allocation failures close the handle immediately, and missing or undecodable effects log a warning. Standard WAV/OGG/MP3/FLAC music uses the same approach. Existing ADX, SON, and ADP decoders stay in place.

The existing `InitSaveData` hook creates `SAVE` and all four mode/five track ghost-directory combinations. A complete slot still contains 554 little-endian words. Short reads are rejected. Options and numbered slots are written to sibling temporary files and replaced after successful write and close on ordinary filesystems. Failed writes preserve the previous disk and cached slot. The menu stays open, displays a storage error, and permits retry. The Dreamcast VMU retains its direct shim.

Android checkpoints options through `SDL_APP_WILLENTERBACKGROUND` on the engine thread. Demos temporarily change effects volume and difficulty, so the engine saves the chosen settings before entering title demos or race replays and skips settings writes during playback. Both title-demo exit paths restore effects volume. This also protects normal desktop exit during a demo.

These repairs do not add progress autosaving. Save an unlock or race-table change explicitly, then load that numbered slot after restarting the game. `SONICR.INF` contains options; ghost files contain time-attack recordings and their times. The local desktop `gamedata` now has the missing empty directory tree, so the pinned original can also write its existing save paths. Its source and launcher remain unchanged.

## Verification

`./tests/audio-files` reproduces the Android filename mismatch by refusing filename-based SDL reads while exercising real C file reads, SDL decoders, and the mixer. It checks all 42 mapped effects with generated PCM clips, missing/corrupt files, stream allocation failure and ownership, WAV music playback with fallback from a corrupt preferred format, and plain, pitched, and looping effects.

`./tests/save-data` exercises fresh and existing directories, all ghost-mode/track paths, disk/cache round trips, invalid slot indices, short writes, close failures, open/rename failures, temporary-file cleanup, incomplete options/slots at the one-byte boundary, trailing bytes, and demo-option protection. Its second binary invokes the actual save screen with rendering/timing stubs and real file operations, checks that a failed write displays an error and keeps the screen open, then verifies a successful retry.

A fresh test-gap audit also ran physical buffered close and short-write failures through `/dev/full`, loaded the 42 real imported effects, streamed WAV/OGG/MP3/FLAC music, checked stream closure over repeated reloads, and invoked the actual Android-compiled background handler. Those probes passed. The maintained menu test, boundary cases, and WAV playback/fallback assertions were added after that audit. The report is copied under ignored `playtests/android-audio-saves/test-gaps-audit.md`.

Android runtime checks used an isolated x86-64 emulator with the real APK. Explicit saving created `SAVE/R01.SAV`, exactly 2,216 bytes. A fresh process loaded a marked slot and saved it into `R02.SAV`; all 554 words matched. The effects slider was changed from four to six through the game UI, then backgrounded and terminated; `SONICR.INF` retained six.

With music muted as a test fixture, the emulator's authenticated local audio stream recorded effects during menus, a Resort Island race, and input after background/resume. The 62.5-second PCM recording has nonzero audio during the effects and silence between them. The recording and screenshots are under ignored `playtests/android-audio-saves/`. This establishes playback in the emulator; phone speaker volume, Bluetooth routing, and ARM64 execution still need a device playtest.

The Android ADR was updated with these persistence and file-stream constraints. The fresh prose/comment check found a stale source-location comment and baseline findings written in present tense; both were corrected. No TypeScript or repository formatter applies. The sound and save repairs reuse the existing engine and menu paths, and the original controls switches remain independent.

The final Android attract-demo check reached Resort Island with the replay camera, then backgrounded and terminated the process. The saved options retained difficulty two, effects volume seven, and music volume eight. `demo-background.png` and `settings-after-demo.inf` record that check. APK signature and 16 KB ZIP alignment checks passed; both ARM64 and x86-64 libraries are present, with no bundled game assets.
