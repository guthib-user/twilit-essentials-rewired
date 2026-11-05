# Current status

Updated 2026-11-05. Revision **26** is installed in the separate Rewired Playtest
instance. Core **0.1.5**, HUD **0.1.4**, slot service **1.4**. The normal TP Custom
instance and shared Dusklight AppImage were preserved. This documentation update
does not change the installed game.

## Current behavior

Five canonical saved slots: D-pad Left, Right, Up, Down, RT. Menus and HUD share
that registry, including combinations and ammo. Modern mode is optional and
switches on restart. Dawnlight is disabled in this instance; Twilit Essentials
3.0.1 remains enabled. Its Items page is adapted through scoped hooks/overlay;
a full native Items/Settings extension is still unavailable in that package.

## Last release and verification

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

The player's current save is authoritative. Never restore a test card over it.
