# Android graphics options

Callum requested: "Check the original readme of course, but what options do we have for increasing internal render resolution or other such 3D improvements? Can we push up to 4K? I'll be running on a Z Fold 7 if that helps with targeting. Let's include options in our Android in-game menu if it's possible." Source: project conversation, 2026-10-18.

## Renderer and scope

The pinned upstream README directs desktop users to `docs/desktop.md`. Neither documents a resolution switch. `BeginFrame` in `render_gl.c` already fits a 4:3 viewport to the drawable in physical pixels. The original 640×480 screen coordinates describe positions, while OpenGL rasterizes triangles at the drawable's resolution. Higher resolution sharpens polygon edges but retains the original textures, models, draw distance, and animation.

Build on `ControlSettingsView`, shared by the launcher and game menu, and retain its preference owner in the launcher process. Send graphics choices through the existing launch intent and menu result. A synchronized native snapshot applies them at the next `BeginFrame` on the SDL thread. Fixed resolutions use a color texture and depth buffer attached to an offscreen framebuffer; `FlipD3D` presents it in the same 4:3 display rectangle. Native display bypasses that framebuffer. Desktop and `run-original` retain their existing paths.

Expose Native display, 480p, 720p, 1080p, 1440p, and 2160p. At 4:3 these are 640×480, 960×720, 1440×1080, 1920×1440, and 2880×2160. The last has the height of 4K UHD content with pillarboxing. It does not claim a 3840-pixel-wide 4:3 scene. Downsampling uses bilinear filtering. A separate smoothing switch applies bilinear filtering to game textures, including HUD sprites. Color-keyed sprites can develop colored fringes when smoothed, as seen on title sparkles in the emulator capture. Defaults preserve Native display and the game's texture filtering. Unsupported targets fall back to Native display and report that in the menu.

[Samsung specifies](https://news.samsung.com/global/samsung-galaxy-z-fold7-raising-the-bar-for-smartphones) a 2184×1968 inner display, a 2520×1080 cover display, and Snapdragon 8 Elite for Galaxy. With the full inner display available in landscape, a 4:3 scene fits approximately 2184×1638. 2160p therefore supersamples on the inner display. Start with Native display; compare 1440p for lower GPU work and 2160p for edge quality. Sustained frame rate, heat, and battery use need a physical Fold 7 playtest.

Widescreen would require camera, culling, and HUD work. Higher frame rates would require checking the original fixed-step simulation. Mipmaps and anisotropic filtering need texture-atlas handling to avoid bleeding between sprites. Those are separate changes; this iteration adds resolution and texture smoothing.

## Test list

| Unit | Case | Expected | Level |
|---|---|---|---|
| Graphics target | Native bypass preserves drawable and creates no target | Existing physical-pixel viewport | Native GLES integration |
| Graphics target | Every fixed preset renders to its 4:3 dimensions | Listed dimensions, complete framebuffer | Native GLES integration |
| Graphics target | Invalid targets select Native; unsupported targets report fallback | Invalid presets normalize to Native; unavailable targets show explicit fallback status | Native GLES integration |
| Graphics target | Each GPU cap accepts equality and rejects an oversized target | Independent texture, renderbuffer, and viewport limits | Native GLES integration |
| Graphics target | Shader failures fall back and recover; depth16 remains supported | Native on shader failure, complete depth16 target without depth24 | Native GLES integration |
| Frame lifecycle | Present restores game draw state and image orientation | Same bindings, program, viewport, clear color, enabled tests, and upright image | Native GLES integration |
| Frame lifecycle | Changing presets and returning to Native releases targets | No stale framebuffer or texture use | Native GLES integration |
| Frame lifecycle | Drawable resize updates presentation and Native viewport | Correct 4:3 framing at wide and Fold-like sizes | Native GLES integration |
| Texture filter | Smoothing toggles independently and restores game filtering | Linear while enabled, per-page rules when disabled | Native renderer integration |
| Android settings | Menu changes survive resume and relaunch | Actual native status matches preference, same session on resume | Emulator instrumentation |
| Android settings | Defaults and controls remain independent | Native, smoothing off, gameplay switches unchanged | Emulator instrumentation |

Game data stays in ignored storage. Emulator checks establish rendering and lifecycle behavior; they do not establish Fold 7 performance.

## Verification

The dual-ABI APK built successfully. `./tests/android-graphics` uses real surfaceless GLES rendering and pixel readback. It checks every preset, independent GPU limits and equality at each limit, framebuffer and shader failures, depth16 fallback, target cleanup, image orientation, the game's draw state, and repeated frames through the actual `BeginFrame`/draw/`FlipD3D` path. Texture tests include a real dirty upload and smoothing on/off changes through the renderer.

The Android menu journey passed with native status reporting 2880×2160, then 960×720, then Native display with game texture filtering. A 720p smoothing choice survived returning to the launcher and a fresh engine process. The earlier control, touch visibility, layout, and held-input release checks still passed. Screenshots and logs are under ignored `playtests/android-menu/`.

A separate Resort Island smoke launched with 2160p and smoothing enabled, captured the scene and native menu status, and resumed the same race at Native with smoothing off. That scratch launch supplied graphics extras directly; the menu's saved preferences were Native, so resuming applied those values. The clock advanced from 2.53 to 4.23 seconds across the menu and a two-second post-resume wait. These captures are `playtests/android-graphics/race-2160p.png`, `race-2160p-menu.png`, and `race-native-resumed.png`. They demonstrate rendering and live switching, not a frame-rate benchmark.

The independent test-gap audit reproduced no product defect. It identified missing assertions around independent device limits, the real renderer's filtering call, and frame hooks. The maintained cases now fail when those checks or calls are removed. Shader failures and depth16 fallback were also added after the audit's successful throwaway probes. The report is in `playtests/android-graphics/test-gaps-audit.md`.

The architecture review traced shared preferences through the existing launch and menu-result paths. JNI publishes values without issuing GL calls; the engine thread applies them during frame setup and frees graphics resources before deleting the SDL GL context. The finishing pass removed no production diagnostics or comments. A fresh claims review narrowed the test table's invalid-value and GL-state wording. The prose pass made those corrections; TypeScript and a repository formatter do not apply. The existing Android ADR records this extension. Desktop release, controls, touch, import, APK signature, and 16 KB ZIP alignment checks passed. Physical Fold transitions, sustained performance, heat, and battery use remain device checks.

## Linux resolution comparison

The Linux follow-up rendered the same Resort Island race frame at 640×480, 1440×1080, and 2880×2160 using the desktop renderer and Mesa llvmpipe. All three runs submitted exactly the same 7,855 four-component vertex records at race tick 90, and their 91-sample player/camera traces matched byte for byte. OpenGL reported matching viewports at each resolution. The side-by-side crop shows much sharper polygon edges at 2160p while preserving the original assets. This confirms that the shared engine geometry can benefit from higher-resolution rasterization. Source inspection shows that Native display uses the physical-pixel drawable viewport. When that viewport exceeds 480p, Native already draws above 480p; the size of a Native-to-2160p improvement depends on that starting resolution.

The scratch runner, vertex dumps, traces, and comparison images are under ignored `playtests/graphics-resolution/`. `comparison.png` shows the 480p capture enlarged with nearest-neighbor sampling beside the actual 2160p crop; `full-scene.png` shows the complete scene at a common display size. Production code and the APK were unchanged by this experiment. The Android GLES integration checks also passed again. This is a rendering comparison, with no claim about phone performance or all scenes.
