# Twilight Princess controls workspace

The current development target is **Twilit Essentials: Rewired** plus its
separate **Rewired HUD**. They are built under `rewired-core/` and
`rewired-hud/`, and tested against an isolated Dusklight profile at
`test-profile-rewired/`. Dawnlight is disabled there. The normal TP Custom
profile is not modified while this candidate is in development. The older
Dawnlight revision history below is retained as the rollback reference, not
as the description of the active Rewired build.

Rewired owns five saved item slots (Left, Right, Up, Down, RT), gameplay
controls, and item-combination projections. The HUD reads the core's published
slot service. A separate launcher switches Essentials' compatible settings
when the modern mode is enabled and restores their baseline when it is off;
the mode switch takes effect on restart. The isolated candidate currently uses
the official Twilit Essentials 3.0.1 binary. Its Collection service supports
equipment entries but does not expose the Items or Settings page for direct
extension. See `TE_MENU_INTEGRATION.md` for the menu boundary and remaining
source-level work.

Current isolated checks have exercised native Y jump alongside the existing
A+LT+forward Manual Jumps mod, LT targeting, opening the advanced Items page,
five-slot display, and swap/self-clear from RT. The remaining old item
description prompts and third-slot Settings row require a native menu
integration. Do not install this candidate into the normal profile yet.

## Previous Dawnlight implementation and rollback reference

Dawnlight 5.4.2 fork on branch `kcrux-controls`, upstream base `b5e9046cd98bdd8593ab2cf219d72a0f61ef7213`. Exact host SDK is Dusklight 2.0.3 `40457c6adb381928e4b5fef6ed459ed291edd5e2`; Aurora is `3227d76c60e1e782ca576610bce61c9e7744d8be`. No host edits or dependency pin changes. Release build uses `KCRUX_TEST_DRIVER=OFF`.

The canonical five-slot registry owns D-pad Left/Right/Up/Down and RT. Explicit assignment swaps occupied slots or moves into an empty slot; assigning an existing item to itself clears that slot. Ownership is by inventory entry, including combination components. Native three-slot save fields are compatibility mirrors, never the source for the new HUD. The two old extra-slot indices migrate once into a versioned 12-byte SaveService blob per save slot and normal/Boss Rush mode. SaveService flushes blobs on a successful game save. Gameplay cannot commit assignment changes. Failed storage writes roll back the entire transaction.

The adapter maps the active higher logical slot through native item channel 2 without losing the canonical Up assignment. It intercepts native playback updates to prevent recursive clearing of unavailable items. Dynamic bottle contents and ammo resolve from inventory. Scene changes reset transient input state; held inputs are suppressed after blocking menus. RT selects its quick item outside aiming and fires the current supported aimed item inside aiming. X supplies native sword input, including under LT; Y uses independent Dawnlight raw input. The separate A Manual Jumps mod remains enabled. L3 is Midna and R3 first person. Bumpers are preserved.

Native J2D HUD rendering separates four bottom-left item slots from the top-right RT slot and XYBA face-button prompts. It uses native item art, digit textures, action strings and lantern oil meters. Wheel assignment badges use the same registry. Legacy three-slot prompts/meters are suppressed only in custom mode. GUI resources live in an owned expandable heap, text buffers are reused, and meter deletion/mod shutdown releases resources. Original behavior remains available with custom mode disabled.

Validation records are under `build/revision3-*.json`. The isolated driver is compiled out of production. Native integration exercised Down/Up/RT wheel swaps and self-clear, slingshot firing with/without LT, targeted X and Y, RT quick-bow equip/fire, unchanged assignments during gameplay, transitions native HUD rendering, and native game-save flushing of the five-slot blob. The slot model also passed all 25 source/destination pairs and 100,000 invariant operations. Later equipment, horseback, swimming and wolf contexts remain human progression tests.

Deployment uses the explicit normal-profile desktop launcher, checksum verification, a closed-game guard, fresh backups and one-time merged config updates. Subsequent updates preserve the controller profile and bumper settings. Original restore never rewinds current game saves. No background service is installed. See `outputs/Twilight-Princess-controls-playtest.md` for the user-facing map and checks.

Revision 4 adds standalone-text idle fade, a top-right shoulder-shaped RT badge, a slightly lower D-pad cluster, native assignment sounds and pulses, and native backward Y backflips/backhops. Runtime checks confirmed the backward-jump procedure with unchanged facing and complete idle fade. Settings/controller/saves were unchanged at installation.

Revision 5 moves native stow and horse dismount consumers to physical B using scoped input restoration, leaving physical A available to movement and its separate jump mod. Stow can be requested while moving; native eligibility checks still run. The HUD draws the native label under B. Native runtime checks confirmed sword stow while moving, A movement retaining the sword, and slingshot cancel/stow. Focused scoped-input tests covered dismount, nested restoration, menus and disabled mode.

Revision 6 provides native-text D-pad Senses/Dig indicators in wolf form, active-Senses highlighting, and a dedicated wolf input path independent of human slots. Item/RT displays return in human form. In a copied profile, runtime checks passed with all five human slots empty: Senses on/off, X attack and held charge, Right Dig, A movement, unused-input isolation and assignments preserved on return to human. Story unlocks used for the isolated fixture are excluded from the production build.

Revision 7 uses native item-light textures behind the wolf labels, moves those indicators closer/lower, and places native sword art inside X. Specific A/B/X/Y Pikari frame pointers redirect only native button highlights into the custom layout; unrelated HUD effects retain their coordinates. Native wallGrabTrigger/wallGrabButton consumers take raw physical Y instead of R/RT, with a context guard preventing Y jump during block/chain actions and a native action label at Y. No new bitmap/vector art is generated or bundled. Native texture views are owned by the meter archive, while cloned J2DPictures are released before meter deletion. An isolated headless Gamescope check keeps the user's display off. Tests and installation evidence are under build/revision7-*.

Revision 8 adds state-driven context-change notices using the existing ring art. New Talk/Climb/Grab/Push and native emphasized Dig availability produce one moderate 0.65-second outward fade; routine Put Away/Attack/Roll/Dash are excluded and unchanged prompts do not loop. Revision 7's ordinary Sense/Dig lighting stays. Only the central push/climb and emphasized remapped wolf helpers' J2DScreen draws are suppressed: the native dMeterButton update/draw function, animation clocks and all sound calls continue. Its native Pikari coordinates follow the custom A/Y locations. Render-only fixture contexts are compiled out of production and never alter game physics. Tests: kcrux_context_cues_test.py and hud_fade_test.py; native context, Talk and Dig captures.


Live recording harness (development only): `tools/run_candidate.py --headless` starts the copied profile in headless Gamescope, so the physical display can stay off. `tools/playcheck.py boot` opens the test save; `tools/record_live.py talk`, `tools/record_push8.py`, and `tools/record_dig.py` record focused native gameplay. Push/Dig scripts spawn original game actors before recording; Talk uses the village NPC. The driver supplies virtual controller input and native GPU readback, records monotonic frame timestamps, and exports state alongside each video. `pw-record` targets only the isolated game's PipeWire stream; no microphone/system fallback. `KCRUX_TEST_DRIVER=OFF` excludes all fixtures, actor setup and capture code from the shipped mod. Never copy the test profile over the normal profile. Launch guard prevents duplicate test instances; stop with `tools/playcheck.py quit` and wait for exit before rebuilding/relaunching. No background recording service is installed.

Revision 8 live validation: Talk cue on entering/re-entering conversation range and A starting native dialogue; native wooden movebox moves 150 world units through Y, RT does not grab; native sensed Dig emphasis raises a cue, then D-pad Right enters native wolf Dig procedure 306. The recorded tests use a copied save; player positioning and native actor spawning are test setup, not production features. State/timing evidence is in `build/live-*-states.json`. Video deliverables are in the workspace `outputs/` directory.


Revision 9 retains the 0.65-second context animation and adds reminders after a 0.75-second quiet cooldown (1.40-second start period), only while the native meaningful action is available. Native audio clocks/calls remain unchanged. A quick re-entry schedules its next cue instead of dropping it. Blank prompts are quiet. Focused coverage enumerates all 154 native status IDs through A/X, including human carry/key pickup and bug/lever aliases; it separately verifies B dismount and wolf emphasis. `outputs/Twilight-Princess-interaction-coverage.md` distinguishes shared status routing from native input channels outside the custom face/wolf indicators and from actual gameplay tests.

The final revision-9 policy excludes ordinary human/wolf pickup, lifting, taking and grass picking, including native aliases 31/52/57. A native existing-rock check passed grab-ready/up/carry procedures 108/109/113 using A; the earlier pickup pulse video is superseded by the final quiet policy. Native pickup Pikari colors are suppressed, while its original draw/timer/audio calls continue.


Revision 10 adds native-procedure-driven activity channels. Push/chain Y, Dig Right, active Senses Left, dismount B and mapped contextual A procedures hold the existing native Pikari opening artwork at frame 18 with a local clock, animation speed 0, draw mode 0 and elapsedFrames 0. This produces no extra sound calls or native frame-clock changes. Its owned parent alpha is temporarily multiplied by HUD alpha and restored after drawing. Expanding custom rings and the corresponding animated native flare colors are suppressed during execution; original native update/draw/audio paths continue. End-of-action availability gets a fresh 0.75-second quiet cooldown. No latch is inferred solely from raw button hold. Ordinary pickup procedures stay excluded. The test harness compiled out of release; the new Push video/state evidence is under outputs/build respectively.


## Visual cue vocabulary

- **Basic:** action text immediately above/below its mapped button; no flash or pulse.
- **Minor:** restrained availability light on the button, between plain text and a flash; no outward pulse.
- **Major:** opening full-button flash plus outward reminder pulses. While the native action executes, hold the full glow and suppress the ring; clear it on action end.
- **Super:** a separate on-screen instruction, dedicated interaction display or special symbol, such as the retired central Push helper or Midna's appearance.

In revision 19, pickup/lift/take/grass picking uses Major feedback; Place and Throw remain Minor. Native tutorial/central helper displays are a separate Super tier and were not globally removed for every interaction. Sound cues are independent of these visual tiers.


Revision 11 classifies pickup as Minor, reusing the same native opening-art sample at 0.14 alpha strength for a low steady available-button light, without native flash/custom ring. Native helper A/B/R pickup rows alone are temporarily hidden in their owned J2DScreen draws, with unrelated text/button rows preserved and visibility restored. The helper itself continues to update/draw and run native sound clocks; its pickup Pikari colors are suppressed. Major action states and reminder cadence are unchanged. The existing-rock live check passed native A pickup/carry, and focused tests cover mask scoping, companion preservation and alpha/visibility restoration. HUD auto-fade was disabled only in the copied recording profile for visibility and restored afterwards; normal-profile settings are unchanged.


Revision 12 corrects the incomplete pickup policy: the shared Minor carry family now owns pickup/lift/take/pick and Place/Throw, including native human/wolf carry animations. Their execution procedures are removed from the Major activity route, and low steady light plus pickup/carry-only helper masking remain through transitional no-label frames. The regression check exercises Pickup->Place->Throw prompt transitions, human/wolf execution procedures, masks and Major push preservation. No new recording was run, per the user's request. The installed production build excludes the test driver.


Revision 13 moderately compresses the face diamond and human item cluster, adds native TP swirl backing and LB/RB Map/Items hints using the existing Henriko Xbox shoulder artwork, and stacks wolf Senses/Dig at the lower left. Wolf raw Up/Down now route to native Senses/Dig; Left/Right cannot activate human equipment. Shared geometry drives images, labels and contextual/native glow positions. The original Midna button flare is made transparent by its exact clock-pointer identity without suppressing the separate Midna symbol or original audio/timers. All new pictures and standalone text share the existing fade scope.

LB uses a 0.55-second hold threshold: short release opens the native full map; a hold toggles the native minimap once, and its release does not open the full map. Blocking menus/events suppress stale held inputs. Twilight Compass defaults to hiding the native minimap, so this release explicitly migrates only `mod.com_dusklight_twilight__compass.showVanillaMinimap=true`. The native map is positioned above the item cluster while retaining native eligibility, slide state and alpha. Controller bytes, saves and all other settings are preserved.

The isolated revision-13 driver selects its named virtual controller instead of assuming device index zero; physical-controller hot-plug order must not invalidate the fixture. Production excludes the complete driver and diagnostics. Focused runtime checks and still captures cover human/wolf layouts, LB tap/hold, visible minimap, both wolf actions, unused directions, item ownership and idle fade. Two full-graphics 1080p videos were subsequently recorded at the user’s request; links are in the playtest guide. The recording profile copies current normal settings, overriding only pauseOnFocusLost for headless execution, with the complete normal mod stack.


Revision 14 increases wolf circles from 32 to 36 logical units (12.5%) and aligns their centres with the human Left slot, using 44 units of vertical spacing. LB Map/RB Items move to the middle-left column. The native minimap picture callback supplies its final scaled top edge and slide visibility; navigation moves above that edge and returns only after the map exits. Translation uses a short time-based smoothing envelope and resets with meter resources. Midna uses the existing `midona64.bti` portrait and native shaded controller-cap texture, with an L3 label under the face diamond. Native visibility/availability dims or hides the hint; HUD fade hooks own both pictures and text. No control, sound, assignment or config migration is required. No new artwork is generated.

Revision 15 removes the GC-specific backing ornament and moves Midna right, centering the 12-logical-unit gap between her 44-unit symbol and the 24-unit L3 cap beneath the A/Y axis. No input or setting changes. Release installed with the driver excluded; saves, controllers and settings verified unchanged. Native full-graphics video and fade/context checks passed.

Revision 16 places XandasLegend’s original four-circle Twilight-style HUD ornament behind the XYBA diamond, sized 100×98 logical units to align all four equal rings with the existing 27×29 button offsets. It is bundled unchanged as a BTI, with source and MIT license. Rendering borrows the game’s original wire-pane black/white and corner tint and applies the established 48% ornament alpha, multiplied by the HUD parent fade. The layout was inspected in human and wolf forms at the normal 3× graphics resolution, then deployed with no control, setting, or save changes.

Revision 17 moves the ornament down 2 logical pixels and enlarges it 4% by default. Dawnlight Settings → HUD now exposes independent saved horizontal offset, vertical offset and size controls for the custom frame, plus reset. Midna’s native portrait is reduced from 44 to 40 logical units without moving its centre. The native Wii D-pad Map/Items shortcuts are suppressed during custom-control gameplay, while native menus retain their D-pad input. Physical RB alone feeds the item-wheel shortcut; LB remains the map/minimap tap/hold action. The launcher migrates only the exact legacy controller mapping that wired item buttons to arrows; manually altered mappings are preserved. Isolated runtime checks reproduced the duplicate wheel/map activations before the fix, then verified all four D-pad slots without side effects, RB wheel, LB map and stable assignments. Release excludes the test driver.

Revision 18 adds independent horizontal and vertical horseback camera speed controls under Dawnlight Settings → Aiming. The riding-camera hook temporarily withholds right-stick input from the native fixed-rate and on-foot free-camera paths, then applies the saved horse-only speeds to the completed riding view; this leaves aim camera and on-foot sensitivity separate. A second hook extends the native lantern-equipped test from its original two slots to all five custom slots. The original pour animation, oil update and bottle consumption remain native. In the isolated game, lantern on Down and oil bottle on RT entered native pour procedure 184, filled oil from 0 to 7200 and retained assignments. The production build excludes the test driver. Horseback feel still requires a human ride check because the isolated fixture does not mount Epona.

Revision 19 promotes native Pick Up, Lift, Take, Pick and their observed aliases 31/52/57 to the existing Major cue. Availability shows the native opening flash and repeated outward ring; native pickup procedures keep the full button glow lit until the action ends. Place and Throw still use their subdued Minor light. The central pickup helper stays hidden, while native input, action text and sound timing are unchanged. A full-graphics isolated Ordon rock check displayed the Pick Up prompt at A, and the game entered its native grab-ready/up procedures on A. Focused cue tests cover the seven pickup statuses, human and wolf pickup procedures, reminder timing, and Place/Throw exclusions. No new video is needed for this already proven effect.

The same revision moves contextual R actions from RT to Y: boomerang target lock, bow/arrow switching, shield attack, and horseback defend/cover. Native R trigger/hold consumers take physical Y only while one of those statuses is active. The mapped Y label follows the native action, and the Dawnlight manual jump yields during the contextual action or active item aiming. RT continues to select/use/fire items. Focused checks cover each status with Y and RT independently, and the isolated game loaded both new native method hooks. A full target-lock interaction still needs normal progression playtesting.


Revision 22 (Rewired 0.1.1) fixes Essentials compatibility: actual rendered menu
page and item-cell geometry drive five-slot assignments and badges; cursor state
is preserved; owned bomb bags display correctly. Midna is exclusively L3 in modern
gameplay, with the duplicate native portrait/button removed. The minimap draws
above the items and positions navigation labels above itself. Essentials stamina
state hides Map/Items during use and recovery, then restores them. Stamina moves
up four logical units once on the next launch.

Player delivery remains the separate **Twilight Princess — Rewired Playtest**
shortcut. A staged update applies on its next launch after the running game is
saved and closed. The launcher verifies both production packages, backs up the
previous versions, preserves unrelated configuration and saves, and refuses to
replace files while that profile is running. Normal TP Custom remains unchanged.


Revision 23 (Rewired 0.1.2) replaces the inventory assignment proxy with a direct
five-slot transaction from the rendered cursor cell. Every slot supports swapping
and self-clear independently of the native X/Y/Z mirrors. Wheel and both map
screens suppress old D-pad closing shortcuts. Native Z base/text artwork is
suppressed in both detached picture draws and pane self-draws; native ammo pictures
cannot leak behind the custom RT item. Both minimap types are enlarged by about
14%, with the navigation column automatically moving above their new boundary.

## Revision 24 — five-slot menu combinations

Core and HUD 0.1.3 use slot service v1.4. The wheel consumes physical Y for
combine/separate and derives availability from all five canonical slots. The
Advanced Items page consumes A for the same transaction on the actual highlighted
component, suppressing Essentials' obsolete native mix write. Assignment input
remains D-pad/RT; native item descriptions remain unchanged. Footer assignment
instructions and symbols are removed through bounded native font/picture hooks;
the context-sensitive A Combine/Combo Off and B Back prompts remain.

Map/Items are 10% smaller and share the normal HUD alpha, with no separate
sprint/stamina suppression. Their existing minimap-aware position is preserved.
The independently drawn `j_light1` Midna gradient is suppressed along with its
already-hidden native portrait/button; the custom L3 portrait is retained.

Live full-graphics checks covered combine and separate with the bow in each of
five slots in both menus, including the Down/RT availability paths, and firing a
bomb arrow consumed both ammo types. Revision 24 is staged in the separate
Rewired Playtest launcher's pending directory; reopening after saving/closing
installs both production packages with automatic backups.

## Revision 25 — wheel counts and quieter combination prompts

Core/HUD 0.1.4 retain service v1.4. Bomb-arrow assignments explicitly report the
arrow count and capacity in every HUD/menu slot. The wheel uses the HUD's compact,
non-padded digits at the lower-left of each item, clear of its ownership badge.
Detached native third-slot digits are suppressed during the wheel as well as
normal gameplay, including when RT changes to a non-ammo item or becomes empty.

The wheel's canonical Y Combine/Combo Off prompt places Y on the left. Its native
R prompt updater is replaced, eliminating both its availability ping and its
independently drawn gradient. The Items footer's hidden combination flare uses
silent animation mode instead of the repeating audio mode. Actual combine and
separate transactions retain their existing ON/OFF sound calls. All changes are
conditional on modern mode; legacy rendering and availability cues remain native.

## Revision 26 — escort crash investigation and safeguards

Core 0.1.5 adds a hat-animation lifetime guard. Native `setItemMatrix` can
leave a calculator attached to cached model data when the next Link actor has
no corresponding demo animation wrapper. Clear that unowned root attachment
before native model calculation; active cutscene animations still use the
original entry path. Two unpatched isolated runs failed at hat model calculation
during the escort transition; the patched run logged cleanup and reached
interactive horseback gameplay in F_SP121.

The two original player crash dumps independently identify fixed renderer
buffer exhaustion: storage upload in `push_gx_draw`, and index upload in
`handle_aurora`. A private copy of the exact Linux Dusklight 2.0.3 executable
raises vertex/index/storage capacity from 5/2/8 MiB to 20/8/32 MiB. Uniform and
texture budgets remain 24 MiB. `tools/patch_renderer_capacity.py` verifies the
entire source executable hash and changes 19 audited allocation, capacity,
mapping and copy-offset sites together. It retains overflow checks. This is a
version-pinned capacity increase, not an unlimited/dynamic allocator.

The private runtime lives beside the Rewired profile as `runtime-v2.0.3`.
The launcher checks its hash before use. The shared AppImage and normal TP
Custom installation are untouched. No game files or runtime binaries belong
in this source repository; the patcher operates on a locally supplied runtime.

Validation: full graphics, compass and the latest copied player save reached
the escort's horseback gameplay; 60 frames uploaded 6 MiB vertices, 3 MiB
indices and 10 MiB storage per frame (each over the original limit), with all
service calls successful and no renderer validation failure. Save and config
hashes remained unchanged at installation. The rest of the escort mission is
reserved for the player's playtest, not marked complete.

Separate follow-up: closing the loaded escort test exposed an engine resource
archive teardown crash during process exit. It is recorded separately from the
in-mission crashes and is not claimed fixed by this patch.
