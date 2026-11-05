> Historical snapshots; later revisions supersede earlier behavior. Read
> [current status](../STATUS.md) before using these notes.

# Control and prompt acceptance checklist

## Revision 26 crash checks

| Check | Result |
|---|---|
| Original player crash traces | Both abort in fixed GPU staging buffer growth; storage and index paths identified |
| Escort cutscene failure | Reproduced twice in the isolated copy, including with enhanced shadows off |
| Animation cleanup | Patched log records orphaned hat calculator removal; escort transition reaches F_SP121 horseback gameplay |
| Expanded renderer layout | Runtime reads confirm 20/24/8/32/24 MiB capacities; 19 pinned patch sites checked |
| Buffer stress | 60 frames with extra 6/3/10 MiB vertex/index/storage uploads, all service results MOD_OK; no validation failure |
| Graphics, saves, isolation | Original graphics config retained; player save/config hashes unchanged; shared AppImage unchanged |
| Full escort mission | PLAYER PLAYTEST — not completed by automation |
| Process-exit resource teardown | OPEN — isolated loaded-game quit crashed in deleteArchiveRes; distinct from reported gameplay failure |

## Rewired candidate status (isolated profile)

| Check | Evidence | Status |
|---|---|---|
| Five canonical slots and inventory swap/self-clear | RT assignment in Advanced Items changed `[0,1,2,3,4]` to `[4,1,2,3,0]`, then cleared RT to `255` on a second press | PASS |
| Item art and owner badges | Full-graphics isolated capture shows five item indicators aligned to the inventory grid | PASS |
| Native Y jump plus existing Manual Jumps | Y entered jump procedure 21; A+LT+forward also entered procedure 21 with the external mod enabled | PASS |
| LB/RB and LT | Revision 22: RB opens wheel (window 2), short LB release opens full map (window 4), B backs out through native map zoom levels and closes both menus; held LB toggles minimap without calling Midna. LT native lock previously verified | PASS |
| Native five-slot Items and Settings pages | Essentials 3.0.1 exposes no Items/Settings extension; temporary overlay still leaves old description text and third-slot Settings option | PENDING |
| Modern/legacy mode restart switch | Launcher sync and baseline restore passed in a temporary profile; isolated game booted in legacy mode with the original Essentials Items footer and no Rewired indicators | PASS |
| Normal TP Custom game | Deliberately untouched while development continues | PASS |

The remaining checklist below comes from the earlier Dawnlight implementation
and is retained to guide final Rewired playtesting.

Use an isolated profile and disposable test save. Record PASS, FAIL, or NOT TESTED with a brief observation; do not infer runtime success from compilation.

| Context | Functional check | Visual check |
|---|---|---|
| Movement | A roll/sprint, contextual interaction, selected jump provider, LT targeting | Correct action labels; no misleading duplicate prompts |
| Combat | X drawing/attacks/combos, targeted attacks, jump attack, damage/cancel transitions | X sword prompt; correct cancel prompts |
| Dialogue/shop | A confirm, B cancel; no attack after closing a window | A/B and stick glyphs readable and correct |
| Item wheel | Assign/use each supported slot; combine/uncombine; all menu directions usable | D-pad direction hints, items, ammo/oil and mixture state agree |
| Map | LB opens full map; close/back works; minimap behavior is explicit | Correct map and close button labels |
| Bow/slingshot | Original input and RT independently fire; held-input overlap; no double shot; ammo/empty/reload | RT action correct only in valid aim states |
| Other aimed tools | Clawshot/boomerang/rod/ball hold, fire, target, return/control behavior preserved | Tool-specific prompts agree with the action |
| Horseback | Camera, aim, item use, sprint and interaction | No accidental first-person prompt or misleading fire label |
| Other forms/activities | Wolf, swimming, fishing, carrying and scripted actions preserved where applicable | No prompts advertising unavailable abilities |
| Lifecycle | Scene change, pause, focus loss, disconnect, reconnect, disable/unload, save/load | No stuck prompts or stale item icon |
| Fourth slot | Selection, mixed items, persistence, missing-item and disable behavior | Fourth direction/icon/count is complete |

Established controls: L3 Midna, R3 first-person, Y contextual jump/secondary action; LB tap map / hold minimap, RB items. Existing saved item directions are retained.

## Revision 22 final patch run

Validated on the copied player save in the isolated full-graphics profile, with
modern aspect ratio, Henriko replacements, Compass, Manual Jumps, Essentials 3.0.1,
and Rewired. Production packages exclude the isolated virtual controller.

- Items assignment swaps and assigning to self clears; real rendered page determines input routing.
- Inventory badges follow the grid; cursor corners remain joined.
- Acquired bomb bag is bright, unowned bags remain dim; raw inventory is unchanged.
- LB/RB do not call Midna; L3 opened actual Midna dialogue.
- Native blue Midna glyph and portrait under stamina are absent; right-side custom L3 portrait remains.
- Minimap sits above items; Map/Items labels sit above map without overlap.
- Sprint and stamina recovery hide Map/Items labels; recovered HUD restores them.
- Stamina shifts up four layout units once; later manual position changes survive relaunch.
- Staged updater rejects corrupted packages, preserves previous versions, installs production files, and preserves unrelated settings. Legacy mode restores its saved Essentials baseline.

Evidence: `build/revision22-*-results.json` and `build/revision22-evidence/`.
Native Essentials description strings and its third-slot Settings row remain a
known integration boundary, documented in TE_MENU_INTEGRATION.md. This is a
focused patch, not a claim that all later-game interactions have been played.


## Revision 23 reported-regression checks

Used a read-only copy of the player's saved Goron Mines progress; no live save or
normal TP Custom files were changed. Full modern graphics and Compass remained on.

- All five inventory destinations: assign and self-clear passed individually.
- Each of five source slots to each other destination and back: 20 pairs / 40
  direct inventory-menu transfers passed, preserving the displaced assignment.
- Moving the highlighted cell to Lantern assigned the lantern, then cleared it,
  instead of using a stale native cursor/channel.
- Wheel stayed open on all four D-pad inputs; dungeon map (window 5) stayed open
  on all four; overworld map (window 4) stayed open on Left and Right.
- RT bow fired an actual arrow, reducing ammo 19 to 18. Captures before/after and
  after changing back to sword show one correct RT count and no detached digits.
- Legacy blue Z base/text are absent from the wheel and under the stamina bar.
- Both overworld and dungeon minimaps use the increased .74 scale (formerly .65).
  Their bottom boundary is fixed above items; navigation follows the top boundary.

Evidence: build/revision23-*-results.json and build/revision23-evidence/.
All new behavior is guarded by modern mode; the slot-service API is append-only
v1.3. The native Essentials source/menu-description integration boundary remains.

## Revision 24 checks

- Full graphics, modern aspect ratio, Compass and Essentials enabled in isolated profile.
- Items A combine/separate: bow in Left, Right, Up, Down, RT all passed.
- Wheel Y combine/separate: bow in all five slots passed, menu remained open.
- Canonical bomb combination resolves to item 89 with bomb inventory 15 and bow mix 4.
- Actual Down-slot bomb-arrow firing reduced arrows 18→17 and bombs 26→25.
- Final screenshots show clean assignment footer, A Combo Off/Combine and B Back.
- Final sprint screenshot shows stamina, smaller navigation and minimap together.
- Former Midna blue gradient absent; L3 portrait preserved.
- Production archives 0.1.3 exclude the integration driver; hashes checked before staging.

Evidence: `build/revision24-combination-results.json`, `build/revision24-shot.json`,
`build/revision24-evidence/`, `build/revision24-delivery.txt`.
Bomb combinations were live-tested; the shared predicate also preserves existing
Hawkeye and rod-component support, without claiming live coverage of every later
item. Previous revision 23 packages remain in `dist/revision23`.

## Revision 25 checks

- Full modern graphics, wide aspect ratio, Compass and Essentials in isolated profile.
- Combine/separate in both menus with the bow in all five slots: ten cases passed;
  combined views equal the arrow count for each slot.
- Wheel RT changed through bow (16), bombs (24), slingshot (0), bottle, boomerang,
  and empty; non-ammo and empty assignments have no count. Screenshots inspected
  for the detached native-digit leak and count/badge separation.
- Native wheel counts use compact bottom-left digits; Y is left of Combine/Combo
  Off. The old native combination gradient and availability updater are disabled.
- With bow on native-compatible Left, the live Items footer audit observed 432
  silent flare calls and zero audio-enabled calls. The availability check was
  exercised explicitly; Down-only ownership does not trigger the native flare.
- Actual combined Left-slot shot: arrows 16→15 and bombs 24→23, displayed count 15.
- Actual combine/separate sound calls remain unchanged. Audio output itself was
  not independently captured; silence is verified at the native flare mode.
- Production archives exclude the integration driver and report revision 25.

Evidence: `build/revision25-combination-results.json`, `build/revision25-rt-results.json`,
`build/revision25-flare-audit.json`, `build/revision25-shot.json`, and
`build/revision25-evidence/`. Revision 24 packages remain available for rollback.
