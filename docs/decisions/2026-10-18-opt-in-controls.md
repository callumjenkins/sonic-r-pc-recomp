Status: accepted.

Callum specified: "Ensure there's always a way to run the recomp as-was before any of our additions, and that for each addition they can be toggled on and off, eventually through the game interface." Source: project conversation, 2026-10-18.

Keep a launcher that builds and runs upstream revision `3da98f2e134eac82e45cd69660216fa0e4563ac5` from an isolated local archive. Keep that executable and build tree outside the tracked working source. This makes the original reproducible after further changes and avoids relying solely on a cached executable.

Default each gameplay addition to off. Represent movement and camera switches separately in one options structure. Command-line switches select the first experiment; a later in-game interface can use the same structure. An unmodified executable and an updated executable with additions disabled remain separate comparison targets.

Use the existing human-player movement and camera functions. The initial experiment applies only to a single local Sonic player. Modern movement cannot be represented by the original button-only ghost and replay format, so prevent those runs from saving incompatible ghosts or starting an invalid replay. Classic-mode recordings keep their existing interpretation.

The cost is maintaining a separate original build and limiting the first experiment's scope. A copied binary alone could disappear or be overwritten; treating the modified executable with switches off as the only original would not independently establish compatibility.
