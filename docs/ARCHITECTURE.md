# Architecture

| Responsibility | Source |
|---|---|
| Canonical assignments, ownership, combinations, serialization | `rewired-core/src/kcrux_slots.hpp` |
| Input sampling, native projections, wheel/Items transactions, interaction hooks | `rewired-core/src/kcrux_controls.cpp` |
| Input edges, trigger state and map tap/hold gesture | `rewired-core/src/kcrux_input_state.hpp` |
| Published slot service and implementation | `rewired-core/include/kcrux/controls.h`, `src/core_service.cpp` |
| Jump provider, settings, initialization | `rewired-core/src/manual_jump.cpp`, `settings_ui.cpp`, `mod.cpp` |
| On-foot movement/aim stick separation | `rewired-core/src/aim_movement.cpp` |
| Zora underwater left-stick vertical reversal | `rewired-core/src/swim_controls.cpp` |
| Hat-animation lifetime guard | `rewired-core/src/crash_compat.cpp` |
| HUD rendering, cues, native masking and menu presentation | `rewired-hud/src/kcrux_prompts.cpp` |
| HUD coordinates and saved options | `rewired-hud/src/kcrux_hud_layout.hpp`, `config.cpp` |
| HUD access to core service | `rewired-hud/src/core_adapter.cpp` |
| Restart mode switch, package validation and staged updates | `deploy/rewired_launch.py` |
| Pinned private runtime capacity patch | `tools/patch_renderer_capacity.py` |
| Disposable controller/capture fixtures | `rewired-core/src/kcrux_test_driver.cpp`, `tools/*rewired*.py` |
| Small core regressions and crash triage | `tests/`, `tools/check_core.py`, `tools/inspect_minidump.py` |

## Data flow and invariants

The slot registry owns Left/Right/Up/Down/RT, in that order. Native channels 0/1
mirror Left/Right. Native channel 2 temporarily projects the required higher
slot so original item consumers can run. Those mirrors must never become the
canonical assignments or overwrite inactive slots during gameplay.

The registry stores inventory indices, including combination components;
resolved icons, bottle contents and ammo come from current inventory. Assigning
an item to its current slot clears it. Assigning an item owned elsewhere swaps
the previous target back to its source. Combination ownership includes both
components. Five slots serialize to a versioned 12-byte SaveService blob per
game save slot/mode. Gameplay projections do not commit new assignments.

The separate HUD reads `KcruxControlsService` v1.4 and renders native art and
labels. Inventory frontend requests use that service's canonical transactions;
they must not synthesize old X/Y/Z assignment writes. Collection page state is
reported from the rendered page, not guessed from bumper presses. Combination
availability and execution search all five slots.

Mode switching is restart-based. The launcher applies required Essentials
settings in modern mode and restores its saved baseline in legacy mode. Do not
silently hot-reload the player profile or let a running game overwrite installed
configuration. Preserve unrelated settings and saves when deploying.

The revision 26 renderer patch raises geometry capacities and all matching
allocation/map/copy offsets in a private runtime. It leaves overflow checks
enabled. Runtime hash validation and the shared AppImage remain separate from
the gameplay mod. Consult the patcher's table before touching renderer layout.

`dawnlight`/`kcrux` names persist internally for compatibility. They do not mean
the current mods require Dawnlight to be enabled. The core/HUD stable boundary
is intended to survive a future native Essentials menu integration.
