The first APK had digital touch buttons. Callum asked, "Did this bring the touch controls like Saturn Bomberman? Though in this case we'd want to add in 3D pad layout too". Source: project conversation, 2026-10-18.

Add a separate, remembered **Saturn 3D Pad layout** choice to the Android launcher, initially off. Keep the existing digital overlay available. The closest existing component here is `GameActivity`'s button overlay. The Bomberman reference is `saturn-recomp/runtime/src/host/touch_pad.h`: it draws a D-pad, two rows of face buttons, shoulders, and Start. Use that arrangement as a reference while implementing the touch view for this project's SDL2 activity.

The new layout has an analogue movement stick above a smaller D-pad, six face buttons, shoulder buttons, and Start. A separate camera stick supports the optional orbit camera. The layout does not enable either gameplay option. Classic movement and menus receive digital directions; modern movement also receives the stick's continuous direction and strength through the existing platform input function. Movement and collision remain in the existing engine.

Keep finger ownership while dragging a stick outside its bounds. Track fingers independently so movement, camera, and actions can be held together. Release all inputs on cancellation, focus loss, pause, and a layout resize. Directions from the stick and D-pad must combine without one finger releasing another's input.

## Test list

| Unit | Case | Expected | Level |
| --- | --- | --- | --- |
| Touch state | Stick centre, partial travel, edge, and outside | Zero at centre, proportional axes inside, unit magnitude outside. | Java unit |
| Touch state | Diagonal and direction changes | Both digital directions update; previous directions release. | Java unit |
| Touch state | Movement, camera, and action fingers together | Independent axes and action remain held as other fingers release. | Java unit |
| Touch state | Stick drag crosses face buttons | Stick keeps its finger; no accidental face press. | Java unit |
| Touch state | Multiple fingers hold one action or direction | Releasing one leaves the other held. | Java unit |
| Touch state | Cancel and resize while held | All axes and buttons return to zero. | Java unit and emulator |
| Touch state | Wide, compact, and tall landscape sizes | Controls stay visible and separated. | Java unit and screenshot |
| Modern input | Touch analogue plus synthesized digital directions | Continuous heading and strength survive digital menu input. | C unit |
| Modern input | Existing keyboard and physical-controller priority | Keyboard direction still takes precedence for ordinary controller input. | Existing C unit |
| Platform input | Independent touch takeover and release with a physical-controller fixture | Only the active axis pair overrides the controller; release restores it, and recenter remains available. | Native SDL virtual-controller unit |
| Launcher | Fresh install and independent saved choices | Digital overlay by default; layout choice persists without changing movement or camera options. | Emulator |
| Android input | Start menus, enter a race, hold movement and jump, then pause/resume | SDL receives touch input, analogue movement works, and held input clears. | Emulator |
| Desktop | Existing controls tests and release build | Default desktop and pinned original remain available. | Native checks |

Physical phone ergonomics, controller coexistence on hardware, and Fold display transitions need a device check. Emulator checks establish input wiring and layout, rather than how it feels to play on a phone.

## Digital Saturn pad refresh

Callum requested, "the Saturn touch control design could be taken from Saturn Bomberman's. The 3D version is okay though." Source: project conversation, 2026-10-18. Adapt `saturn-recomp/runtime/src/host/touch_pad.h` and `host.cpp`'s Western pad styling for the digital layout. Keep the existing 3D geometry and rendering. The digital layout preserves Bomberman's proportions, with a tighter width cap to separate Start and the face buttons on narrow displays. Both layouts use `TouchPadState`, `TouchPadView`, and the same SDL key mapping. The digital pad emits zero analogue axes. Replace the text Menu button with a three-bar vector icon while preserving its accessible label, shortcuts, and visibility independence.

| Unit | Case | Expected | Level |
| --- | --- | --- | --- |
| Digital layout | D-pad, curved face rows, shoulders, and Start | Bomberman proportions, controls within landscape bounds. | Java unit and emulator screenshot |
| Digital input | Diagonals, slides, duplicate fingers, and simultaneous face presses | Independent held bits, zero analogue axes. | Java unit |
| Digital release | Cancellation, resize, and menu opening | Held buttons clear. | Java unit and Android instrumentation |
| Compatibility | Existing 3D state and native axes checks | Existing geometry and analogue behavior retained. | Existing tests |
| Menu icon | Icon click, hide/show, and both layouts | Accessible menu entry survives all overlay states. | Android instrumentation and screenshot |

The adapted code comes from the local `saturn-recomp` checkout at `dad85a00496a55ff80376fdd24f141251a235575`. Its MIT notice is included in `android/app/src/main/assets/licenses/saturn-recomp.txt` and packaged in the APK. The icon is a small Android vector drawable. Neither change needs a bitmap or game artwork.

The refreshed ARM64/x86-64 APK built successfully. `./tests/android-touch` and `./tests/run` passed, and the Android menu journey passed with the icon, both layouts, hidden controls, and same-session changes. The game-process input runner now checks menu release separately for digital D-pad plus Run and for both 3D sticks plus Run. Screenshots `digital.png`, `hidden.png`, and `3d.png` under `playtests/android-menu/` show the new pad, retained menu access, and the existing 3D layout. The test-gap audit added duplicate-pointer and D-pad ownership assertions; both reject their corresponding mutations. Exact appearance was reviewed from the screenshots and reference drawing code.

The finishing pass found no temporary diagnostics or explanatory comments to remove. The fresh claims check found no corrections. TypeScript and a configured formatter do not apply. APK signing, 16 KB ZIP alignment, and the packaged MIT notice passed inspection. The ADR sweep updated the existing Android decision; the input lifecycle remains the same.

## How input reaches the game

`TouchPadState` converts finger positions into four continuous axes and held buttons. `TouchPadView` draws that state and sends it to `GameActivity`. Buttons enter SDL as keyboard events so the existing menus and classic movement keep working.

The axes take a small JNI call into the C platform layer. JNI is Android's interface between Java and native code. The UI and game run on different threads, so the platform reader takes a synchronized snapshot of all four values. `ModernControlsUpdate` then applies its existing deadzone, camera-relative heading, and strength calculations. The game still accelerates, steers, collides, and moves the camera through the same functions used on desktop.

An active touch stick overrides only its corresponding physical-controller axes. Releasing it restores controller input. Modern movement reads continuous touch axes before the digital directions synthesized for menus; otherwise every stick direction would collapse to one of eight digital headings. The existing keyboard priority for ordinary physical-controller input remains unchanged.

Run `./tests/android-touch` for the Java finger-state tests and native platform-input tests. The native test compiles the Android input path against desktop SDL and JDK JNI headers, then uses an SDL virtual controller to check coexistence without needing Bluetooth hardware. It needs the same SDL development libraries as the desktop build and `sdl2-config`. Run `./tests/run` for the existing controls tests.

## Verification

On 2026-10-18, `./tests/android-touch`, `./tests/run`, and the desktop release build passed. The Android build produced a signed APK with ARM64 and x86-64 libraries, verified ZIP alignment, and no bundled game assets.

A software-rendered Android 15 emulator navigated the menus into Resort Island. After the countdown, a three-finger gesture held movement, Jump, and the camera together. Sonic moved and jumped, and the camera changed height. Cancellation and backgrounding with fingers held returned all stick visuals and action buttons to their released state. The maintained Java tests check every axis and button after reset; the native tests check the resulting input selection. These checks establish input wiring, rather than phone ergonomics.

Fresh launcher settings had all three choices off. Selecting only the 3D Pad layout, closing the app, and reopening it preserved that choice while movement and camera stayed off. The existing digital overlay also launched successfully during the runtime checks.

The host-GPU emulator crashed during activity switching, so the completed runtime checks used software rendering. Short scripted menu taps were unreliable there; the successful run held them for 400 ms and waited for transitions. The harness, commands, captures, and audit report are under ignored `playtests/android-touch/`.

The test-gap audit found four surviving mutations in the initial tests. Maintained cases now cover downward movement, camera height, every action button, and the digital deadzone. Additional cases cover separate control hit regions, duplicate fingers, full resize reset, and controller takeover. The finishing review corrected the shoulder labels to Drift and moved stick labels above their circles. The architecture check confirmed that touch feeds the existing platform reader and gameplay functions. No TypeScript or configured formatter applies to these files. The ADR sweep updated [the Android decision](decisions/2026-10-18-android-sdl-build.md).
