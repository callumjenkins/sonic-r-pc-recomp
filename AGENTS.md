Keep the original game available through `./run-original`. Its source revision is pinned in that script.

Every gameplay addition must default to off and have an independent switch. Store the switches in the shared options structure so a later in-game interface can edit the same settings.

Reuse the existing input, movement, collision, and camera lifecycle. The first modern-controls experiment supports one local Sonic player. Document any broader support before enabling it.

Build with `make -C source/sdl release -j8`. Run focused checks with `./tests/run`. Native playtest captures belong under the gitignored `playtests/` directory. Game assets belong under the gitignored `gamedata/` directory.

Architecture decisions live in `docs/decisions/`. The code map and staged test list are in `docs/modern-controls.md`.

Custom CPU rosters default off and use the existing offline Grand Prix setup with five racers total. Apply choices before player initialization and validate against the loaded unlock table. Preserve the stock roster for disabling the mode between races. Clone colours are independent and apply during regenerated character lighting. Run `./tests/cpu-opponents`; see `docs/cpu-opponents.md`.

Wider Resort Island defaults off and widens the loaded course at `InitLevel`, after the terrain header is parsed. Keep the stock collision grid and map lookups through `WideIslandGridX`. Never add or reorder track objects, because triggers and animations use fixed object indices. Ghosts and replays stay off while a widened course is in play. Run `./tests/wide-island` for the layout, `./tests/wide-island-ground` for collision across the strip, and `./tests/wide-island-race NAME` for a full Grand Prix; see `docs/wide-island.md`.

HD textures default off and replace whole texture pages at load, in the GPU copy only. Collision reads the stock 16-bit ground pages, so never write replacement pixels into `g_tpagePixelBuf`. Code that patches a page in place drops its replacement first. Packs are named folders inside `HD`, derive from game data, and stay in gitignored folders: `gamedata/HD` or under `playtests/`. `tools/hd-upscale` makes them with upscaling models on the GPU. Run `./tests/hd-textures`, `./tests/hd-textures-tools`, and `./tests/hd-textures-shots`; see `docs/hd-textures.md`.
