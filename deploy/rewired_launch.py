#!/usr/bin/env python3
"""Launch a profile with Twilit Essentials and Rewired in a coherent mode.

The mod's toggle is saved by Dusklight on exit. This launcher applies the
matching TE switches on the next start, before either mod initializes.
"""

import argparse
import fcntl
import json
import os
from pathlib import Path
import shutil
import sys
import hashlib
import zipfile
import subprocess
from datetime import datetime, timezone

APP = Path.home() / "Applications/Dusklight/Dusklight-v2.0.3-linux-x86_64.AppImage"
ROM = Path.home() / "Applications/Dusklight/Game data/Twilight Princess - Wii USA Rev 2.rvz"
DEFAULT_PROFILE = Path.home() / ".local/share/TwilitRealm/Dusklight"
PREFIX = "mod.com_dusklight_twilit__essentials."
TE_SETTINGS = {
    PREFIX + "collectionAdvancedDpadItems": False,
    PREFIX + "collectionAdvancedScreen": True,
    # TE's third native channel is the compatibility transport for Rewired
    # slots Up, Down and RT. Rewired hides the corresponding legacy HUD.
    PREFIX + "customZButtonEnabled": True,
}
MODE_KEY = "mod.local_twilit__rewired_controls.modern-controls"
REQUIRED_MODS = ("com.dusklight.twilit_essentials.dusk", "twilit_rewired_controls.dusk", "twilit_rewired_hud.dusk")
MISSING = {"missing": True}
RUNTIME_SHA256 = "43f45837a8d49061b3de4b765adab2b6ceba26f156a5fbc53fd7a5a9f5b4fc2e"

def runtime_for_profile(profile: Path) -> Path:
    # Optional private runtime beside this profile. The shared AppImage and
    # normal TP Custom shortcut are never patched or redirected.
    runtime = profile.parent / "runtime-v2.0.3" / "usr/bin/dusklight"
    if not runtime.exists():
        return APP
    if hashlib.sha256(runtime.read_bytes()).hexdigest() != RUNTIME_SHA256:
        raise RuntimeError("The private Rewired runtime failed its integrity check; restore its verified copy before launching.")
    return runtime

def apply_staged(profile: Path) -> None:
    pending = Path(__file__).resolve().parent / "pending"
    manifest = pending / "manifest.json"
    if not manifest.exists():
        return
    if running(profile):
        raise RuntimeError("Save and close the running Rewired game, then open this shortcut again to apply its update.")
    expected = json.loads(manifest.read_text())
    allowed = {"twilit_rewired_controls.dusk", "twilit_rewired_hud.dusk"}
    if set(expected) != allowed:
        raise RuntimeError("The staged Rewired update has an unexpected package list")
    for name, digest in expected.items():
        package = pending / name
        if hashlib.sha256(package.read_bytes()).hexdigest() != digest:
            raise RuntimeError("Staged update checksum failed: " + name)
        with zipfile.ZipFile(package) as archive:
            if b"Rewired isolated integration driver" in archive.read("lib/linux-x86_64/mod.so"):
                raise RuntimeError("An automated test package cannot be installed for player testing")
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    backup = pending.parent / "update-backups" / stamp
    backup.mkdir(parents=True)
    for name in allowed:
        shutil.copy2(profile / "mods" / name, backup / name)
        shutil.copy2(pending / name, profile / "mods" / (name + ".next"))
    replaced = []
    try:
        for name in sorted(allowed):
            os.replace(profile / "mods" / (name + ".next"), profile / "mods" / name)
            replaced.append(name)
    except BaseException:
        for name in replaced:
            shutil.copy2(backup / name, profile / "mods" / name)
        raise
    manifest.unlink()
    for name in allowed:
        (pending / name).unlink()


def running(profile: Path) -> bool:
    target = profile.resolve()
    for proc in Path("/proc").glob("[0-9]*/cmdline"):
        try:
            args = proc.read_bytes().split(b"\0")
            if b"--user-dir" not in args:
                continue
            index = args.index(b"--user-dir")
            if index + 1 < len(args) and Path(os.fsdecode(args[index + 1])).resolve() == target:
                if b"Dusklight-v2.0.3" in args[0] or Path(os.fsdecode(args[0])).name in ("dusklight", "AppRun"):
                    return True
        except (OSError, ValueError):
            continue
    return False


def atomic_json(path: Path, data: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_name(path.name + ".rewired-tmp")
    with temp.open("w", encoding="utf-8") as stream:
        json.dump(data, stream, indent=4, sort_keys=True)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temp, path)


def snapshot(config: dict) -> dict:
    return {key: config[key] if key in config else MISSING for key in TE_SETTINGS}


def restore(config: dict, baseline: dict) -> None:
    for key in TE_SETTINGS:
        value = baseline.get(key, MISSING)
        if value == MISSING:
            config.pop(key, None)
        else:
            config[key] = value


def sync(profile: Path) -> str:
    if running(profile):
        raise RuntimeError("This Dusklight profile is still running. Save and close it before launching again.")
    for name in REQUIRED_MODS:
        if not (profile / "mods" / name).is_file():
            raise RuntimeError(f"Required mod is missing: {name}")
    path = profile / "config.json"
    current = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(current, dict):
        raise RuntimeError("Dusklight config has an unexpected structure")
    state_path = profile / "mod_data/local.twilit_rewired.controls/launcher-state.json"
    state = json.loads(state_path.read_text(encoding="utf-8")) if state_path.exists() else {}
    baseline = state.get("te_baseline") or snapshot(current)
    if state.get("last_mode") == "legacy":
        # Any TE changes the player made in legacy mode become the next restore baseline.
        baseline = snapshot(current)
    mode = "rewired" if bool(current.get(MODE_KEY, True)) else "legacy"
    updated = current.copy()
    updated["mod.com_dusklight_twilit__essentials.enabled"] = True
    updated["mod.dev_bezide_dawnlight.enabled"] = False
    updated["mod.local_twilit__rewired_controls.enabled"] = True
    updated["mod.local_twilit__rewired_hud.enabled"] = True
    updated[MODE_KEY] = mode == "rewired"
    if mode == "rewired":
        updated.update(TE_SETTINGS)
        if state.get("layout_revision", 0) < 22:
            updated[PREFIX + "staminaBarY"] = int(current.get(PREFIX + "staminaBarY", 0)) - 4
            updated["video.lockAspectRatio"] = False
    else:
        restore(updated, baseline)
    if updated != current:
        backup_dir = state_path.parent / "config-backups"
        backup_dir.mkdir(parents=True, exist_ok=True)
        timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
        shutil.copy2(path, backup_dir / f"config-{timestamp}.json")
        atomic_json(path, updated)
    atomic_json(state_path, {"format": 1, "last_mode": mode, "te_baseline": baseline,
                            "layout_revision": 22 if mode == "rewired" else state.get("layout_revision", 0)})
    return mode


def main() -> int:
    parser = argparse.ArgumentParser(description="Launch Twilight Princess with coherent Rewired/TE settings")
    parser.add_argument("--user-dir", type=Path, default=DEFAULT_PROFILE)
    parser.add_argument("--sync-only", action="store_true")
    args = parser.parse_args()
    profile = args.user_dir.expanduser().resolve()
    profile.mkdir(parents=True, exist_ok=True)
    with (profile / ".rewired-launch.lock").open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        apply_staged(profile)
        mode = sync(profile)
        print(f"Twilit Essentials mode: {mode}")
        if args.sync_only:
            return 0
        runtime = runtime_for_profile(profile)
        os.execv(str(runtime), [str(runtime), "--user-dir", str(profile), "--backend", "vulkan", "--log-level", "info", str(ROM)])


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"Rewired launcher: {error}", file=sys.stderr)
        try:
            subprocess.run(["notify-send", "Twilight Princess — Rewired", str(error)],
                           timeout=3, check=False)
        except (OSError, subprocess.TimeoutExpired):
            pass
        sys.exit(2)
