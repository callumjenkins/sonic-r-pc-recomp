The Android build uses the same C engine as the desktop version. The first local APK contains ARM64 and x86-64 libraries, for a phone and emulator respectively. It requires Android 8.0 or later. Dependencies and generated files remain under ignored directories.

Copy `android/build/sonic-r.apk` and `android/build/sonic-r-data.zip` to the phone. Install the APK, open Sonic R, choose **Import game data**, and select the ZIP. The import can take a little time. Once it finishes, **Play** opens the game. The ZIP includes the local track, character, texture, sound, and music files; the APK contains the engine and launcher.

The two launcher checkboxes select camera-relative movement and the orbit camera independently. Both start off and the launcher remembers each selection. Their arguments use the same parser and options structure as the desktop switches. The original local-Sonic and recording restrictions described in [Controls iteration 1](controls-iteration-1.md) still apply.

The APK's launcher icon uses newly generated artwork. Its source, prompts, and the original-asset permission research are linked from [the icon notes](android-icon.md).

The launcher also has an independent **Saturn 3D Pad layout** choice, initially off and remembered between launches. It adds an analogue movement stick, a smaller D-pad, six face buttons, shoulder buttons, Start, and a camera stick. It works with either classic or modern movement. Enable **Camera-relative movement** for continuous heading and running strength, and **Orbit camera** to use the camera stick. Choosing the layout does not enable either gameplay option. Leave the layout unchecked to use the digital Saturn pad adapted from Bomberman, with a D-pad, curved A/B/C and X/Y/Z rows, shoulders, and Start.

Both touch layouts use the default keyboard bindings for buttons and menu directions. Changing those bindings also changes the touch actions. The 3D Pad's continuous axes enter the same platform input reader as a mapped physical controller. Its A button is Jump/OK, B is Run/Back, C is look back, X/Z orbit left/right, Y recenters, L/R drift in classic mode, and Start pauses. Hold L and R together to brake in classic mode. Modern movement ignores those two drift inputs. Fingers can hold movement, camera, and actions together. Cancellation, focus loss, pause, and resizing clear touch input. See [the touch layout and test list](android-touch.md).

During play, tap the **three-bar menu icon** in the top-right corner or use Android Back, controller Select, or a keyboard Menu key. The game pauses while the Android menu is open. **Show touch controls** hides or restores either overlay when you resume, and **Saturn 3D Pad layout** switches layouts in the same session. The Menu button stays visible when controls are hidden. Movement and camera changes in this menu apply on the next game launch. **Resume game** or Back returns to play; **Return to launcher** closes the current session. Settings persist across launches. See [the menu implementation and checks](android-menu.md).

**Internal render resolution** offers Native display, 480p, 720p, 1080p, 1440p, and 2160p. Native already draws polygons at screen resolution. The 2160p choice renders a 2880×2160 scene at the original 4:3 aspect ratio, then scales it to the screen. **Smooth textures** independently enables bilinear filtering, including HUD sprites. Both graphics choices apply on resume and persist between launches; Native and game texture filtering are the defaults. The menu reports the active render dimensions and any fallback. For a Fold 7, start with Native and compare 2160p for supersampling or 1440p for lower GPU work. See [graphics options, limitations, and tests](android-graphics.md).

Options are saved when the game backgrounds and on normal exit. Progress uses the original ten numbered slots: open **Options → Load/Save Data**, move left from **Load** to **Save**, choose a slot, and press A/OK. Overwriting an occupied slot asks for confirmation, initially **No**; select **Yes** to proceed. On a fresh launch, use **Load** on that slot to restore unlocks and race-table progress. Time-attack ghosts use separate files. Modern movement still disables legacy ghost saving, as documented in the controls restrictions. A failed slot write keeps the menu open with a storage error.

The [sound and save investigation](android-audio-saves.md) explains the Android audio-path fix, save-folder initialization, and verification. Run `./tests/audio-files` and `./tests/save-data` for their regression checks. Updating the APK keeps installed game data and saves; importing the ZIP again is unnecessary for these fixes.


The closest existing Android implementation was `pc-saturnbomberman`'s playtest launcher. Its `saturn-recomp/playtest/README.md:97–98` says, "The game runs in SDL's activity in a process of its own, as `libmain.so`." This project follows that separation with SDL2, Java activities, and local ZIP import. It does not depend on Bomberman's generated code, SDL3 runtime, or playtest service. SDL documents this integration in its [Android guide](https://wiki.libsdl.org/SDL2/README-android).

The renderer already transforms and clips vertices on the CPU before calling `R_DrawTriFan`. Android's OpenGL ES 2.0 path submits those same vertices through a shader in the existing backend. The shader applies the original screen projection, texture multiplication or signed addition, and alpha test. Texture uploads, blending, depth tests, and the render-state tracker remain in use. `SONICR_GLES` selects this platform path at build time. Movement, collision, and camera code compile from the same files on both platforms.

Build on this Linux machine with:

```sh
./tools/build-android
./tools/package-android-data
```

The first command builds ARM64 by default. Add `--abi arm64-v8a --abi x86_64` to include the emulator architecture in the same APK. It uses Android SDK platform 36, NDK `30.0.16248370`, Java 17 source compatibility, Gradle `9.7.1`, and Android Gradle Plugin `9.4.1`. Set `ANDROID_HOME` to choose a different SDK directory and `JAVA_HOME` to use an installed JDK. If no compiler is available, the script can download a checksum-verified Temurin JDK 21 into its dependency cache.

SDL2 `release-2.32.10`, SDL_mixer `release-2.8.0`, and miniupnpc `miniupnpc_2_3_3` are pinned to commit identities in the build tool. The SDL Java sources come from the same checkout as its native library. The APK uses development signing; future local builds use the same Android debug key on this machine. No release has been uploaded.

Run the existing controls checks with `./tests/run` and the ZIP-import checks with `./tests/android-import`. The Android checks use Java directly and do not need an emulator. The native APK and import UI also need a runtime smoke test; a successful cross-compile alone cannot establish that the renderer works on Android.

On a Linux machine with GLES2 development libraries, the Android shader can also run through the native playtest tool:

```sh
make -C source/sdl release -j8 RELEASE_BUILDDIR=../../playtests/build-gles GL_CFLAGS=-DSONICR_GLES GL_LIBS=-lGLESv2
./tools/playtest --binary playtests/build-gles/sonicr gles-check
```

The test list for this port is:

| Unit | Case | Expected | Level |
| --- | --- | --- | --- |
| ZIP import | Valid mixed-case game folders and music | Asset names become uppercase, music filenames become lowercase, and the required folders are ready. | Java unit |
| ZIP import | Parent traversal, absolute paths, and Windows separators | Reject unsafe paths and preserve installed game data. | Java unit |
| ZIP import | Duplicate paths after case normalization | Reject the archive. | Java unit |
| ZIP import | Missing required asset folder | Reject the archive before replacing the old data. | Java unit |
| ZIP import | Re-import over existing saves and options | Replace assets while preserving save/config contents. | Java unit |
| ZIP import | Too many entries | Reject the archive and retain the old save. | Java unit |
| APK | Package native libraries and SDL Java code | Signed installable APK with the selected ABIs and an exported SDL entry point. | Build and package inspection |
| Android launcher | No data, then document-picker import | Play starts disabled, import succeeds, and Play becomes enabled. | Emulator |
| Android launcher | Independent feature choices | Both default off, persist independently, and pass the selected engine flags. | Emulator |
| Renderer and input | Launch and navigate into a race | Menus, track, characters, and touch input work through the normal engine. | Emulator |
| Renderer | Text, sparkle, and particle transparency | Colour-keyed pixels reveal the scene underneath them. | Emulator and native GLES capture |
| Desktop | Rebuild without the Android platform define | Existing controls tests pass and the fixed native scenario matches its original trace. | Native integration |

Physical ARM64 execution, Bluetooth-controller behaviour, audio quality, suspend/resume under memory pressure, and Fold display transitions require checks on a phone. These are limits of emulator verification rather than claims established by producing the APK. The ZIP importer caps accepted extracted game data at 1 GB; the automated fixture checks the entry-count cap without writing a gigabyte of test data.

The initial local checks on 2026-10-18 passed both test suites, APK signature verification, and 16 KB ZIP and native-library alignment checks. The APK contained both selected architectures and no game assets. A fresh Android 15 emulator imported the 501-file ZIP through the document picker. Both controls started off. With them enabled, the game reached Resort Island, and touch movement and orbit input worked during a race. At that stage Android Back closed the native process and returned to the launcher; it now opens the game menu described above. Independent checkbox values persisted through reopening, and Play started a fresh native process. Screenshots and a short recording are under ignored `playtests/android/`.

The native desktop and GLES2 runs each matched all 301 original movement and camera samples. Across eleven captured frames, GLES2 differed from the current desktop build by a mean absolute colour-channel value of 0.01894 out of 255. This is a scene comparison, not proof of every render-state combination. Isolated alpha-reference boundaries, alternate virtual screen sizes, and split-view projection remain additional shader tests.

The test-gap audit found and verified fixes for duplicate ZIP path aliases and interrupted-import save recovery. The maintained tests also cover read failures, complete save preservation, every required folder, and traversal through existing parents. The 1 GB arithmetic boundary is tested directly; extraction of a full-size archive remains an integration gap. The finishing pass removed temporary graphics diagnostics and moved the native GLES build under `playtests/`. Comment and prose checks found no further issues after documenting the touch-binding requirement. TypeScript checks do not apply, and this repository has no configured formatter. The ADR sweep updated the Android rendering decision.

SDL's accelerometer-as-joystick option is disabled on Android. Otherwise, gravity appears as a held direction through the engine's legacy joystick reader. SDL also receives an explicit landscape orientation hint and fullscreen window request.

The architectural choices are recorded in [the Android decision](decisions/2026-10-18-android-sdl-build.md). The original source pin and its desktop launcher are unchanged.
