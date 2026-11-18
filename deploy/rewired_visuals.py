"""Private original-FX layer setup. Does not alter game graphics settings."""
import hashlib
import json
import os
from pathlib import Path
import zipfile

PREFIX = 'mod.local_twilit__rewired_visuals.'
LAYER = 'VK_LAYER_VKSHADE_post_processing'


def prepare_original_fx(profile: Path, environment: dict) -> bool:
    bundle = profile.parent / 'original-fx-runtime'
    package = profile / 'mods/twilit_rewired_visuals.dusk'
    if not (bundle / 'manifest.json').exists() or not package.exists():
        return False
    with zipfile.ZipFile(package) as archive:
        meta = json.loads(archive.read('mod.json'))
    if tuple(int(n) for n in meta['version'].split('.')) < (0, 1, 2):
        return False
    manifest = json.loads((bundle / 'manifest.json').read_text())
    if manifest.get('format') != 1:
        raise RuntimeError('Unsupported original FX runtime manifest')
    # Revisions 1 and 2 caused GPU resets on normal title-screen launches.
    # Bypassing effects is insufficient: the injected layer still captures
    # depth, allocates resources and reloads them during startup resizing.
    if manifest.get('runtime_patch_revision', 0) <= 2:
        print('DrawnShade is temporarily disabled after startup GPU crashes. Current and native color presets remain available.')
        return False
    # The private layer currently requires a native Wayland surface. X11 must
    # remain a clean native-color fallback instead of presenting a black frame.
    display = environment.get('WAYLAND_DISPLAY', '')
    socket = Path(display) if display.startswith('/') else Path(environment.get('XDG_RUNTIME_DIR', '/nonexistent')) / display
    if not display or not socket.exists():
        print('DrawnShade unavailable in this session: a Wayland display is required. Native color presets remain available.')
        return False
    for relative, digest in manifest['files'].items():
        path = bundle / relative
        if not path.resolve().is_relative_to(bundle.resolve()) or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise RuntimeError('Original FX integrity check failed: ' + relative)
    config = json.loads((profile / 'config.json').read_text())
    enabled = config.get(PREFIX + 'enabled', True) and config.get(PREFIX + 'preset', 0) == 1 and not config.get(PREFIX + 'compare-original', False)
    width = min(3, max(1, int(config.get(PREFIX + 'original-mesh-width', 2))))
    mesh = int(bool(config.get(PREFIX + 'original-mesh-detail', True)))
    state = profile / 'visual-fx-state'
    state.mkdir(exist_ok=True)
    control = state / 'control.txt'
    control.write_text(f'1 {int(enabled)} {width} {mesh}\n')
    (state / 'vkShade.conf').write_text('enableOnLaunch = false\ndepthCapture = on\noverlayKey = None\ntoggleKey = None\nreloadKey = None\n')
    environment['VK_LAYER_PATH'] = str(bundle / 'layer') + (os.pathsep + environment['VK_LAYER_PATH'] if environment.get('VK_LAYER_PATH') else '')
    layers = [x for x in environment.get('VK_INSTANCE_LAYERS', '').split(':') if x and x != LAYER]
    environment['VK_INSTANCE_LAYERS'] = ':'.join(layers + [LAYER])
    environment.update(SDL_VIDEODRIVER='wayland', DISABLE_GAMESCOPE_WSI='1', ENABLE_VKSHADE='1',
                       VKSHADE_CONFIG_FILE=str(bundle / 'DrawnShade.conf'), VKSHADE_CONFIG_DIR=str(state),
                       REWIRED_FX_CONTROL=str(control), VKSHADE_LOG_LEVEL='info',
                       VKSHADE_LOG_FILE=str(state / 'runtime.log'))
    return True
