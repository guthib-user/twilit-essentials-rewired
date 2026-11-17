# Visual preset sources

These are native Dusklight adaptations, not the complete ReShade effect stacks.
The independent module draws before the HUD and keeps the player's existing
textures, shadow renderer, grading, bloom and gameplay settings. Current bypasses
the module. No blur, bloom, ambient-light glow, grain, lens dirt, extra vignette,
chromatic aberration or extra sharpening is applied.

## DrawnShade - Cel

- Preset author: durchfallalarm.
- [Original post](https://www.reddit.com/r/pcmasterrace/comments/1tiiuyt/the_pc_port_of_twilight_princess_is_awesome/)
- [Original DrawnShade.ini](https://drive.google.com/file/d/1tTB4RP2DPs-4QwAFhFWH8jiFjTL1wm1A/view)
- [Comic.fx by Alexander Federwisch](https://github.com/Daodan317081/reshade-shaders/blob/master/Shaders/Comic.fx)
- License: BSD 3-Clause, included in Comic-LICENSE.txt.

The Comic color/chroma, surface and mesh-edge calculations are ported to WGSL.
DrawnShade's enabled color/chroma types, powers, fading and luma/saturation
weights are retained. Depth comes from Dusklight's resolved scene depth using
the active camera's near/far planes and reversed-Z flag. The width defaults to
one render pixel and is adjustable for both color and depth edges; the original
4K preset used two mesh iterations. Fine mesh detail is optional and off by
default for cleaner scenery. Reverse-Z linearization avoids subtraction of
far-plane-sized quantities, which produced distant striping during development.
Original
bloom, AmbientLight, eye adaptation, inferred depth/AO and contrast-stretch
passes are omitted. The game already supplies dynamic shadows. This is the
cel-outline part of the shared look, not a claim of screenshot equivalence.

## Twilight Princess community color presets

- [Enhanced Colors and Lighting v2 by SwordDeath](https://sfx.thelazy.net/games/preset/2085/)
  Uses the published LiftGammaGain, Tonemap, Vibrance and Curves values.
- [Better Lighting by SwordDeath](https://sfx.thelazy.net/games/preset/2359/)
  Uses the published Levels, DPX, LiftGammaGain, Curves and Sepia values.
- [ChromeFX by Magiike](https://sfx.thelazy.net/games/preset/3492/)
  Uses the published Tonemap, Vibrance and Sepia values. Identity LiftGammaGain
  is omitted along with its bloom/Gaussian/grain/vignette passes.

The original presets target older SweetFX/Dolphin. Only their color stages are
adapted here. Old one-based curve IDs and old defog units are preserved. The
native renderer's antialiasing and the player's existing sharpening are retained.
The selected preset has a shared 0–100% blending control.

[SweetFX source](https://github.com/CeeJayDK/SweetFX/tree/master/Shaders/SweetFX):
LiftGammaGain by 3an/CeeJay.dk; Tonemap, Vibrance, Curves and Sepia by CeeJay.dk;
DPX by Loadus. MIT license included in SweetFX-LICENSE.txt. WGSL adaptations
retain the upstream equations with bounds/zero guards appropriate to this host.
