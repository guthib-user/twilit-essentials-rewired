# Working on Rewired

Read `docs/STATUS.md` first. Open `docs/ARCHITECTURE.md` only when choosing an
implementation location, then read the specific files needed for the task.
`docs/pins.json` contains dependency identifiers. Do not load all history,
logs, vendor code, or the whole source tree into context by default.

## Working rules

- Keep the player's normal **TP Custom** instance and saves intact. Development
  uses a separate copied profile. Check which profile a launcher targets.
- The user's latest instructions override historical plans. They prefer to do
  manual playtesting; use focused automated checks when needed, and do not
  spend time playing through missions unless asked.
- Production core builds must have `REWIRED_TEST_DRIVER=OFF`. Never install a
  virtual-controller fixture package into the player's profile.
- All five slots share one canonical registry. Native X/Y/Z values are
  compatibility projections. The HUD and menu must call the core service.
- Preserve saved assignments, combination ownership, original audio cues,
  settings, and mode-switch behavior unless the task explicitly changes them.
- Do not generate replacement artwork. Use appropriate existing assets and
  retain their licensing/source notices.
- Build only the affected component and run relevant checks. A docs-only change
  does not need a game launch. Follow `docs/BUILD.md` for commands and deployment
  caveats; the general installer defaults to a different profile from Playtest.
- Do not commit ignored game data, saves, credentials, compiled packages,
  runtime binaries, recordings, dumps, or the locally licensed bumper texture.
- The renderer patch is pinned to one executable hash; never guess offsets or
  disable bounds checks to accommodate a different release.
- Update `docs/STATUS.md` after a release, preserving the difference between
  verified behavior, user playtesting, and open issues. Record pins when changed.

## Local data (not in Git)

`build/` holds build trees, verification records and backups; `dist/` holds
release packages. `vendor/` holds the pinned SDK. `test-profile-rewired/` is
the disposable test profile. `dawnlight/` and older tools are rollback/research
material. Search one relevant filename or revision prefix instead of recursively
reading these folders. They are not part of the distributable source.

On the development machine the player shortcut is **Twilight Princess — Rewired
Playtest**. Local installation paths are recorded in ignored `local/paths.json`.
Resolve them there instead of searching the entire home directory.
