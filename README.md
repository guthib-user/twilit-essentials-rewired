# Twilit Essentials: Rewired

An unofficial Twilit Essentials companion with five saved item slots, modern
controller bindings, and a separate Rewired HUD. Current development targets
Dusklight 2.0.3 and Twilit Essentials 3.0.1 on Linux x86-64.

Start with [current status](docs/STATUS.md). It records the installed revision,
known issues, and what has actually been tested.

| Need | Read |
|---|---|
| Where behavior lives and what must remain consistent | [Architecture](docs/ARCHITECTURE.md) |
| Build, external assets, quick checks, isolated testing | [Build and testing](docs/BUILD.md) |
| Controller mapping and visual cue policy | [Controls](docs/CONTROLS.md) |
| Exact dependency and release identifiers | [Version pins](docs/pins.json) |
| Essentials menu integration boundary | [Menu integration](docs/essentials-integration.md) |
| Multi-item interactions and remaining progression checks | [Interaction audit](docs/interaction-audit.md) |
| Older experiments and detailed evidence | [History](docs/history/) |

`rewired-core/` owns controls and canonical assignments. `rewired-hud/` consumes
the core service and renders the HUD. `deploy/` contains launcher and installation
helpers; `tools/` contains current isolated test and renderer-patch tools.
`tests/` contains quick checks that run without the game SDK.

The repository contains source and small documentation/assets. Game images,
saves, credentials, third-party mod packages, SDK checkouts, compiled runtimes,
recordings, and crash dumps stay local. The controller bumper texture must be
provided from the user's installed pack; its source note is included. The
bundled ornament has its own MIT license. Dawnlight-derived portions retain
their [CC0 notice](THIRD_PARTY/Dawnlight-CC0.txt); no blanket license is asserted
for separately licensed assets.
