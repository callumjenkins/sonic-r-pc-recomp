# Wider Resort Island

Callum requested: "How might we expand Resort Island? As a first POC, could we take the middle portion of the map, make a slice, and then open up a plain flat space bridging the two slices? We should make our Android launcher UI a little more elaborate to handle these extra cases - let's give it pages and a sidebar, with this option going on to an Experiments page". Source: project conversation, 2026-10-20.

The experiment cuts Resort Island on the playfield column boundary at X −16 and moves everything east of it by 8, 16, or 32 tiles. One tile is 216.45 world units, so 16 tiles is 3463 units. At about 49 units a metre, that is roughly 70 m. Roads, walls, and collision faces that cross the cut stretch over the strip. The flat ground repeats the column west of the cut, so the sea stays sea and the lagoon, grass, and beach widen. It defaults off. Run it on Linux with `--wide-island=16`, or turn on **Wider Resort Island** on the Android Experiments page. The [decision record](decisions/2026-10-20-wide-resort-island.md) explains the approach.

## Code map

| Data | Where it loads | What the experiment does |
|---|---|---|
| Track parts and decorations, `ISLAND_E.BIN` | `LoadTrack3`, then `InitTrackObjectModes` | Stretches parts per vertex and grows their cull radius; moves narrow decorations whole; moves rotating decorations by pivot |
| Rings | `LoadTrack3` sub-entries | Moves each ring east of the cut |
| Collision mesh, `ISLAND.TER` | `LoadTerrain`, `ParseTerrainHeader` | Moves vertices, wall edges, and loop rows; re-centres surfaces and walls that move or stretch |
| Collision faces that cross the strip | Same file | Get normals from their stretched corners, and `PointInPolygon` tests their surfaces face by face as two triangles |
| Collision grid | Same file | Unchanged. The 7 cell lookups map X through `WideIslandGridX`, and the +X clamps grow |
| AI waypoints, `AISTUFFI.BIN` | `LoadAI` | Moves both X fields of each waypoint |
| Lap line, race-position spline, spawns, flyover nodes | `LoadTrack3` position sections | Moves each point east of the cut |
| Flat ground, `ISLAND.MAP` | `InitIsland` | Inserts copies of column 54 and widens `g_worldBoundsD`; `g_playfieldColumns` replaces the fixed 128 in the renderer and `IsOnTrackSurface` |
| Balloons and battle items, weather spawns | C tables in `track_anim_init.c` and `weather.c` | Move as they are copied |
| Minimap | `DrawMinimapWidget` | Maps X back to the stock map art |

The intro fly-by and the chase cameras are relative to the player and need nothing. Section 7 of the BIN holds relative deltas and stays as it is.

## Test list

| # | Unit | Case | Expected | Level |
|---|---|---|---|---|
| 1 | Layout | The cut sits on the column boundary nearest X = 0 | Column 55, cut −16, width 3463, 144 columns at 16 tiles | Unit, `tests/wide-island` |
| 2 | Layout | Shift moves only points east of the cut, and unshift inverts it | −16 stays, −15 moves; strip positions map to −16 | Unit |
| 3 | Layout | Off and invalid counts leave the course alone | Every count except 8, 16, and 32 gives the stock layout | Unit |
| 4 | Layout | Widened bounds keep the tile size, and 32 tiles stay inside int16 | Under 0.01 units of drift; 16460 for the furthest terrain vertex | Unit |
| 5 | Layout | Strip columns repeat the column west of the cut | Columns 55 to 70 equal column 54 in every row | Unit |
| 6 | Status | The five status texts for off, pending, active, changed, and turned off | Exact strings | Unit |
| 7 | Ground | Five points inside every collision face pass `PointInPolygon` for its surface | No misses at 0, 16, and 32 tiles; the stock run is the control | Linux engine probe, `tests/wide-island-ground` |
| 8 | Stock | With the switch off, the scripted Time Attack trace equals the build before the change | Byte for byte | Linux playtest, `tools/playtest` |
| 9 | Race | A Grand Prix at 16 and 32 tiles: every CPU crosses the strip and finishes three laps | Four CPU finishers | Linux playtest, `tests/wide-island-race` |
| 10 | Android | Experiments page, pending status, and the status inside a widened race | Status text on the Game page | Emulator, `tests/android-menu` |

## Results

Checked on 2026-10-20 on Linux and an Android 15 emulator.

- The unit tests pass, and so does the ground probe at 0, 16, and 32 tiles. Ten collision surfaces cross the strip, and every sampled point on all 561 surfaces finds its own surface.
- With the switch off, the scripted Time Attack trace is byte for byte the same as the build before the change.
- In a Grand Prix, all four CPUs crossed the strip and finished three laps at 0, 16, and 32 tiles. A CPU lap took about 1300 frames on the stock course, about 1400 at 16 tiles, and about 1700 at 32. The probe's own driver for player 0 does not finish the race even on the stock course, so the check counts the CPUs only.
- On the emulator, the Experiments page, the status lines on the Game page, and a 32-tile Grand Prix all work. The launcher switches between the row of page buttons and the sidebar when the display changes width.

A test-gaps audit by a fresh agent found the collision holes and stale face normals that the per-face test and recomputed normals now handle, and a lost unlock when Replay was chosen after a blocked race. Its report is in the knowledge base under `pr-test-gaps/mos/sonic-r-wide-island/`.

## Known limits

- Textures stretch across the strip.
- The AI's per-cell ground codes at the cut repeat across the strip, including checkpoint and wall-steering cells.
- Rain spawns over the stock area, so some rain falls inside the strip.
- Nothing yet tests the guards that keep the course stock on other tracks, in demos, and in network races, or the ghost and replay gates. They were checked by reading the code.
- The Dreamcast grid renderer still assumes 128 columns. The Dreamcast build has no way to turn the experiment on.
- A phone check on the Fold is still to do.
