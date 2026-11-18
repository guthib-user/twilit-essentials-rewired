# Visual preset sources

Version 0.1.2 executes DrawnShade using original FX files through a private
vkShade runtime. The earlier native partial Comic recreation was removed.
The three other looks remain native adaptations of the color stages below.

## DrawnShade - original FX (no bloom)

Preset author: durchfallalarm.
- [Original post](https://www.reddit.com/r/pcmasterrace/comments/1tiiuyt/the_pc_port_of_twilight_princess_is_awesome/)
- [Original INI](https://drive.google.com/file/d/1tTB4RP2DPs-4QwAFhFWH8jiFjTL1wm1A/view)
- [Comic, Alexander Federwisch](https://github.com/Daodan317081/reshade-shaders): BSD 3-Clause.
- [Vignette, CeeJay.dk](https://github.com/CeeJayDK/SweetFX): MIT.
- [EyeAdaption, brussell](https://github.com/brussell1/Shaders): author license in original file.
- [Lumenite Kernel/RTAO, Afzaal (Kaido)](https://github.com/umar-afzaal/LumeniteFX): AGNYA; provisioned from official links for local use, not redistributed by this repository.
- [Depth Cues, Jose Negrete / BlueSkyDefender](https://github.com/BlueSkyDefender/AstrayFX): original file terms; downloaded unmodified.
- [ContrastStretch, Lord Of Lunacy](https://github.com/LordOfLunacy/Insane-Shaders).
- [ReShade includes, crosire](https://github.com/crosire/reshade-shaders).

The seven non-bloom techniques run in original order with original preset values.
AmbientLight (glow/lens/dirt), OrtonBloom and PD80 Bloom are excluded. Existing
in-game bloom remains untouched. Depth Cues' internal filtering is retained.
Original mesh settings are width/iterations 2 and enabled. Optional mesh controls
are explicit user changes, not hidden defaults.

The INI has no shader revision pins. The source manifest records compatible
upstream revisions actually used; it cannot establish which commits the author
installed. Original source hashes are verified locally. FX execute on the final
presented image, including HUD pixels. The author's texture pack/display are not
bundled and exact screenshot equivalence is not claimed.

Runtime: [slobodaapl/vkShade](https://github.com/slobodaapl/vkShade), derived from
vkBasalt and using the original ReShade compiler. Rewired's separate compatibility
patch handles texture sharing, supported device features, config isolation and
preset control. It does not rewrite shader algorithms. See docs/VISUAL-PRESETS.md
and original-fx-sources.json in the source repository for versions and checks.

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
