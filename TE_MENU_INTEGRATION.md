# Twilit Essentials menu integration boundary

The isolated candidate uses the official Twilit Essentials 3.0.1 package
(`17b40c012ad5a598a9aeaa24345c7708a96f8828c1052491b1518e6e83f654f3`).
It is the same package the user downloaded. Its `mod.json` exports
`com.dusklight.twilit_essentials.collection` v1.0, but that service only adds
Sword, Shield, and Tunic equipment entries. It cannot replace or extend the
Items tab, Settings tab, or the native three item assignment channels.

Rewired's five canonical assignments and combination behavior live in
`rewired-core/src/kcrux_slots.hpp` and `rewired-core/src/kcrux_controls.cpp`.
The stable boundary for a future native Items page is
`rewired-core/include/kcrux/controls.h` (`KcruxControlsService` v1.4):

* `get_slot(0..4)` provides item, mix, resolved icon item, availability, ammo,
  and oil state. Order is Left, Right, Up, Down, RT.
* `owner_of_inventory(index)` supplies the badge for each inventory cell.
* `assign(slot,index)` is the single user-menu transaction. It swaps an item
  already assigned elsewhere, clears it when assigned to itself, saves the
  result, updates the native item projection, and plays the matching cue.
* `report_collection_page(page)` reports the actual rendered Collection page to the input adapter, instead of guessing from bumper presses.
* `assignment_revision()` tells a menu when to refresh its view. Save loading
  and gameplay projections do not play assignment cues.

The current Essentials Advanced Collection adapter is limited to translating
D-pad/RT input into a highlighted native item cell and drawing five slots over
the existing Items page. It lives behind the `collection_page()` predicate in
`rewired-core/src/kcrux_controls.cpp` and
`rewired-hud/src/kcrux_prompts.cpp`. The HUD is a separate mod and reads the
core service. When Twilit Essentials source or a real Items-page extension API
becomes available, replace these two adapter points. Keep the slot model,
save data, gameplay hooks, and HUD layout intact. The new menu should call
the service above instead of writing native X/Y/Z channels directly.

The current package cannot hide Essentials' native third-slot Settings row,
or item descriptions through the published Collection
service. Revision 22 suppresses the extra Midna glyph through scoped HUD hooks,
without affecting the custom L3 portrait. A source-level integration should hide or rewrite those views only
while the modern mode is on; turning it off should restore the original page.
`deploy/rewired_launch.py` already supports that restart-time mode switch and
restores Essentials' original configuration in legacy mode.

The regular TP Custom game is not modified by this isolated workspace.

## Revision 22 adapter validation

The Items overlay records the twenty actual frame positions from the rendered
Essentials grid. Inventory badges and five assignment indicators use those
coordinates, and draw once in the main HUD screen pass to preserve the native
cursor projection. Bomb-bag ownership reads raw inventory slots 15–17; only the
owned bag's display tint is restored. No acquisition or save state is changed.

The custom minimap draw temporarily scales its dimensions and positions it above
the lower-left items. Map/Items labels follow the map's actual upper boundary.
The optional Essentials stamina service suppresses both navigation labels through
stamina use and recovery; the original native Midna icon and its button are masked
before Essentials captures their alpha. Gameplay Midna checks resolve only to L3.

## Revision 23 direct assignment boundary

The inventory adapter no longer synthesizes native X to request an assignment.
That route ran Essentials' three-channel swaps against compatibility mirrors and
could select the wrong item or undo another change. The HUD identifies the actual
rendered cursor cell against the captured grid, then calls
`assign_collection_selection(inventory)` once to consume the queued controller
request through the canonical five-slot transaction. The native setter is blocked
only on the modern Items page. Requests survive skipped presentation frames and
are cancelled when leaving that page. The slot service remains append-only.

The old native D-pad close shortcuts are filtered in the wheel, overworld map
(window 4), and dungeon map (window 5). Action bindings also resolve map access
from LB only, without letting the configured old shortcuts close those views.

## Revision 24 combinations and footer

`combination_state(inventory)` is appended to service v1.4: 0 means unavailable,
1 combine, 2 separate. It searches canonical ownership across all five slots.
`assign_collection_selection` now also continuously reports the highlighted
inventory entry and consumes a pending A combination request. The native A
consumer is suppressed only while a valid combination is available; ordinary A
behavior on other items/pages remains available. Both menus call the same
combination transaction, which owns persistence, projection, uniqueness and
combine-on/off sounds. Native wheel availability hooks use the same predicate.

The footer adapter hides native pictures/text in the assignment/combine group
only on the modern Items page, preserving Back and custom rendering. Native item
descriptions are deliberately retained; future source integration can rewrite
their legacy button references without changing the slot service. No GX-level
hooks or binary patches are included. Stamina no longer suppresses navigation.

### Revision 25 presentation follow-up

Native `dMenu_Ring_c::drawNumber` is replaced in modern mode with the same compact
HUD digits, using the incoming native item position so selected-item resizing is
respected. `setMixMessage` hides its obsolete parent and resets the pikari clock;
our canonical-service Y prompt remains responsible for availability. The hidden
Items-page footer changes drawPikari mode 2 (recurring combination ping) to mode 0
(silent), leaving actual transaction audio independent. Detached third-slot digit
suppression now includes window 2. Bomb-arrow slot views use arrow count/capacity,
not an unsupported resolved-item capacity or the minimum of arrows and bombs.
