#!/usr/bin/env python3
"""Stage Rewired beside the existing TP Custom launcher, with a rollback copy."""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import shutil
import sys
import zipfile

from rewired_launch import DEFAULT_PROFILE, running

ROOT = Path(__file__).resolve().parents[1]
PACKAGES = {
    "twilit_rewired_controls.dusk": ROOT / "build/rewired-core/mods/twilit_rewired_controls.dusk",
    "twilit_rewired_hud.dusk": ROOT / "build/rewired-hud/mods/twilit_rewired_hud.dusk",
}
LAUNCHER = Path.home() / "Applications/Dusklight/Rewired/rewired_launch.py"
DESKTOP = Path.home() / ".local/share/applications/twilit-essentials-rewired.desktop"
BACKUPS = ROOT / "build/rewired-install-backups"


def copy_atomic(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_name(target.name + ".rewired-install-tmp")
    shutil.copy2(source, temporary)
    os.replace(temporary, target)


def write_atomic(target: Path, contents: str) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_name(target.name + ".rewired-install-tmp")
    temporary.write_text(contents, encoding="utf-8")
    os.replace(temporary, target)


def files(profile: Path) -> dict[str, Path]:
    return {
        "core": profile / "mods/twilit_rewired_controls.dusk",
        "hud": profile / "mods/twilit_rewired_hud.dusk",
        "config": profile / "config.json",
        "launcher_state": profile / "mod_data/local.twilit_rewired.controls/launcher-state.json",
        "launcher": LAUNCHER,
        "desktop": DESKTOP,
    }


def validate(profile: Path) -> None:
    if running(profile):
        raise RuntimeError("The normal Dusklight game is still open. Save and close it first.")
    if not (profile / "mods/com.dusklight.twilit_essentials.dusk").is_file():
        raise RuntimeError("Twilit Essentials is missing from this profile")
    if not (profile / "config.json").is_file():
        raise RuntimeError("Dusklight config is missing")
    for name, source in PACKAGES.items():
        with zipfile.ZipFile(source) as package:
            if not {"mod.json", "lib/linux-x86_64/mod.so"}.issubset(package.namelist()):
                raise RuntimeError(f"Invalid built package: {name}")


def install(profile: Path) -> Path:
    validate(profile)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    backup = BACKUPS / stamp
    backup.mkdir(parents=True)
    originals = files(profile)
    manifest = {"profile": str(profile), "files": {}}
    for key, path in originals.items():
        existed = path.is_file()
        manifest["files"][key] = {"path": str(path), "existed": existed}
        if existed:
            shutil.copy2(path, backup / key)
    write_atomic(backup / "manifest.json", json.dumps(manifest, indent=2) + "\n")
    copy_atomic(PACKAGES["twilit_rewired_controls.dusk"], originals["core"])
    copy_atomic(PACKAGES["twilit_rewired_hud.dusk"], originals["hud"])
    copy_atomic(ROOT / "deploy/rewired_launch.py", LAUNCHER)
    LAUNCHER.chmod(0o755)
    write_atomic(DESKTOP, "\n".join((
        "[Desktop Entry]", "Type=Application", "Name=Twilight Princess — Rewired",
        "Comment=Twilit Essentials with optional five-slot controls",
        f'Exec=/usr/bin/python3 "{LAUNCHER}"',
        f"Icon={Path.home() / 'Applications/Dusklight/dusklight.png'}",
        "Terminal=false", "Categories=Game;AdventureGame;", "StartupNotify=true", "",
    )))
    return backup


def rollback(backup: Path) -> None:
    manifest = json.loads((backup / "manifest.json").read_text(encoding="utf-8"))
    profile = Path(manifest["profile"])
    if running(profile):
        raise RuntimeError("The Dusklight game is still open. Save and close it first.")
    for key, entry in manifest["files"].items():
        target = Path(entry["path"])
        if entry["existed"]:
            copy_atomic(backup / key, target)
        else:
            target.unlink(missing_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--user-dir", type=Path, default=DEFAULT_PROFILE)
    parser.add_argument("--rollback", type=Path)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.rollback:
        rollback(args.rollback.expanduser().resolve())
        print("Rewired installation restored from", args.rollback)
    elif args.dry_run:
        validate(args.user_dir.expanduser().resolve())
        print("Installation ready; the existing TP Custom launcher will be left alone.")
    else:
        backup = install(args.user_dir.expanduser().resolve())
        print("Installed Rewired packages and a separate launcher. Rollback copy:", backup)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print("Rewired installer:", error, file=sys.stderr)
        sys.exit(2)
