Modern controls investigation, started 2026-10-17 against upstream source `3da98f2`.

Callum asked for controls and camera behaviour closer to a modern third-person game, with an explanation at each stage. This document records the initial investigation and proposed sequence of experiments. The first implementation is now described in [Controls iteration 1](controls-iteration-1.md).

The engine is editable C compiled into a native executable. The original game files supply tracks, models, textures, and audio. Changing the C and rebuilding changes the running engine; these experiments do not require patching the original Windows executable.

The source still reflects the layout of the original executable. Names such as `0x00485740` in comments identify original code addresses, and offsets in `player_struct.h` identify fields in the reconstructed player layout. These help explain how the author recovered the game. They do not mean we must keep the recovered behaviour forever.

The README describes the architecture as "sharing one game-logic codebase across all targets." See `README.md:6`. The Dreamcast Makefile reuses the C in `source/sdl/src` and replaces platform, rendering, and sound implementations. Consequently, changing physics in that directory can affect other builds too.

The existing input-to-render path is:

| Step | Existing implementation | What it does |
| --- | --- | --- |
| Read devices | `source/sdl/src/platform/platform_sdl.c`, `platform_poll_events` and `platform_poll_gamepads` | Reads SDL events and controllers. At lines 740–767 the left stick is reduced to digital direction bits. |
| Assign input | `source/sdl/src/input/input.c`, `UpdatePerPlayerInput` | Assigns devices to players, handles pause/countdown and mirror mode, and records or plays input streams. |
| Move a human player | `source/sdl/src/player_camera.c`, `UpdateHumanPlayerPhysics` | Applies character abilities, steering, acceleration, drag, and velocity. Despite its filename, this is the main human-player movement function. |
| Update the world | `source/sdl/src/main.c`, race loop | Runs movement alongside the existing terrain, collision, animation, and race logic. |
| Position the view | `source/sdl/src/render_scene.c`, `BuildCameraView` | Places the gameplay camera using the player's smoothed heading, samples terrain, smooths position, and computes the view. |

The tank controls are visible directly in `player_camera.c:695–721`: left subtracts `turnRate` from `angleYaw`, and right adds it. Acceleration at lines 800–807 acts along the character's forward direction. At lines 746–752, `moveMode` follows the character's yaw; `BuildCameraView` then uses `moveMode` to place the camera behind the character. The `BuildChaseCamera` function in `camera.c` is not the main gameplay camera path used by this build.

`Player.angleYaw` uses 4096 angle units per full revolution. Position and velocity use fixed-point integers with 12 fractional bits. For example, the movement function reconstructs world velocity using sine and cosine tables, then adds it to position at lines 864–874. We should respect those units when introducing direction calculations. The simulation is capped at about 30 updates per second by `WaitForFrameCap` in `screens.c`; camera smoothing and turn rates must be judged at that cadence.

The proposed control model is a movement vector relative to the camera's horizontal forward and right directions. Pushing left means run toward the left of the screen; pushing backward means turn and run toward the viewer. Stick magnitude supplies movement strength. A separate camera heading lets the right stick orbit the character. A deadzone prevents stick noise from moving either one, and diagonal keyboard movement should have the same maximum strength as movement along one axis.

The closest existing implementation for device access is the SDL platform layer. Extend that layer to expose analogue movement and look axes while preserving its existing button output for menus and classic controls. The closest movement implementation is `UpdateHumanPlayerPhysics`; adapt its steering and acceleration inputs inside the existing movement lifecycle. The closest camera implementation is `BuildCameraView`; give its normal gameplay positioning an independent heading while retaining the existing viewport, terrain, and view-matrix path. Avoid a second player controller that writes positions separately from the existing movement code.

Camera-relative movement and automatic camera following need designing together. If holding right turns the character and the camera immediately rotates behind that character, the requested world direction keeps rotating too. The first prototype should use a stable camera heading during directional movement, with explicit orbit control and a recenter action. Delayed automatic recentering can be tuned afterwards. This is a proposed experiment, not an accepted final camera design.

The repository's desktop documentation covers launching, controls, Time Attack ghosts, and saves. No automated test suite, scripted-input runner, or capture workflow was found in the tracked source and documentation. The README describes validation against the original executable and human playtesting. `screens.c` refers to `state_machine.md`, but that document is absent from this checkout.

A temporary native test probe has demonstrated the following without modifying game source:

- SDL's offscreen video driver renders the game using Mesa software rendering, with the dummy audio driver.
- An `LD_PRELOAD` library supplies key-down and key-up events through `SDL_PollEvent` and captures rendered frames before `SDL_GL_SwapWindow`.
- A script navigates the existing menus into a Grand Prix race as Sonic on Resort Island, then accelerates, steers left, releases movement, and jumps.
- The probe reads position, heading, and countdown from the current executable's symbols and writes a CSV trace. Those symbol addresses are specific to the executable and must be regenerated after a rebuild.
- The test uses its own writable config, save, and ghost directories under `/tmp/sonic-r-controls-baseline/data`. Asset directories are symlinks to the local game data.

The preserved original executable, probe source, screenshots, and trace are under the gitignored `playtests/baseline/` directory. The executable's SHA-256 is `bafc871512f9e68f2870ef8e885169c077e0162228964626519dc658ddf5f1e5`. The probe is an exploratory local tool. It has not established deterministic replay, controller injection, or camera telemetry yet. Grand Prix opponents and time-based state make a Time Attack scenario preferable for repeatable comparisons.

For a visible comparison, launch the preserved executable from the repository root:

```sh
./playtests/baseline/sonicr "$PWD/gamedata"
```

That command uses the normal writable game data. Automated experiments should continue using their separate writable directories. The offscreen command used during this investigation was:

```sh
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 \
  LD_PRELOAD=/tmp/sonic-r-controls-baseline/probe.so \
  ./playtests/baseline/sonicr /tmp/sonic-r-controls-baseline/data
```

The probe currently stops after its short race sequence. It is Linux-specific, and software rendering is useful for correctness checks rather than performance measurement. Captures are silent. Its sampled presentation-frame count must not be treated as a universal simulation tick across all screens.

The baseline screenshots also show the camera getting very close to Sonic and clipping through geometry after the scripted turn reaches a barrier. Existing terrain-height sampling does not establish that an orbit camera will avoid walls. Geometry occlusion needs a separate investigation before calling the new camera finished.

The recommended iteration sequence is:

1. Establish a repeatable Time Attack start, input schedule, and capture. Record movement intent, position, velocity, character heading, and camera heading. Verify a repeated original run before claiming deterministic comparisons. Explain the game loop and its fixed-point units at this stage.
2. Add an opt-in modern control mode for a single local Sonic player, using the existing movement path and an independent camera heading. Keep classic mode available for comparison. Explain how screen directions become world directions and then a desired character heading.
3. Add or tune manual orbit, recentering, turn responsiveness, and release behaviour. Change one group of values per build. Explain how camera smoothing differs from movement inertia.
4. Extend checks to the other characters, slopes, water, jumps, loops, respawns, and additional players. Decide explicitly how cars such as Amy's and Eggman's should feel. Explain the character-specific branches encountered along the way.
5. Settle recording and network compatibility before enabling the mode for those uses. Explain why a recording of buttons depends on the simulation that interprets them.

At each playable iteration, compare the same short exercises: set off from rest, run a straight, make a 90-degree turn, reverse direction, jump while steering, release movement, orbit while standing still, and orbit while moving. Feedback should identify the exercise and describe the result, for example "the character turns promptly, but slides sideways after the turn" or "the camera recenters while I am still looking around." Video helps relate that report to the inputs and measured movement.

Ghost recording is a compatibility constraint. `input/input.c:460` writes `g_perPlayerInput[0]` into the ghost stream, which stores the original button word rather than analogue axes or world-space intent. Existing ghosts cannot be assumed to reproduce their route after physics changes. Network play also has its own input assignment path: `main.c:1355–1356` skips `UpdatePerPlayerInput` for a network session. A modern implementation that works only inside local input assignment would therefore not automatically support network play. The first prototype should explicitly limit itself to the supported local scenario while these formats remain undecided.

The following test list is proposed before implementation. Numerical expectations for turn speed, acceleration, and recenter delay should be written down when those tuning values are chosen.

| Unit | Case, as the test name | Expected | Level |
| --- | --- | --- | --- |
| Direction conversion | Camera rotation rotates the requested movement direction | At camera headings 0, 90, 180, and 270 degrees, screen-forward maps to the corresponding world-forward vector. | Pure C unit |
| Direction conversion | Camera pitch does not add vertical movement | The movement vector has zero vertical component. | Pure C unit |
| Direction conversion | Full diagonal input has unit maximum strength | The resulting magnitude is at most 1, including both keyboard diagonals and stick corners. | Pure C unit |
| Stick processing | Input inside the deadzone produces no movement | Centre and values inside the boundary produce zero intent; values outside it produce nonzero intent. | Pure C unit |
| Stick processing | Partial stick tilt produces partial movement strength | Half-strength processed intent is weaker than full-strength intent without changing its direction. | Pure C unit |
| Heading | Turning across the angle wrap takes the short route | A change from 4090 to 6 requests +12 units, and the reverse requests -12. | Pure C unit |
| Movement | Holding screen-left does not make the movement target rotate with the character | With a fixed camera, desired world direction remains constant while the character turns. | Native integration with telemetry |
| Movement | Opposite input turns toward the requested direction | The desired heading reverses by 180 degrees using a defined tie rule; velocity remains governed by the existing movement integration. | Native integration with telemetry |
| Movement | Releasing input clears movement intent | No residual desired acceleration remains; stopping distance follows the selected drag behaviour. | Native integration with telemetry |
| Camera | Orbiting at rest leaves player position unchanged | Camera heading changes while the stationary player's position remains unchanged on flat ground. | Native integration with telemetry |
| Camera | Recentering follows the selected delay and explicit recenter action | Measured heading follows the documented timing and respects current look input. | Native integration with telemetry |
| Lifecycle | Pause, countdown, restart, and respawn reset or suppress modern input | No movement during blocked states and no stale look or movement intent after transitions. | Native integration |
| Movement | Slopes, water, loops, and airborne abilities retain their intended behaviour | The existing surface and character-ability branches still execute, with camera-relative intent applied only in the states the prototype supports. | Native scenarios and human playtest |
| Camera | Mirror mode and separate viewports use the intended movement basis | Screen-left remains screen-left in mirror mode, and each local player uses that player's own camera heading. | Native integration |
| Classic mode | Disabling modern controls preserves the original input and movement path | The same fixed scenario matches the preserved baseline within an established repeatability tolerance. | Native integration |
| Recording | Legacy ghost playback uses the intended control rules | Classic recordings continue through classic interpretation; modern playback is rejected or explicitly supported by a defined format. | Native integration |
| Device assignment | Keyboard and controller intent remain assigned to the intended player | Disconnecting a pad clears its axes, and another player's inputs do not move this player. | SDL integration |
| Presentation | Turning, jumping, and orbiting remain readable in motion | No persistent character occlusion, abrupt camera flip, or unexpected orbit in the comparison clip. | Capture review and human playtest |

The accepted requirement to preserve the original version and keep additions independently optional is recorded in [the controls decision](decisions/2026-10-18-opt-in-controls.md). Recording-format extensions remain undecided. The first iteration keeps modern movement out of legacy ghost saving and race replay.
