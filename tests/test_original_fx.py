import hashlib
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
def load(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module
provision = load('provision', 'tools/provision_original_fx.py')
bridge = load('bridge', 'deploy/rewired_visuals.py')

class OriginalFxTests(unittest.TestCase):
    def test_enabled_chain_only_and_exact_values(self):
        chain = provision.EXPECTED.copy()
        chain.insert(1, 'AmbientLight@AmbientLight.fx')
        chain += ['OrtonBloom@OrtonBloom.fx', 'prod80_02_Bloom@PD80_02_Bloom.fx']
        preset = ('Techniques=' + ','.join(chain) + '\n[Comic.fx]\nfUIEdgesLumaWeight=-0.855000,1.000000,0.800000\niUIMeshEdgesIterations=2\n[Inactive.fx]\nNoise=999\n').encode()
        output = provision.configuration(preset, Path('/private/runtime'))
        self.assertIn('effects = Vignette:EyeAdaption:lumenite_Kernel:lumenite_RTAO:Depth_Cues:ContrastStretch:Comic', output)
        self.assertIn('Comic.fUIEdgesLumaWeight = -0.855000,1.000000,0.800000', output)
        self.assertNotIn('AmbientLight', output); self.assertNotIn('OrtonBloom', output)
        self.assertNotIn('prod80', output); self.assertNotIn('Noise', output)
        with self.assertRaises(ValueError): provision.configuration(b'Techniques=Comic@Comic.fx', Path('/private'))

    def fixture(self, root):
        profile=root/'profile'; (profile/'mods').mkdir(parents=True)
        config={bridge.PREFIX+'preset':1,'game.bloomMultiplier':0.1,'game.bloomMode':2,'saved.assignment': [1,2,3,4,5]}
        (profile/'config.json').write_text(json.dumps(config))
        with zipfile.ZipFile(profile/'mods/twilit_rewired_visuals.dusk','w') as archive:
            archive.writestr('mod.json',json.dumps({'version':'0.1.2'}))
        bundle=root/'original-fx-runtime';bundle.mkdir()
        (bundle/'asset').write_bytes(b'verified')
        (bundle/'manifest.json').write_text(json.dumps({'format':1,'runtime_patch_revision':3,'files':{'asset':hashlib.sha256(b'verified').hexdigest()}}))
        (root/'wayland-test').touch()
        env={'WAYLAND_DISPLAY':'wayland-test','XDG_RUNTIME_DIR':str(root),'VK_INSTANCE_LAYERS':'existing','VK_LAYER_PATH':'/existing'}
        return profile,bundle,env

    def test_prepare_preserves_graphics_and_scopes_layer(self):
        with tempfile.TemporaryDirectory() as tmp:
            p,b,e=self.fixture(Path(tmp));before=(p/'config.json').read_bytes()
            self.assertTrue(bridge.prepare_original_fx(p,e))
            self.assertEqual((p/'config.json').read_bytes(),before)
            self.assertEqual((p/'visual-fx-state/control.txt').read_text(),'1 1 2 1\n')
            self.assertEqual(e['SDL_VIDEODRIVER'],'wayland')
            self.assertEqual(e['DISABLE_GAMESCOPE_WSI'],'1')
            self.assertEqual(e['VK_INSTANCE_LAYERS'],'existing:'+bridge.LAYER)
            self.assertTrue(e['VK_LAYER_PATH'].endswith(':/existing'))

    def test_compare_and_disabled_mod_start_bypassed(self):
        with tempfile.TemporaryDirectory() as tmp:
            p,b,e=self.fixture(Path(tmp))
            for key in ('compare-original','enabled'):
                c=json.loads((p/'config.json').read_text());c[bridge.PREFIX+'compare-original']=False;c[bridge.PREFIX+key]=key=='compare-original'
                (p/'config.json').write_text(json.dumps(c));bridge.prepare_original_fx(p,e)
                self.assertEqual((p/'visual-fx-state/control.txt').read_text(),'1 0 2 1\n')

    def test_modified_assets_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            p,b,e=self.fixture(Path(tmp));(b/'asset').write_bytes(b'modified')
            with self.assertRaisesRegex(RuntimeError,'integrity'):bridge.prepare_original_fx(p,e)

    def test_no_wayland_does_not_inject_layer(self):
        with tempfile.TemporaryDirectory() as tmp:
            p,b,e=self.fixture(Path(tmp));del e['WAYLAND_DISPLAY'];before=e.copy()
            self.assertFalse(bridge.prepare_original_fx(p,e));self.assertEqual(e,before)

    def test_crashing_revisions_never_inject_even_when_cel_selected(self):
        with tempfile.TemporaryDirectory() as tmp:
            p,b,e=self.fixture(Path(tmp));before=e.copy()
            config=(p/'config.json').read_bytes()
            manifest=json.loads((b/'manifest.json').read_text())
            for revision in (None, 1, 2):
                if revision is None: manifest.pop('runtime_patch_revision',None)
                else: manifest['runtime_patch_revision']=revision
                (b/'manifest.json').write_text(json.dumps(manifest))
                self.assertFalse(bridge.prepare_original_fx(p,e))
                self.assertEqual(e,before)
                self.assertFalse((p/'visual-fx-state').exists())
                self.assertEqual((p/'config.json').read_bytes(),config)

if __name__=='__main__':unittest.main()
