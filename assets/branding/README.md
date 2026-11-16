# Racing R artwork

The master image is `racing-r.png`. It was generated for this project with the built-in image generation tool on 2026-10-18. The input was text, followed by a cleanup edit using the first generated image. No original Sonic R artwork was supplied to either operation.

The Android resource is a 432 × 432 resampling of the master, with its alpha channel preserved. The adaptive foreground adds 20% padding on each side. The background is `#102633`. Android 13 and later can also use the foreground as a monochrome themed icon.

This is a newly generated design, not a cleared commercial trademark. The research and scope of the original-asset permissions are in [the icon notes](../../docs/android-icon.md).

## Generation prompt

```text
Use case: logo-brand.
Asset type: Android launcher adaptive-icon foreground, a single square transparent PNG.
Primary request: Create a fresh, independently designed logo for a community-built PC racing-game engine port. Make an unmistakable capital "R" from a single broad, rounded racing-track ribbon, with a confident forward-leaning diagonal leg and a small integrated sweep suggesting running speed.
Style: exceptionally clean, flat, vector-like graphic design, strong silhouette readable at 48 pixels. Ivory-white main ribbon with one restrained teal lane accent. No gradients, lighting, texture, shadows, or tiny details.
Composition: one centered mark with generous genuine transparent padding. The entire painted mark must fit within the central 60% of the square canvas so Android circle and squircle masks will not crop it. No background shape, enclosing circle, tile, border, or mockup. Transparent background.
Text verbatim: "R", only as the designed monogram. No other text.
Constraints: create the geometry from scratch. Do not imitate a commercial game logo or use any game artwork, screenshots, mascot silhouette, cartoon face, animal quills, wings, stars, gold rings, red shoes, or checkered flags. No existing brand marks or watermarks. The result should identify an independent running/racing software project without suggesting an official publisher product.
```

## Cleanup prompt

```text
Use case: precise-object-edit.
Edit target: the supplied original R racing-track monogram.
Keep the exact overall R silhouette, ivory-and-teal two-colour design, centered position, square canvas, and genuine transparent background.
Change only the surface and edge quality: redraw the mark as immaculate flat vector-like artwork. Remove EVERY stray fleck, speckled pixel cloud, grunge, texture, fringe, and rough paint edge around and inside the R. Outside the clean logo boundaries the pixels must be fully transparent. Inside the ivory ribbon use a perfectly uniform ivory fill; inside the teal stripe use a perfectly uniform teal fill. Preserve the existing hollow opening of the R as fully transparent. Edges should be clean smooth antialiasing. No white haze in the hollow area, no shadow, no glow, no detached pixels.
Do not introduce any other object, backdrop, border, lettering, commercial brand, or mascot. This is a production Android icon foreground, not a distressed illustration.
```
