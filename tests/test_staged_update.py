"""The optional visuals package must preserve transactional player updates."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("rewired_launch", ROOT / "deploy/rewired_launch.py")
launcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(launcher)
CONTROLS = "twilit_rewired_controls.dusk"
HUD = "twilit_rewired_hud.dusk"
VISUALS = "twilit_rewired_visuals.dusk"


class StagedUpdateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.profile = self.root / "profile"
        self.mods = self.profile / "mods"
        self.mods.mkdir(parents=True)
        self.pending = self.root / "pending"
        self.pending.mkdir()
        self.originals = {CONTROLS: b"previous controls", HUD: b"previous HUD"}
        for name, data in self.originals.items():
            (self.mods / name).write_bytes(data)
        for target, value in (("__file__", str(self.root / "rewired_launch.py")), ("running", lambda _: False)):
            context = patch.object(launcher, target, value)
            context.start()
            self.addCleanup(context.stop)

    def stage(self, names=(CONTROLS, HUD, VISUALS), test_driver=False):
        hashes = {}
        for name in names:
            with zipfile.ZipFile(self.pending / name, "w") as archive:
                archive.writestr("lib/linux-x86_64/mod.so", b"Rewired isolated integration driver" if test_driver else b"production")
            hashes[name] = hashlib.sha256((self.pending / name).read_bytes()).hexdigest()
        (self.pending / "manifest.json").write_text(json.dumps(hashes))
        return hashes

    def assert_unchanged(self):
        for name, data in self.originals.items():
            self.assertEqual((self.mods / name).read_bytes(), data)
        self.assertFalse((self.mods / VISUALS).exists())

    def test_first_optional_install_and_backup(self):
        hashes = self.stage()
        launcher.apply_staged(self.profile)
        for name, digest in hashes.items():
            self.assertEqual(hashlib.sha256((self.mods / name).read_bytes()).hexdigest(), digest)
        backup, = (self.root / "update-backups").iterdir()
        for name, data in self.originals.items():
            self.assertEqual((backup / name).read_bytes(), data)
        self.assertFalse((self.pending / "manifest.json").exists())

    def test_existing_two_package_update(self):
        self.stage((CONTROLS, HUD))
        launcher.apply_staged(self.profile)
        self.assertFalse((self.mods / VISUALS).exists())

    def test_invalid_checksum_preserves_previous_packages(self):
        self.stage()
        (self.pending / VISUALS).write_bytes(b"corrupt")
        with self.assertRaisesRegex(RuntimeError, "checksum"):
            launcher.apply_staged(self.profile)
        self.assert_unchanged()

    def test_test_driver_rejected(self):
        self.stage(test_driver=True)
        with self.assertRaisesRegex(RuntimeError, "test package"):
            launcher.apply_staged(self.profile)
        self.assert_unchanged()

    def test_running_game_is_not_modified(self):
        self.stage()
        with patch.object(launcher, "running", return_value=True):
            with self.assertRaisesRegex(RuntimeError, "Save and close"):
                launcher.apply_staged(self.profile)
        self.assert_unchanged()

    def test_partial_install_rolls_back(self):
        self.stage()
        replace = launcher.os.replace
        def fail_last(source, target):
            if Path(target).name == VISUALS:
                raise OSError("simulated replacement failure")
            return replace(source, target)
        with patch.object(launcher.os, "replace", fail_last):
            with self.assertRaises(OSError):
                launcher.apply_staged(self.profile)
        self.assert_unchanged()
        self.assertTrue((self.pending / "manifest.json").exists())

    def test_unknown_package_rejected(self):
        self.stage((CONTROLS, HUD, "unrelated.dusk"))
        with self.assertRaisesRegex(RuntimeError, "unexpected package list"):
            launcher.apply_staged(self.profile)
        self.assert_unchanged()


if __name__ == "__main__":
    unittest.main()
