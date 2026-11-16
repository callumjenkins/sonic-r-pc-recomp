# HD textures

Callum asked: "How would we upscale or recreate the textures themselves, on the characters and/or Resort Island?" Then: "yes, plan the HD textures POC", and "go with the defaults and build phase 1". After trying an inverted test pack on the phone: "I'd want both the inverted and normal colours then", answered with a pack picker and the upscaled packs. Source: project conversation, 2026-10-21.

The game data's `HD` folder holds named packs, one folder each. With **HD textures** on and a pack chosen, any texture page that has a replacement in that pack draws at 512, 1024, or 2048 pixels instead of 256. Everything else keeps reading the stock page, so collision, timing, and saves are unchanged. It defaults off. Run it on Linux with `--hd-textures PACK`, or turn on **HD textures** on the Android Experiments page and choose a pack under it. The [decision record](decisions/2026-10-21-hd-texture-packs.md) explains the approach. The upscaled packs are derived from the game data, so they stay in the gitignored `gamedata/` and `playtests/` folders.

## Making a pack

```sh
tools/hd-upscale                             # upscales the race pages with each model and packs them
tools/hd-textures dump                       # or: every stock page as PNG in playtests/hd/stock/
# upscale or repaint the PNGs you want, keeping their names and folders
tools/hd-textures pack FOLDER NAME           # writes gamedata/HD/NAME in the engine's format
```

`tools/hd-upscale` runs three 4× ESRGAN models on the GPU through spandrel and PyTorch: 4x-UltraSharp, a general model; 4x-AnimeSharp; and HDcube3, made for GameCube and Wii textures. It downloads each into `playtests/hd/models/` on first use and checks its SHA-256. All three are CC-BY-NC-SA-4.0, so the packs are for personal use, and each pack's source folder under `playtests/hd/upscaled/` holds a `LICENCE.txt`. It writes the PNGs there, packs them as `gamedata/HD/UltraSharp`, `AnimeSharp`, and `HDcube3`, and draws a contact sheet per page in `playtests/hd/sheets/`: stock, then each model, in the order `ORDER.txt` gives. `--model` and `--only` narrow a run.

Most pages are upscaled in pieces, so one texture's colours stay out of the next one's edge. Island pages are cut into 64-texel cells. Each side of a cell is padded from its neighbour when the page runs on smoothly across the seam, by wrapping when the cell tiles with itself, and otherwise by repeating its edge, so a tiling texture still tiles at 4×. Ground tiles are 32 texels and sit beside different tiles on the ground, so they are mirrored. Character pages have no grid and go whole. Only colour is upscaled: `pack` sets alpha from the stock key mask.

`pack` takes a square PNG of 512, 1024, or 2048 pixels per page. It sets transparency from the stock page's key green, scaled up, so the drawn edge of the track stays where collision puts it. It also gives transparent texels the colour of their opaque neighbours, so filtering and mipmaps never pull key green into an edge. `tools/hd-textures resize --scale 4 --out FOLDER` writes stock pages scaled up by repeating texels, which is what the tests use.

The engine reads `HD/<pack>/<folder>/<name>.RGBA`, raw RGBA with the size taken from the file length. A pack name is one folder: not hidden, printable, and under 64 bytes. The flat ground comes from a PLY tile sheet that the engine lays out on two pages, so its replacements are `HD/<pack>/ISLAND/ISLAND.PLY.0.RGBA` and `.1.RGBA`, as `dump` writes them.

On Android, put the `HD` folder in the game data ZIP. `tools/package-android-data` adds `gamedata/HD` when it exists. Each import replaces the `HD` folder, the same as `MUSIC`. The importer upper-cases paths inside `HD`, as it does for the game folders, but keeps each pack folder's own case, because the picker shows it. The picker lists the packs it finds and remembers the last one chosen. When that pack is gone it uses the first, and keeps remembering the missing one until a pack other than that first one is picked, so a later import that brings it back brings the choice back too.

## Code map

| Part | Where | What it does |
|---|---|---|
| Option, status, and loading | `hd_textures.c` | The option the `--hd-textures` flag in `main.c` sets, the JNI setter and status, and `HdTextures_Load`, which reads a replacement from the chosen pack after a page loads |
| Pack names, paths, sizes, mips, and status text | `hd_textures_format.c` | Pure functions, used by the engine and the unit tests |
| Pack picker | `ExperimentsView`, `HdPacks` | The switch and the pack list on Android |
| Upscaling | `tools/hd-upscale` | Models, cells, padding, and contact sheets |
| Whole pages | `LoadTPageRGB` | Looks for a replacement after the stock page loads |
| Ground pages | `D3D_LoadPlayfieldTilesRGB` | The same for both pages it lays out |
| Dropping a replacement | `ClaimTpage`, `GL_KeepPixels`, `GL_SetPendingRGBA` | A page that gets new content, or that code is about to patch, goes back to stock |
| Upload | `GL_UploadTpage`, `GL_SetTpageHD` | The upload sends the replacement and its mip chain in place of the 16-bit page. Dropping a page that is on the GPU uploads the stock page straight away |
| Filtering | `GL_ApplyTpageFilter` | Mipmapped minification on HD pages; magnification follows the existing per-page rules |

## Test list

| # | Unit | Case | Expected | Level |
|---|---|---|---|---|
| 1 | Path | Stock page paths map to paths in the pack, including the snow folder and the two ground pages | `HD/Upscaled/ISLAND/ISLAND00.RGBA`, `HD/Upscaled/ISLAND/SNOW/ISL_S_04.RGBA`, `HD/Upscaled/ISLAND/ISLAND.PLY.0.RGBA` | Unit, `tests/hd-textures` |
| 2 | Path | Paths outside the data folder or with no extension have no HD path | No path | Unit |
| 2b | Pack name | A pack name is one printable, visible folder under 64 bytes, and the engine, the tool, and the picker apply the same rule | `..`, `.hidden`, `a/b`, a tab, 0x7f, and 64 bytes refused; the picker also refuses characters outside the Basic Multilingual Plane, which JNI would hand the engine in different bytes | `tests/hd-textures`, `tests/hd-textures-tools`, `tests/android-import` |
| 3 | Size | Only square pages of 512, 1024, and 2048 load | Other sizes are skipped | Unit |
| 4 | Mips | Halving averages the opaque texels, rounds, and keeps alpha at 0 or 255 | As stated | Unit |
| 5 | Status | The status texts | Exact strings | Unit |
| 5b | Loading | With no pack chosen a present pack is ignored; a missing pack or a name outside `HD` loads nothing; each pack supplies only its own pages; a slot reloaded without a replacement goes back to stock; truncated files and out-of-range slots are skipped; and the status needs the pack to be a folder | As stated | Unit, `tests/hd-textures` |
| 6 | Pack | Alpha follows the stock key mask and no transparent texel stays key green, on every Resort Island and character page | No mismatches | `tests/hd-textures-tools` |
| 6b | Pack | Transparent texels take the colour of nearby opaque ones, the key test matches the engine's 5-bit test at its boundaries, and the command writes the engine's names into the pack's folder and fails on a page with no stock match | As stated | `tests/hd-textures-tools` |
| 6c | Tools | `dump` and `resize` write the pages they select, scaled and inverted as asked, and `dump` skips the `HD` folder | As stated | `tests/hd-textures-tools` |
| 7 | Pack | Sizes the engine would skip are refused | `ValueError` | `tests/hd-textures-tools` |
| 8 | Ground | The tool lays PLY tiles out where the engine does | Tile 9 at (32, 32) on page 0, tile 64 at (0, 0) on page 1 | `tests/hd-textures-tools` |
| 9 | GLES | A 2048 page minified on screen draws, so its mip chain is complete, with mipmapped filters for both smoothing settings; dropping it brings back the stock page and plain filters | Green, then red | `tests/android-graphics` |
| 9b | GLES | A drop in the middle of a frame leaves the page that was bound drawing, and a dropped page whose dirty flag is cleared, as the frozen-page flow does, still shows stock | Blue, then red | `tests/android-graphics` |
| 10 | Gameplay | With two packs present and none chosen, with a pack chosen that is not there, and with each pack loaded, the scripted Time Attack trace equals the pre-change baseline, and no page is replaced while none is chosen | Byte for byte | `tests/hd-textures-shots --baseline playtests/runs/wide-island-baseline` |
| 11 | Mapping | With both packs present, the repeated-texel 4× pack draws frames close to stock, and the inverted pack does not | Mean difference at most 4 out of 255, and at least 40 for the inverted pack | `tests/hd-textures-shots` |
| 12 | Patches | The minimap, which the race patches into `ICON01`, stays stock with `ICON01` in the pack | Region difference at most 4 | `tests/hd-textures-shots` |
| 13 | Slots | The race's HD slots, and slots reloaded with pages outside the pack go back to stock | Slots 0 to 6, 9, 12, 14 to 16 replaced; 7 and 8 returned | `tests/hd-textures-shots` |
| 14 | Import | An `HD` folder imports with each pack folder's case kept, a later import replaces it, and `HD` alone is not ready | As stated | `tests/android-import` |
| 14b | Picker | The pack list is the visible folders the engine accepts, sorted without case; the remembered pack wins when present, else the first; and a remembered pack that is missing stays remembered until another is picked | As stated | `tests/android-import` |
| 15 | Android | Experiments switch and pack picker, the status in a Resort Island race, changing pack mid-race, and turning it off | "HD pack Nearest, 11 pages replaced", "HD pack Inverted from the next race, 11 pages of Nearest in use now", then "off from the next race" | Emulator, `tests/android-menu` |
| 16 | Upscale | Splitting a page into cells and merging it back is exact for Island, ground, and character pages | Equal to a repeated-texel upscale with a repeating stand-in model | `tests/hd-textures-tools` |
| 17 | Upscale | Padding reads on across a smooth seam, wraps a cell that tiles, and otherwise repeats the edge; a smoother wrap beats the neighbour; a flat edge reads on across a 3-level step but not a 10-level one; and a tiling cell upscales the same as it would in the middle of copies of itself | Within 2 levels | `tests/hd-textures-tools` |
| 17b | Upscale | Ground tiles are mirrored whatever their neighbours, and character pages go whole, reflected at the border | Equal to the stand-in model over the padded input | `tests/hd-textures-tools` |
| 18 | Look | Stock and each model's pages and race frames side by side | Callum's eye | `playtests/hd/sheets/` |

## Results

Checked on 2026-10-21 on Linux.

- Two stock runs differ by a mean of up to 0.2 out of 255, from sparkles. With the switch on and no pack, frames differed by between 0.0 and 1.2 across runs, all of it in the first frames. The repeated-texel pack differed by up to 3.1, because mipmaps soften the distance. The inverted pack differed by at least 47.
- The mapping check uses a mean difference rather than a count of equal pixels, because mipmaps change most distant pixels slightly. A page mapped wrongly over a small part of the frame could stay under the limit, so the inverted pack's screenshots are worth a look after changing the upload.
- All traces matched the pre-change baseline byte for byte.
- Before the immediate re-upload on drop, the minimap drew as a black box: the race uploads `ICON01`, patches the minimap into it, and never uploads the whole page again.
- `tools/hd-upscale` ran all three models over the 19 race pages in about six minutes on an RTX 3090, most of it installing PyTorch and downloading the models. Each pack is 76 MiB. A pack race with each kept the trace and replaced 11 pages.

## Known limits

- The minimap, the menu backgrounds, and the title page are patched while the game runs, so they always draw stock.
- The software and Dreamcast renderers ignore packs.
- The Android importer refuses game data over 1 GiB. The stock data is 233 MiB, and a pack of all 93 pages is 372 MiB at 1024 but 1.5 GiB at 2048, so a 2048 pack on Android has to cover only some pages.
- Seams between ground tiles, which are mirrored rather than given real neighbours, are the likeliest flaw in the upscaled packs.
- A phone check of the upscaled packs on the Fold is still to do.
