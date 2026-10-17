This iteration adds two independent, optional changes: camera-relative movement and an orbit camera. Both default to off. The experiment supports Sonic with one local viewport; other characters, split-screen, network sessions, and demo playback use the original controls. Loop sections fall back to the original movement and camera.

Run these commands from `/home/callum/gh/sonic-r-pc-recomp`:

| Command | Behaviour |
| --- | --- |
| `./run-original` | Runs an independent build of upstream revision `3da98f2e134eac82e45cd69660216fa0e4563ac5`. The first launch builds that pinned source in the gitignored `playtests/original/` directory. |
| `./run` | Builds the current source and runs with both additions off. |
| `./run --modern-controls` | Enables movement and camera together. This is the recommended first playtest. |
| `./run --modern-movement` | Enables movement alone, relative to the existing chase camera. |
| `./run --orbit-camera` | Enables the camera alone, retaining original movement. |
| `./run --modern-controls --classic-camera` | Explicitly disables the camera addition while keeping modern movement. |
| `./run --modern-controls --classic-movement` | Explicitly disables movement while keeping the orbit camera. |
| `./run --classic-controls` | Explicitly disables both additions. |

Add `--fullscreen` to any launch command. Flags are processed in order, with the last setting for each feature winning. Launchers use the local `gamedata/` directory, including its normal saves and options. These switches are not saved between launches. Their values live in `g_modernControlsOptions`, which a later in-game settings interface can edit directly. There is no in-game toggle yet.

On a mapped controller, use the left stick to run, the right stick to orbit or raise/lower the camera, and click the right stick to recenter behind Sonic. Jump and pause retain their existing button bindings. The left stick controls both desired direction and movement strength; a circular deadzone ignores the innermost 20 percent of travel. Devices without an SDL controller mapping can still use their existing digital direction input, but this experiment does not guess their right-stick axes.

Analogue movement and orbit require a mapped controller in the first detected slot. Other connected controllers can contribute through the original digital input path. Use one connected controller for this first experiment.

On the keyboard, the existing movement-direction bindings work, normally the arrow keys. With modern movement enabled, left/right request a running direction and down requests a direction toward the camera. Hold Q/E to orbit, Page Up/Page Down to raise/lower the camera, and R to recenter. Space still jumps and Enter pauses. Menus retain their original controls, including Space to confirm selections. The old separate accelerate and sharp-turn actions do not drive modern movement outside loop sections.

The movement-only combination uses the original following camera as its reference, so a sustained direction can change its world-space target as the camera turns. Use both additions for the stable-heading experiment. The camera has manual recentering in this iteration; automatic recentering is deliberately left for a separate, independently optional addition.

The first lesson in this implementation is the difference between input intent and physical movement. `platform_control_axes` reads continuous controller axes alongside the original button interface. `ControlsStick` applies the deadzone and caps magnitude. `ControlsHeading` converts the two-dimensional movement direction into the game's 4096-unit heading, relative to the camera. It computes a desired heading, not a new world position.

`ModernControlsSteer` then turns Sonic toward that heading by the shortest route. The first tuning values are 96 angle units per update on the ground, 128 at low grounded speed, and 48 in the air. A ground turn of 90 degrees therefore takes about eleven updates at the 96-unit limit. The exact movement also depends on surfaces and the existing velocity. These are initial values to evaluate, not final handling targets.

The existing `UpdateHumanPlayerPhysics` still calculates velocity, drag, character abilities, and position. Modern movement replaces its steering command and scales acceleration and the speed target by stick strength. Releasing the stick removes desired acceleration while retaining the existing drag. This keeps slopes and collision in the original lifecycle instead of implementing a second mover that teleports the character.

The camera change supplies an independent heading and height offset to the normal gameplay branch of `BuildCameraView`. The remainder of its terrain sampling, position smoothing, and view calculation still runs. Holding left therefore turns Sonic without immediately turning the camera behind him. Camera elevation is a height offset, not a separate free-camera pitch model. Wall occlusion is still limited by the original camera code; the capture shows clipping near barriers.

Legacy ghosts store button words, so they cannot reproduce the continuous movement intent. Modern Sonic movement does not load or save a Time Attack ghost or its associated best-time record, and requesting a Grand Prix race replay returns to the menu. Existing classic runs and demo playback retain the classic interpretation. An extended recording format and an explanatory replay UI are future work. Ordinary progress can still be saved because the interactive launchers share `gamedata/`.

The automated scenario now uses Time Attack on Resort Island, with an SDL virtual controller. It idles, accelerates forward, requests left, requests a partial diagonal, reverses direction, releases movement, orbits at rest, recenters, and jumps while moving. Each run uses separate writable config/save directories and symlinks to the game assets. It captures 301 race frames of position, velocity, heading, and camera position. Optional video records the same ten-second race segment without audio.

Run focused checks with `./tests/run`. To record a new native run, choose an unused name:

```sh
./tools/playtest modern-iteration-1 --video --modern-controls
./tools/playtest classic-iteration-1
./tools/playtest camera-iteration-1 --orbit-camera
./tools/playtest movement-iteration-1 --modern-movement
./tools/playtest original-iteration-1 --binary \
  playtests/original/3da98f2e134eac82e45cd69660216fa0e4563ac5/source/sdl/build-release/sonicr
```

The original binary must have been built first through `./run-original`. Run directories are never overwritten. This Linux test tool needs the SDL development libraries, `nm`, and FFmpeg. Offscreen Mesa rendering checks behaviour rather than desktop GPU performance. The original pin, unchanged player layouts, and symbol lookup make the probe usable against both executables; it is not a cross-version engine API.

The first captured runs are `playtests/runs/original-a`, `original-b`, `classic-a`, `camera-a`, `movement-a`, and `modern-a`. The two original traces match exactly. With additions disabled, the current build matches all 301 original samples exactly in this scenario. Camera-only mode preserves the original player trace. The combined mode reaches the expected headings for left, diagonal, and backward input; orbiting at rest leaves position unchanged; recentering moves the camera; and jumping produces upward velocity. These claims are checked by:

```sh
./tools/compare-playtests playtests/runs/original-a playtests/runs/original-b \
  playtests/runs/classic-a playtests/runs/camera-a playtests/runs/movement-a \
  playtests/runs/modern-a
```

These comparisons establish this scenario, not every track or ability. Manual feel, long races, looping transitions, respawns, unusual controller mappings, and wall occlusion still need broader checks. Changes to the probe's input schedule should use a new set of run directories for every comparison target.

For the first human playtest, select Sonic and Resort Island in Time Attack. Run a straight, push left without pressing accelerate, reverse direction, jump while steering, release the stick, orbit while stationary, and recenter. Report which exercise felt wrong and whether the problem was turning delay, sliding, stopping distance, or camera motion. The next iteration can then change one group of values while preserving the same baseline.
