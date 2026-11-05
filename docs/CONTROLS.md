# Controls and visual policy

| Input | Modern behavior |
|---|---|
| D-pad Left/Right/Up/Down | Human item slots |
| D-pad Up/Down in wolf form | Sense / Dig |
| RT | Fifth quick item; select/use/fire in the appropriate context |
| Y | Jump, overridden by contextual secondary actions such as Push or boomerang lock |
| X | Attack |
| B | Cancel, put away, dismount where applicable |
| A | Native main/context action; existing Manual Jumps behavior also supported |
| LT | Target/lock |
| LB tap / hold | Full map / minimap toggle |
| RB | Items |
| L3 / R3 | Midna / first person |

Contextual actions take precedence over Y jump. RT must remain available for
item use; do not reintroduce major native R secondary-action conflicts there.
The five displayed slots, both inventory frontends, and native consumers must
agree about an item's ownership.

## Cues

- Basic: label only.
- Minor: restrained available-button light. Place/Throw use this level.
- Major: opening flash and outward pulse for meaningful availability changes,
  including Pickup. Preserve the existing pulse duration and 0.75-second quiet
  cooldown. During the native action, hold the opening glow and suppress rings.
- Super: separate special display; do not recreate the retired central Push
  instruction or native duplicate Midna button/blue gradient.

Ordinary Put Away and similarly frequent actions should not gain Major cues.
Visual availability cues do not require new sounds. Combination availability
prompts are silent; actual combine/separate actions retain their sound.

Map/Items share normal HUD fading, stay visible during stamina use, and move
above the enlarged minimap as needed. Midna uses the existing emblem with an L3
badge; her layout accounts for Tears when shown. Preserve configurable frame
position/scale and user layout settings. Use existing suitable assets; do not
generate new artwork for these elements.
