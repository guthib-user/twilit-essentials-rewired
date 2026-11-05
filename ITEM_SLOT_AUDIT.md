# Five-slot native interaction audit

Target: Dusklight 2.0.3 `40457c6`, Dawnlight's optional KCRUX five-slot mode.
The five saved assignments remain canonical. Native indices 0/1 are Left/Right;
native index 2 is a transient projection of the active Up/Down/RT assignment.
Native `checkItemSetButton` instead uses 2 as its *absent* sentinel. Never return
a logical index from it or treat a fabricated 0 as a real item location.

| Native consumer | Dependence | Five-slot treatment | Verification |
| --- | --- | --- | --- |
| `checkNewItemChange`: oil bottle + lantern | Membership | Existing KCRUX lantern query searches all five | Isolated pour passed in revision 18 |
| `checkNewItemChange`: bait + fishing rod | Membership **and rod item lookup** | Scoped projection lends an unused native query position to the rod, without changing saved assignments | Isolated fixture passed with rod Down/bait RT and rod RT/bait Left; bottle bait consumed |
| `procFishingFoodInit` | Returns a rod button and retains it in `mSelectItemId` | Keep the scoped rod projection through the food procedure, then restore the true active slot | Native procedure 163 reached and returned to rod in both slot orders |
| `checkItemChangeFromButton` and `allUnequip` | Lantern auto re-equip loops only 0/1 (Dawnlight extends to native 2) | Project the actual lantern assignment into native 2 before the native re-equip | Isolated forced unequip re-equipped the Down-slot lantern |
| `checkItemSetButton`: Spinner/Heavy Boots | Membership; boots result also marks a native HUD button | Search all five for membership; direct-use marker uses projected item | Membership fixture passed for Down boots and RT Spinner; boots equipped/removed on Down press |
| `checkSetItemTrigger`: Spinner, Boots, Hookshot, Hawkeye, bomb/grab/swim variants | Trigger **and side-effect on `mSelectItemId`** | Existing KCRUX input projection routes the pressed logical slot through X/Y/Z; native item trigger remains paired with projected `mSelectItemId` | Existing focused targeting tests and isolated boots exercise passed; rare story-specific variants not individually playtested |
| `orderTalk`: trade items | Item lookup **and button-specific event code** | Temporarily project active extra-slot trade item and X trigger during item-talk event | Isolated Letter-on-RT fixture produced native item-talk event type 6; story-specific NPC not available in test save |
| `d_msg_flow::query021` equipped-item check | Fixed native selected-item count | Include canonical five-slot assignments in result | Isolated inactive fifth-slot Letter query returned true |
| Button Fishing rod actor | Optional rod lookup reads only native X/Y plus held button | Project held extra-slot rod to X during rod actor update | Rod actor present and input route exercised; reel feel still needs water-side playtest |
| Other direct `for i<2` selected-item loops | Varies | Audited individual consumers; retained native projections where they are required | Static audit of pinned Dusklight source |

The KCRUX item mix resolver already handles Bow+Bomb, Bow+Hawkeye and
Fishing Rod+Bait within a single logical slot. These must still be checked for
use after transformations, depletion and save/load.

Normal-profile saves and controller configuration must be preserved. Real NPC
trade dialogue, Spinner ride, and fishing at water still require later story
playtesting; isolated fixtures cover their core lookup and input routes.
