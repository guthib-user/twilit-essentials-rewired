# Visual presets

Rewired Visual Presets is an optional, separate mod. After its first installation,
open Dusklight's **F1 menu → Mods → Rewired Visual Presets → Preset**.
Changes take effect immediately and are saved through the host's normal settings.
No restart is needed to change styles. **Current** is the default.

| Choice | Effect |
|---|---|
| Current (original) | Bypasses the added shader completely; keeps the existing look. |
| DrawnShade - Cel | Comic outlines adapted from the preset shared in the linked Reddit post. |
| Enhanced Colors | SwordDeath's Twilight Princess color treatment, with stronger color separation and darker midtones. |
| Better Lighting | SwordDeath's cooler, contrast-focused color treatment. It does not replace the game's lighting renderer. |
| ChromeFX | Magiike's warmer, lifted Twilight Princess color treatment. |

**Style strength** blends the selected look with Current, from 0 to 100 percent.
It is shared between presets. **Compare original** temporarily bypasses the
style without forgetting the selection; switch it off to see the preset again.
**Cel outline width** adjusts the outline in render pixels (1–3). Start at 1;
try 2 for a bolder effect at 4K. **Cel fine mesh detail** adds the original
shader's polygon detail and defaults off for cleaner scenery.

The module never writes bloom, dynamic-shadow, resolution, texture, gameplay or
controller settings. The player's saved bloom multiplier was approximately
**0.10**, and their existing dynamic shadows stay enabled. Original preset bloom,
glow, blur, depth-cue, grain and chromatic-aberration passes are excluded. These
are native adaptations of the sourced effects, not complete ReShade stacks or
a promise of matching the authors' screenshots. The player's existing Essentials
grading/sharpening remains beneath the selected style.

Sources and licenses are in [the package source notes](../rewired-visuals/res/SOURCES.md).
The original downloaded presets and the player's graphics/config backups are
kept locally, outside Git. No replacement artwork or texture pack is included.

## Implementation and checks

`rewired-visuals` uses the pinned Dusklight Gfx/Camera/Config/UI services and a
single WGSL pass before the HUD. Cel uses resolved scene depth with stable
reverse-Z linearization. Other styles resolve only color. Current and Compare
original skip both the extra resolve and draw. This needs no external Vulkan
layer, runtime patch, gameplay hook or new controller binding.

Focused isolated rendering checks exercised all five choices and Compare original
with modern graphics, textures, existing shadows and Compass. Cel was inspected
at night and in daylight; the initial distant-depth striping was corrected.
These are rendering checks, not a new mission playtest or a frame-rate benchmark.
Player visual preference and additional areas remain normal playtesting.

The launcher accepts the optional visuals package alongside the required
production core/HUD packages. SHA checks, running-profile rejection, test-driver
rejection, backups and rollback remain enforced. Tests run with:

```sh
python3 tests/test_staged_update.py
```

To remove the added effects, select Current or disable Rewired Visual Presets
in Mods. The module is independent of Modern Controls mode.
