# Visual presets

Rewired Visual Presets **0.1.2** replaces the incomplete native cel recreation
with the **original ReShade FX files**, executed by a private, patched vkShade
runtime. The previous 0.1.0/0.1.1 description overstated the DrawnShade adaptation:
it implemented only part of Comic and omitted the rest of the active chain.
That implementation has been removed.

Open **F1 → Mods → Rewired Visual Presets → Preset**. Choices persist through the
normal game settings. No restart is required when switching these five choices:

| Choice | Implementation |
|---|---|
| Current (original) | Bypasses the external effect chain and native color pass. |
| DrawnShade - original FX (no bloom) | Original FX files and original active preset values, with the three glow/bloom effects excluded below. |
| Enhanced Colors | Native adaptation of SwordDeath's color stages. |
| Better Lighting | Native adaptation of SwordDeath's color stages; does not replace the lighting renderer. |
| ChromeFX | Native adaptation of Magiike's color stages. |

**Compare original** bypasses the selected style without forgetting it.
**Color preset strength** applies only to the three color adaptations.
DrawnShade retains its original strength. **Cel mesh line width** controls
Comic's mesh iterations (original: 2), and **Cel fine mesh detail** defaults on,
as in the author's preset. The old width/detail settings belonged to the removed
approximation and are not carried into the original shader.

## Exact scope and source provenance

The [author's original preset](https://drive.google.com/file/d/1tTB4RP2DPs-4QwAFhFWH8jiFjTL1wm1A/view)
from the [linked Reddit post](https://www.reddit.com/r/pcmasterrace/comments/1tiiuyt/the_pc_port_of_twilight_princess_is_awesome/)
enables ten techniques. The retained techniques execute in their original order:

1. Vignette
2. EyeAdaption
3. Lumenite_Kernel
4. Lumenite_RTAO
5. Monocular_Cues (Depth_Cues.fx)
6. ContrastStretch
7. Comic

**Excluded:** AmbientLight, OrtonBloom and prod80_02_Bloom. AmbientLight adds
screen-space glow/lens/dirt; its adaptation and glow intensity are coupled in
the source. It is excluded as a whole, not silently replaced. Depth Cues' own
internal filtering is retained. Inactive preset sections are not enabled.
Existing game bloom, textures, shadows and other graphics settings are untouched.
The user's bloom multiplier remains approximately 0.10.

All downloaded FX/include/texture files are byte-for-byte verified against
[the source manifest](../rewired-visuals/original-fx-sources.json). Every retained
preset uniform is copied without value changes. Reversed depth and depth capture
are configured for Dusklight. The two optional mesh controls are explicit user
adjustments, defaulting to the preset values.

The author's INI does not pin shader revisions. This installation pins compatible
upstream revisions, including Lumenite's May 17, 2026 revision containing the
named Kernel/RTAO pair. These are **original shader implementations**, not proof
of the exact revisions installed on the author's PC. The author also used a 4K
OLED display and a separate texture pack. No screenshot-identical claim is made.
External FX process the presented image, including HUD/menu pixels, as ReShade
does; the three native color adaptations still execute before the HUD.

## Runtime and installation

The private layer is loaded only by the Rewired launcher. It is not installed
as a system Vulkan layer and does not modify the game's executable. It requires
Wayland; an X11-only session leaves native presets available and reports Cel as
unavailable. The normal TP Custom shortcut/profile and saves are untouched.

Pinned upstream: slobodaapl/vkShade `1b65208c01ff0a4d3592d4617911d111c5fe4694`.
[The compatibility patch](../rewired-visuals/runtime/vkshade-compat.patch) adds:

- Shared same-named textures between separate FX files, retaining owners and
  checking shape/format. Lumenite AO consumes Kernel's real outputs.
- Safe copies of the loader/Features2 chain to enable supported shader-storage
  features, without modifying the application's input structures.
- Correct external config reload and private config location.
- A small versioned control file for instant preset bypass and original Comic
  mesh parameters. Invalid/missing control data disables the external effects.
- Explicit disabled hotkeys. The in-game F1 panel owns preset controls.

The layer's X11 present path was unsuitable in testing. Native Wayland is
required, and the conflicting Gamescope WSI layer is disabled for this child
process. An earlier test manifest lacking the runtime's custom function exports
was rejected during testing; the provisioner now includes the upstream exports.
Neither failed candidate was installed in the player instance.

`tools/build_original_fx.sh` builds the pinned runtime with Meson/Ninja, Vulkan,
SPIR-V headers, glslang, Wayland, xkbcommon, X11 and Xi development dependencies.
Then provision locally (compiled code and author shader files stay outside Git):

```sh
python3 tools/provision_original_fx.py \
  --destination '/path/to/Rewired Playtest/original-fx-runtime' \
  --preset '/path/to/DrawnShade-original.ini' \
  --library build/vkshade-original-fx/src/libvkshade.so
```

Do not provision over a bundle used by a running game. The launcher verifies
bundle hashes and initializes only its private control state; it does not write
bloom or other graphics preferences. Install `deploy/rewired_visuals.py` beside
`rewired_launch.py` and the 0.1.2 mod package through the normal staged update.
The ordinary native package build remains documented in [BUILD](BUILD.md).

## Verification

Isolated actual-game checks cover the original chain in an interior and Hyrule
Field, real scene-depth AO diagnostic output, all five selector values, Current
bypass and original comparison. Shader creation and shared texture logs were
inspected; screenshots came from presented Gamescope output, not the pre-layer
capture hook. Existing modern graphics and Compass were enabled.

Focused tests: `python3 tests/test_original_fx.py`,
`python3 tests/test_staged_update.py`, and the patched runtime's device-feature
tests. These verify effect order/values/exclusions, config preservation, private
layer setup, integrity rejection, fallback behavior and safe feature-chain copies.
Additional visual preference, performance and mission coverage remain player
playtesting, not inferred from compilation.
