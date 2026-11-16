Callum reported, "We're unable to call a launcher menu in game like with Saturn Bomberman. This means we can't hide the touch controls or add other menu based features yet." Source: project conversation, 2026-10-18.

The closest existing settings screen is `LauncherActivity`. Reuse its preferences and checkbox UI in a game-menu activity in the launcher process. The running engine stays in `GameActivity` in `:game`. Opening the menu backgrounds SDL, which pauses its event loop and audio through the existing lifecycle. Returning passes touch settings in the activity result. This keeps preference reads and writes in one process and avoids restarting a race to change the overlay.

Keep a Menu button outside the hideable controls. Android Back, the keyboard Menu key, and controller Select also open the menu. Resume returns to the current session. Return to launcher ends it. Camera-relative movement and orbit remain independent launch-time choices, explicitly labelled for the next game. Touch visibility defaults on, while the optional 3D layout still defaults off.

The menu entry now uses a three-bar vector icon on a circular background, with the accessible name **Open game menu**. The digital overlay uses Bomberman's Saturn pad design, as described in [the touch refresh](android-touch.md#digital-saturn-pad-refresh). Resolution and texture smoothing use the same settings view and menu result, apply on resume, and report the active native renderer status. See [the graphics options](android-graphics.md).

Both screens are now paged, sharing `ControlSettingsView`. The launcher opens on Home, and the game menu opens on Game with Resume focused. Controls, Graphics, Opponents, and Experiments follow in a sidebar on wide screens and a row of buttons on narrow ones. See [the wide island guide](wide-island.md) and [the HD textures guide](hd-textures.md) for the Experiments page. `tests/android-menu` pushes two packs of Resort Island's and the characters' pages into the app's game data for the HD journey, one with repeated texels and one inverted, and removes them afterwards. It replaces the emulator's whole `HD` folder, so packs imported there are lost.

## Test list

| Unit | Case | Expected | Level |
| --- | --- | --- | --- |
| Entry | Menu button, Android Back, and controller Select | Open one menu without ending the game process. | Android instrumentation |
| Visibility | Hide controls, resume, and reopen | Both layouts disappear while Menu remains reachable. | Android instrumentation |
| Visibility | Restore controls with Back | Controls return in the same game process. | Android instrumentation |
| Layout | Switch digital to 3D and back | Apply on resume with visibility and gameplay options independent. | Android instrumentation |
| Input | Open menu while a touch stick or button is held | Release all synthetic buttons and axes before suspending. | Android instrumentation and existing touch tests |
| Lifecycle | Leave menu open, resume, and background again | Game and audio pause, then resume without a new engine session. | Emulator |
| Preferences | Change options in menu, return to launcher, and relaunch | Both screens agree and the next launch uses the selected flags. | Android instrumentation |
| Exit | Return to launcher | Close game process and reveal launcher, retaining data and preferences. | Android instrumentation |
| Pages | Open each page from the sidebar and from the narrow row | The selected button is marked and its controls appear. | Android instrumentation |
| Experiments | Turn on Wider Resort Island, change the width in game, then enter a race | The Game page reports the pending width, then the width on this course. | Android instrumentation |
| Experiments | Turn on HD textures, choose one of two test packs, enter a race, switch pack, then turn it off | The Game page reports the pack and its replaced pages, then that the other pack, and then none, takes over from the next race. | Android instrumentation |
| Compatibility | Build both Android ABIs and run existing checks | Original desktop path and independent default-off additions remain intact. | Build and native tests |

Physical controller shortcuts, phone Back gestures, and Fold transitions need a phone check.

## Verification

Build the production APK with `./tools/build-android --abi arm64-v8a --abi x86_64`. Install it on a test emulator with imported game data, then run `./tests/android-menu --serial emulator-PORT`. This builds a separate instrumentation APK. It preserves the four control preferences and two graphics preferences, and does not edit game saves. Captures and logs go under ignored `playtests/android-menu/`.

The Android 15 journey passed with Back, Menu, and injected Select/Menu key events. Both overlays hid and returned in the same engine process. The choices persisted through returning to the launcher and fresh game launches, including movement-only and camera-only selections. A canceled Back release kept the game open. These checks observe preferences and overlays; the independent scratch audit checked exact launch arguments across all sixteen combinations of the four choices. The maintained journey does not read native movement/camera flags.

A second instrumentation runner executes in the game process and holds the movement stick, camera stick, and Run together before opening the menu. It checks that all four Java touch axes and synthetic button state clear synchronously before the pause callback. Existing `./tests/android-touch` checks verify the separate JNI axis reader and touch clear behavior. The menu test does not inspect native axis memory directly.

A separate Resort Island run opened the menu for about twelve seconds. Two samples three seconds apart showed the native SDL thread asleep, with unchanged user/system CPU ticks of 183/2229. The race clock showed 2.60 seconds before the menu and 5.16 seconds after resuming and waiting two seconds. The same race resumed, and the hidden overlay left Menu visible. Screenshots and the thread samples are in `playtests/android-menu/`. SDL's blocking pause path suspends its audio devices too; this menu check did not record audio output.

The controls, touch, import, audio-file, and save-data checks passed. APK signature and 16 KB ZIP alignment checks passed. The independent test audit found no normal-path defect. The finishing pass found no explanatory source comments or temporary production diagnostics to remove. TypeScript checks and a repository formatter do not apply. The Android ADR records the shared settings and lifecycle decision.
