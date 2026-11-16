# Essentials 3.0.2 integration and playtest plan

## Upstream check (2026-11-16)

The [Twilit Realm listing](https://twilitrealm.dev/mods/com.dusklight.twilit_essentials/)
publishes Essentials **3.0.2** (November 6). The official download matches the
package already installed in Rewired Playtest by SHA-256. The previous test
profile had 3.0.1; the menu pass uses 3.0.2 with the player’s copied configuration,
Compass, and modern graphics. [Dusklight's releases](https://github.com/TwilitRealm/dusklight/releases)
still list **2.0.3**, our current host. Its private renderer patch still matches
the pinned executable; there is no reason to change it for this mod update.

3.0.2 adds world/dungeon maps, Midna and more menus to the Advanced UI; moves
Collection tab navigation to the bumpers; adds an optional Advanced Item Wheel,
a preset picker, and wider localization. It also changes map markers, rewrites
dynamic shadows, and lists fixes for third-person horseback aiming, Slingshot
aim alignment, Tears indicators, and Lakebed water. Use these as regression
targets. None of those release notes alone proves a previous Rewired issue gone.

The [advertised source repo](https://github.com/F1mmel/dusklight-twilit-essentials)
returns 404 on the web and through GitHub's repo and fork APIs. I checked the
author's public repos, GitHub Pages, the release index and repository searches
including forks. The cached [old release index](https://github.com/F1mmel/dusklight-twilit-essentials/releases)
ends at 2.2.0. The [kamilink7 source snapshot](https://github.com/kamilink7/twilit-essentials-patch/tree/patch)
is 1.0.6, from before Advanced UI. Other matches are companion mods, host
forks, or ports. I found no accessible 3.0.2 implementation; the catalog lists
no license for the package.

The useful find is the author's separate [TE Service Examples repository](https://github.com/F1mmel/te-service-examples/tree/03397540b4418d097daa0c4c95fa1407fde66def),
updated November 15. It contains public service headers and working examples,
including Quick Access and an extra Midna menu option. The examples' newest
interface versions are ahead of the current downloadable package:

| Interface | 3.0.2 export | Public examples | Rewired use |
|---|---|---|---|
| Collection | 1.0 | 1.1 | Sword/shield/tunic equipment only. No Items tab, page events or assignment callbacks. |
| Localization | 1.0 | 1.0 | Custom translations; no ownership of menu input. |
| Stamina | 1.3 | 1.3 | Existing optional integration, independent of slots. |
| Quick Access | 1.0 | 1.2 | Optional quick actions. Custom Quick Access pages require 1.2; these are not equipment slots. |
| Midna Flow | Absent | 1.0 | Future way to add an eligible Warp entry to Midna's menu when a release exports it. |

The official download is still a compiled package. Use the examples as a
supported extension guide, check the actual exported version before relying
on a method, and keep the Essentials binary separately installed.

## Ownership and rollout

1. **This pass:** keep the five saved slots in the core service. Reserve RT for
   slot five in Items and the wheel, and for portals on the world map. In the
   pause UI use LB/RB for outer tabs. The Collection inner row gets LT/RT,
   while the other tabs use LT previous and RT next where free. The item wheel
   also accepts LT for direct select. Keep Y marking, X marker style, A/B map zoom/cancel, stick
   navigation, and the game’s warp restrictions. Let LB close standalone maps.
   Scope prompt substitution to existing glyph assets on the map/tab rows.
2. **Next upstream integration:** when a release exports Midna Flow, prototype
   a Warp menu entry through that API with the existing game eligibility
   checks and native map flow. Evaluate Quick Access independently if a
   shortcut page is useful. Neither needs to own the five saved assignments.
   For Items/Map/Settings, look for a public page API or licensed 3.x source.
   Migrate one view at a time and retire its compatibility hook once proven.
   Keep the slot format and canonical transactions intact.
3. **Release gate:** keep a rollback copy, verify production controller
   fixtures are off, and stage to Rewired Playtest only. Confirm player save
   and normal TP Custom were untouched. Treat player feedback on a new game
   state as a new reproduction, even if the same button worked in Ordon.

## Playtest order

| Priority | Route | Verify |
|---|---|---|
| P0 | Load and exit | Open a copied real save with the user's graphics and Compass. Try a normal quit and compare logs if the historical resource teardown crash returns. Match the private-runtime hash before testing renderer capacity. |
| P0 | World map | LB tap opens, LB hold only toggles minimap; LB closes standalone map. Test A/B zoom, stick move, X style, Y add/remove, and D-pad not closing. Open portals with RT, back out with RT/B, then select and confirm a real destination in an eligible area. Pre-unlock and restricted areas still reject the warp. |
| P0 | Pause and dungeon map | Cycle tabs both ways, including Map → Items and Map → Collection. RT in Map must never reassign an item. In a multi-floor dungeon, check floor selection and zoom before leaving both map routes. Check the Midna page in human and wolf forms, including L3 entry/exit and no extra gameplay portrait. |
| P0 | Items and wheel | Assign, swap and clear in all five slots in both screens. Use partial and held triggers across screen transitions, and check that no item fires on exit. Check pause overlays, empty cell badges, ammo, oil, bags and the optional Advanced Item Wheel. Preserve the quiet combination prompt and the action's sound. |
| P1 | Item interactions | Bomb arrows and rod+bait across RT/Down and the original slots; lantern+oil, Spinner, Iron Boots, trade item query. Confirm counts and persisted assignments after restarting. See [interaction audit](interaction-audit.md). |
| P1 | Gameplay bindings | Human: A action, X sword, B cancel, Y jump/Push/boomerang target, LT lock, RT select/fire, four D-pad slots, LB/RB, L3/R3. Wolf: Up Sense, Down Dig, no human slot firing. Re-run contextual cues (Pickup, Place, Push, Talk, Dig) and Tears placement. |
| P1 | Camera and state changes | On-foot forward/sideways aiming and RT fire; see whether upstream third-person horseback aim now handles steering and free aim without targeting. Verify Zora reversal with Better Swimming and inspect HUD after surfacing with auto fade off and on. |
| P2 | Presentation and modes | Alternate glyph packs/languages, resizing, keyboard and mixed input, legacy mode. Then save reload, cutscene-to-gameplay and the escort transition in the full graphics profile. Online/ranked modes are separate compatibility checks. |

Use `tools/check_core.py` for slot invariants and input edges. The fixture
checks menu routing without playing missions. Complete real destinations,
story transitions, and feel checks on the player's schedule. Record actual
passes, expected refusals and untested cases separately.
