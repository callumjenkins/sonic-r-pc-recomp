# Custom CPU opponents

Callum requested: "Can we add a mode to allow CPU opponents to be duplicated? For example to face five Super Sonics potentially. The clones could use different colours. They'd still be gated by which characters are unlocked." Source: project conversation, 2026-10-18.

The existing race has five slots, with one human and four CPUs. Keep that count. Build on the existing character-selection AI assignment, `InitLevel`, per-player physics, and character lighting. Keep the stock roster as the default. A custom roster selects each of the four CPUs independently, with Random unlocked as an option. Validate against the loaded engine's character-unlock table when starting the race. A separate colour switch distinguishes repeated characters while preserving player-slot identity and the original character IDs.

Android builds on `ControlSettingsView`, the launcher-process preference owner, launch intents, and `GameMenuActivity` results. CPU choices apply on the next race setup, without replacing racers already on the track. JNI publishes requested options; the engine applies them. The engine also publishes availability for the menu, rather than adding a second save parser in Java. Desktop uses command-line options. The pinned original remains unchanged.

The first version applies to offline single-player Grand Prix. Time Attack, ghosts, Tag, special unlock challenges, demos, replays, split-screen, and network races keep their existing rosters. Colour variants apply to the racing models; portrait artwork remains unchanged. Ordinary progress still uses the original save/load flow.

## Test list

| Unit | Case | Expected | Level |
|---|---|---|---|
| CPU roster | Disabled mode | Original IDs, no random calls, unchanged colours | Native integration |
| CPU roster | Every unlocked character, including repeated Super Sonic | Four requested CPU IDs; human unchanged | Native integration |
| CPU roster | Locked, invalid, or random choice | Only unlocked characters selected | Native integration |
| CPU roster | No available characters | Stock roster retained without out-of-bounds access | Native integration |
| CPU roster | Disable or change choices between races | Restore saved stock roster or apply new choices at setup | Native integration |
| Scope | Other race modes, network, demo, or replay | Existing roster and rendering | Native integration |
| Colour | Unique character and disabled colours | Exact original vertex colours | Native integration |
| Colour | Repeated character, grey, black, alpha, and all slot positions | Distinct variants with preserved alpha; no shared-lighting drift | Native integration |
| Unlock snapshot | Load progress or reset availability | Published mask follows the engine table | Native integration |
| Desktop input | Valid and malformed four-character lists | Complete configuration or rejected input | Native integration |
| Android settings | Defaults, locked choices, persistence, resume, and relaunch | Same preference owner, correct native requests, independent switches | Emulator instrumentation |
| Real race | Four Super Sonic CPUs | Five initialized racers; CPUs move and render with distinct colours | Linux playtest |
| Compatibility | Defaults compared with the prior build | Same scripted player/camera trace | Linux playtest |

Increasing the field to six racers would require a separate review of fixed-size race, AI, spawn, HUD, and results structures. This change retains the five-racer engine.
