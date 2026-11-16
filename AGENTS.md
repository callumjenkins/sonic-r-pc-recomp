Keep the original game available through `./run-original`. Its source revision is pinned in that script.

Every gameplay addition must default to off and have an independent switch. Store the switches in the shared options structure so a later in-game interface can edit the same settings.

Reuse the existing input, movement, collision, and camera lifecycle. The first modern-controls experiment supports one local Sonic player. Document any broader support before enabling it.

Build with `make -C source/sdl release -j8`. Run focused checks with `./tests/run`. Native playtest captures belong under the gitignored `playtests/` directory. Game assets belong under the gitignored `gamedata/` directory.

Architecture decisions live in `docs/decisions/`. The code map and staged test list are in `docs/modern-controls.md`.

Android uses the same SDL2 engine and an OpenGL ES platform path. Build with `./tools/build-android`; add `--abi arm64-v8a --abi x86_64` for a phone/emulator APK. Run `./tests/android-import` for asset import changes. See `docs/android.md` for the port test list. Keep dependencies, APKs, and data ZIPs under ignored `android/.deps/` and `android/build/`.

Run `./tests/android-touch` for the optional 3D touch layout. Keep its preference independent of the two gameplay switches and keep the digital Saturn pad available. Continuous touch input belongs in the existing platform axes reader. See `docs/android-touch.md`.

The Android game menu reuses launcher settings in the launcher process. Keep preference access there and return overlay choices to the game through an activity result. Menu must remain reachable while touch controls are hidden. Run `./tests/android-menu --serial emulator-PORT` on a test emulator with imported game data. See `docs/android-menu.md`.

Android graphics default to the existing Native display path. Resolution and texture smoothing use the shared settings view and menu result, with GPU changes on the engine thread at `BeginFrame`. Run `./tests/android-graphics` for offscreen target, presentation, fallback, and filtering checks. See `docs/android-graphics.md`.

Run `./tests/audio-files` for audio stream loading/playback and `./tests/save-data` for save directories, file failures, and the save menu. Android audio files must open through the engine's stdio paths before becoming SDL streams. Keep save formats compatible and checkpoint options on the engine thread before Android suspends. See `docs/android-audio-saves.md`.

Custom CPU rosters default off and use the existing offline Grand Prix setup with five racers total. Apply choices before player initialization and validate against the loaded unlock table. Preserve the stock roster for disabling the mode between races. Clone colours are independent and apply during regenerated character lighting. Run `./tests/cpu-opponents`; see `docs/cpu-opponents.md`.

Wider Resort Island defaults off and widens the loaded course at `InitLevel`, after the terrain header is parsed. Keep the stock collision grid and map lookups through `WideIslandGridX`. Never add or reorder track objects, because triggers and animations use fixed object indices. Ghosts and replays stay off while a widened course is in play. Run `./tests/wide-island` for the layout, `./tests/wide-island-ground` for collision across the strip, and `./tests/wide-island-race NAME` for a full Grand Prix; see `docs/wide-island.md`.

HD textures default off and replace whole texture pages at load, in the GPU copy only. Collision reads the stock 16-bit ground pages, so never write replacement pixels into `g_tpagePixelBuf`. Code that patches a page in place drops its replacement first. Packs are named folders inside `HD`, derive from game data, and stay in gitignored folders: `gamedata/HD` or under `playtests/`. `tools/hd-upscale` makes them with upscaling models on the GPU. Run `./tests/hd-textures`, `./tests/hd-textures-tools`, and `./tests/hd-textures-shots`; see `docs/hd-textures.md`.

The Android launcher and game menu share the paged `ControlSettingsView`, with a sidebar from 600 dp wide and a row of page buttons below that. New options go on an existing page or a new one, never in a second settings screen.

