# Five-slot native interaction audit

Original audit: Dusklight 2.0.3 `40457c6`, originating in Dawnlight's optional
five-slot implementation. Most entries retain their original verification;
they are not proof that every hook survived the move into Rewired. Revision 33
corrects the missing Boots/Spinner membership hook described below. Consult
[current status](STATUS.md) for open work.
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
| `checkItemSetButton`: Spinner/Heavy Boots | Membership; boots result also marks a native HUD button | Revision 33 restores the five-slot search omitted from the Rewired port. A safe native marker is returned for extra slots; visible HUD uses canonical ownership | Revision 33: 28 live checks on Essentials 3.0.2 and another 28 on 3.1.3 cover Boots on all five slots, held/released input, sword drawing, toggling off and clearing assignment. Spinner shares membership handling; riding is not newly tested |
| Essentials per-frame Boots cleanup | Independently scans only three native projections, outside Link execution | Reject false removal only while equipped Boots remain canonically assigned but absent from the three projections. Native player execution and event/death/wolf/prohibited-stage transitions retain removal authority | Down/RT Boots previously disappeared after sword drawing; now retained. Explicit toggling and unassignment still remove them. Water/mount/transform behavior needs player confirmation |
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
