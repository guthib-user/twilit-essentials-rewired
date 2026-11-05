# Build and focused checks

## Local prerequisites

Use CMake 3.26+, a C++20-capable compiler and the pinned Dusklight source/SDK at
`vendor/dusklight` (or supply `-DDUSKLIGHT_DIR=/path/to/dusklight`). See
[pins.json](pins.json). The working machine already has this ignored checkout;
do not re-download or build the entire game just to compile a mod.

The SDK supplies game ABI headers. The optional test driver also needs its
WebGPU surface. A fresh checkout must provision required upstream dependencies
separately; no game image, SDK, or compiled third-party package is bundled here.

Provide `rewired-hud/res/kcrux/xbox-bumper.rgba8` from the locally licensed
controller pack before packaging the HUD. `res/kcrux/SOURCE.txt` identifies the
source image and format. The existing local file is ignored by Git. The small
MIT ornament and its attribution are included.

## Production packages

Run from repository root. Existing configured build directories can be reused.

```sh
cmake -S rewired-core -B build/rewired-core -DCMAKE_BUILD_TYPE=RelWithDebInfo -DREWIRED_TEST_DRIVER=OFF
cmake --build build/rewired-core -j4
cmake -S rewired-hud -B build/rewired-hud -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/rewired-hud -j4
```

Outputs: `build/rewired-core/mods/twilit_rewired_controls.dusk` and
`build/rewired-hud/mods/twilit_rewired_hud.dusk`. Build only the affected component.

## Quick tests, no game required

```sh
python3 tools/check_core.py
```

These existing salvaged tests exercise the actual current core headers: all 25
slot assignment pairs, combination ownership, versioned serialization,
100,000 invariant operations, held-input suppression, trigger hysteresis,
fire cancellation and map tap/hold. They do not establish in-game behavior.

## Isolated tests, only when needed

```sh
cmake -S rewired-core -B build/rewired-test -DCMAKE_BUILD_TYPE=RelWithDebInfo -DREWIRED_TEST_DRIVER=ON
cmake --build build/rewired-test -j4
python3 tools/run_rewired_candidate.py --headless --full-graphics
python3 tools/playcheck_rewired.py state
python3 tools/playcheck_rewired.py quit
```

This requires a separately provisioned `test-profile-rewired/`, game image,
installed third-party mods/textures, and Gamescope for headless mode. The launcher
uses the local home-directory application layout; `REWIRED_TEST_RUNTIME` selects
a different runtime. Do not create a new player save just to satisfy a fixture.
The user prefers to perform manual mission playtests. Existing working mechanics
do not need new video tests unless the user requests them or needs remote review.

The driver can expose title-screen actors before a save is ready. Confirm the
real stage, player and stable procedure before applying fixture grants.

## Installation and rollback

The player's current instance is **Rewired Playtest**, not the default target of
`deploy/install_rewired.py`. Do not run the generic installer with defaults.
Resolve local paths from ignored `local/paths.json`, verify the profile is closed,
and retain package/config backups. Staged package updates must reject test-driver
packages and verify SHA-256. Save restoration is not part of package rollback.

Revision 26 also uses a separately copied `runtime-v2.0.3` beside that profile.
`tools/patch_renderer_capacity.py SOURCE OUTPUT` supports exactly the source hash
in [pins.json](pins.json); it requires a separate output and rejects unknown hosts.
Its launcher hash must match the installed private runtime. Never patch the
shared AppImage. Preserve `build/revision26-installed.json` and the local update
backup when changing or rolling back this runtime.

Docs-only organization does not require reinstalling or launching the game.

For a new crash, start with the relevant local log and use
`python3 tools/inspect_minidump.py /path/to/crash.dmp` for exception/module
addresses. Its stack-address scan is triage, not a full validated unwinder.
Retain the dump locally and publish only the relevant diagnosis or sanitized
trace. Use the pinned runtime/symbols when interpreting addresses.
