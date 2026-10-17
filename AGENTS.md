Keep the original game available through `./run-original`. Its source revision is pinned in that script.

Every gameplay addition must default to off and have an independent switch. Store the switches in the shared options structure so a later in-game interface can edit the same settings.

Reuse the existing input, movement, collision, and camera lifecycle. The first modern-controls experiment supports one local Sonic player. Document any broader support before enabling it.

Build with `make -C source/sdl release -j8`. Run focused checks with `./tests/run`. Native playtest captures belong under the gitignored `playtests/` directory. Game assets belong under the gitignored `gamedata/` directory.

Architecture decisions live in `docs/decisions/`. The code map and staged test list are in `docs/modern-controls.md`.
