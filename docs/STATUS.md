# Current status

Updated 2026-11-05. Revision **29** is staged for the next full launch of Rewired
Playtest: core **0.1.8**, unchanged HUD **0.1.4**, slot service **1.4**. Revision
**27** is currently installed. The launcher validates
the staged packages and retains a rollback copy before installation. Normal TP
Custom, the shared AppImage, player saves and unrelated settings are unchanged.

## Current behavior

Five canonical saved slots: D-pad Left, Right, Up, Down, RT. Menus and HUD share
that registry, including combinations and ammo. Modern mode is optional and
switches on restart. Dawnlight is disabled in this instance; Twilit Essentials
3.0.1 remains enabled. Its Items page is adapted through scoped hooks/overlay;
a full native Items/Settings extension is still unavailable in that package.

## Last release and verification

Revision 29 fixes the forward-only aiming walk slowdown. Native next-action
handling supplies an attention movement limit, but the forward branch uses
normal walking and changes to its normal limit only during animation selection,
after calculating speed. Rewired now supplies that same normal limit before
the native forward movement calculation. Acceleration, analog response,
deceleration and environmental caps remain native; targeted movement and Ball
and Chain pre-swing are excluded from this correction.

An isolated, full-graphics Bow test reproduced forward speed 9.1 before the
patch and 16.1 after it (the native ready-item limit), with sideways speed still
13. Partial-stick speed, stopping and right-stick aim without position change
passed in the actual Bow aiming procedure. Production build and core regressions
pass. Revision 29 includes revision 28.

Revision 28 corrects the full-map Warp input to **RT**, preserving Y for
Essentials' map marking. The user explicitly authorized RT in this menu context.
Trigger hysteresis and held-input suppression use the existing input machinery;
native warp eligibility checks remain intact. Production build and core checks
pass; marking/warp gameplay confirmation remains with the player.

Revision 27 restores left-stick movement with right-stick aiming for on-foot
Bow/Slingshot, Boomerang, Hookshot, Ball and Chain, and Dominion Rod subject
procedures. Scoped and mounted aiming are excluded. A live isolated Bow test
verified movement without changing aim, aiming without changing position, and
both simultaneously with modern graphics and Compass. The later-game weapons
share the hook but have not each been playtested.

Full-map RT now supplies native Z for Warp selection (corrected from revision
27's conflicting Y mapping). This restores the missing input without changing
native portal/story/temporary warp restrictions. It does
not insert a new dialogue choice into Midna's menu. A complete human warp trip
still needs player confirmation.

The enabled-by-default **Reverse Zora swim up/down** setting reverses physical
left-stick Y once during pad normalization, only in Zora underwater swim
procedures. Better Swimming receives that same normalized input. Surface
swimming, walking, menus and right-stick input are excluded. Production and
fixture builds pass; actual underwater feel remains a player playtest. Core
regressions pass; production packaging has the test driver disabled.

Revision 26 adds a hat-animation lifetime cleanup and a private, hash-verified
Dusklight runtime with larger renderer buffers. Two original player crash dumps
showed storage/index buffer exhaustion. Two isolated escort tests also failed
at hat animation calculation. The patched run logged cleanup and reached
interactive horseback gameplay after that transition with the player's graphics
configuration and Compass enabled.

A 60-frame test exceeded the stock vertex, index and storage limits with
successful uploads and no renderer validation failure. Player save/config hashes
were unchanged at installation. See [pins](pins.json) for release hashes and
[historical acceptance](history/acceptance-history.md) for earlier checks.

## Open work and limits

- **HUD recovery:** after Zora swimming, gameplay HUD vanished while inventory
  HUD remained visible. The player disabled Essentials' independent HUD auto
  fade and hard-reset; the HUD returned, and a live read showed full native
  button/life alpha. These overlapping changes do not prove the trigger or a
  code fix. Revision 27 does not force HUD visibility or alter normal fades.
- **Mounted third-person aiming investigation:** feasible using a scoped camera
  mode/view change plus native weapon yaw/pitch and trajectory sight. Mounted
  Bow/Slingshot bypass the on-foot subject procedure, so the new movement hook
  deliberately excludes riding. No mounted camera change is included yet.
- **Open:** loaded-game process exit crashed in `dRes_info_c::deleteArchiveRes`
  during resource teardown. Revision 26 does not claim to fix this.
- **Player playtest:** complete the escort past the previous crash point. Only
  its transition to horseback gameplay was verified automatically.
- Rare later-game interactions remain progression checks; see the
  [interaction audit](interaction-audit.md). Do not infer they were all played.
- Essentials' native third-slot Settings row and legacy wording in item
  descriptions remain integration limitations. The canonical assignment and
  combination behavior already work through the adapter.
- The private runtime patch supports the exact pinned Linux executable only.
  Its larger buffers are finite, not a dynamic/unlimited allocation system.

## Local evidence pointers

These are ignored local files; they are not required for ordinary code work:

- `build/revision26-installed.json`: installed files, hashes and rollback path.
- `build/revision26-crash-original/`: original save/config backup and crash dumps.
- `build/revision26-escort-patched.log`, `revision26-escort-state.json`:
  successful transition and animation-cleanup records.
- `build/revision26-buffer-stress.json`, `revision26-buffer-stress.log`:
  oversized upload check.
- `build/revision26-patched-debug.txt`: separate exit-time teardown failure.
- `build/revision27-staged.json`: candidate hashes and pending update location.
- `build/revision28-staged.json`: corrected RT map Warp candidate hashes.
- `build/revision29-staged.json`: forward aiming fix and cumulative candidate hashes.
- `build/revision29-aim-speed-baseline.json`, `revision29-aim-speed-patched.json`,
  `revision29-aim-speed-verification.json`: focused live aiming speed checks.
- `build/revision27-aim-movement.json`: simultaneous Bow movement/aim result.
- `build/revision27-core-build.log`, `revision27-test-build.log`: build checks.

The player's current save is authoritative. Never restore a test card over it.
