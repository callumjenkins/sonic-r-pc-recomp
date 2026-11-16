Status: accepted.

Callum asked: "How would we upscale or recreate the textures themselves, on the characters and/or Resort Island?" Then he approved the plan with "go with the defaults and build phase 1". Source: project conversation, 2026-10-21. The defaults were raw RGBA files in the engine, mipmaps on HD pages, and 4× packs for the proof of concept. After trying an inverted test pack he said: "I'd want both the inverted and normal colours then", and chose a pack picker with the upscaled packs.

Replace whole texture pages at load, and only in the GPU copy. `LoadTPageRGB` and `D3D_LoadPlayfieldTilesRGB` look for a replacement in the game data's `HD` folder after they fill a page. The 16-bit page stays as it is, because collision reads the ground pages' key green to decide what is off the track, and several screens patch pages in place. The replacement goes to the renderer beside the existing 32-bit sky override, and the upload prefers it. Texture coordinates already reach GL as fractions of the page, so a larger page needs no geometry or UV changes.

The `HD` folder holds named packs, `HD/<pack>/<folder>/<name>.RGBA`, and the option names the pack rather than switching one folder on, so several packs can sit in the game data and be swapped without an import. Packs are raw RGBA, sized by file length, like the game's own `.RAW` pages. That keeps an image decoder out of the engine. `tools/hd-textures` converts from PNG and sets transparency from the stock key mask, so the drawn edge of the track matches the one collision reads from the stock page.

Any code that is about to change a page's pixels drops its replacement first: `ClaimTpage`, `GL_KeepPixels`, and the tinted-background upload. A dropped page that is already on the GPU uploads its stock pixels at once, because the frozen-page flow patches sub-rectangles and never uploads the whole page again. Pages patched in place, such as the minimap's `ICON01` and the menu backgrounds, therefore always draw stock.

HD pages upload with a full mip chain built on the CPU, so each level keeps alpha at 0 or 255. Averaged alpha from `glGenerateMipmap` would pass the renderer's 0.01 alpha test and grow every cutout with distance. Magnification keeps the existing per-page filter rules.

See [the guide and test list](../hd-textures.md).
