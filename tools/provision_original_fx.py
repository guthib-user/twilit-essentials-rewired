#!/usr/bin/env python3
"""Provision unmodified DrawnShade shader sources from their authors (never Git).

Build the separately pinned vkShade runtime first; see docs/VISUAL-PRESETS.md.
The downloaded preset is required locally so it is not redistributed by Rewired.
"""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import shutil
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / 'rewired-visuals/original-fx-sources.json'
EXCLUDED = {'AmbientLight.fx', 'OrtonBloom.fx', 'PD80_02_Bloom.fx'}
EXPECTED = ['Vignette@Vignette.fx', 'EyeAdaption@EyeAdaption.fx',
            'Lumenite_Kernel@lumenite_Kernel.fx', 'Lumenite_RTAO@lumenite_RTAO.fx',
            'Monocular_Cues@Depth_Cues.fx', 'ContrastStretch@ContrastStretch.fx', 'Comic@Comic.fx']


def sha(data):
    return hashlib.sha256(data).hexdigest()


def configuration(preset, destination):
    config = configparser.ConfigParser()
    config.optionxform = str
    config.read_string('[Preset]\n' + preset.decode('utf-8-sig'))
    enabled = config['Preset']['Techniques'].split(',')
    chain = [x for x in enabled if x.split('@')[1] not in EXCLUDED]
    if chain != EXPECTED:
        raise ValueError('Preset enabled techniques differ from the reviewed DrawnShade chain')
    shaders = destination / 'shaders'
    lines = ['enableOnLaunch = false', 'depthCapture = on', 'depthCopyBeforeClears = true',
             'RESHADE_DEPTH_INPUT_IS_REVERSED = 1', 'RESHADE_DEPTH_INPUT_IS_UPSIDE_DOWN = 0',
             'RESHADE_DEPTH_INPUT_IS_LOGARITHMIC = 0', 'RESHADE_DEPTH_LINEARIZATION_FAR_PLANE = 1000.0',
             f'reshadeIncludePath = "{shaders}"', f'reshadeTexturePath = "{shaders}/LumeniteFX/Textures"',
             'effects = ' + ':'.join(Path(x.split('@')[1]).stem for x in chain)]
    for entry in chain:
        technique, name = entry.split('@')
        alias = Path(name).stem
        path = shaders / ('LumeniteFX/Shaders/' + name if name.startswith('lumenite_') else name)
        lines.append(f'{alias} = "{path}"')
        if name in config:
            lines.extend(f'{alias}.{key} = {value}' for key, value in config[name].items())
    return '\n'.join(lines) + '\n'


def provision(destination, preset_path, library, cache=None):
    manifest = json.loads(SOURCES.read_text())
    preset = preset_path.read_bytes()
    if sha(preset) != manifest['preset']['sha256']:
        raise ValueError('Original DrawnShade preset checksum differs; review it before provisioning')
    destination = destination.resolve()
    destination.mkdir(parents=True, exist_ok=True)
    hashes = {}
    for entry in manifest['files']:
        target = destination / 'shaders' / entry['path']
        cached = cache / entry['path'] if cache else None
        data = cached.read_bytes() if cached and cached.exists() else urllib.request.urlopen(entry['url'], timeout=45).read()
        if sha(data) != entry['sha256']:
            raise ValueError('Shader checksum differs: ' + entry['path'])
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        hashes[target.relative_to(destination).as_posix()] = sha(data)
    lib = destination / 'lib64/libvkshade.so'
    lib.parent.mkdir(exist_ok=True)
    shutil.copy2(library, lib)
    hashes['lib64/libvkshade.so'] = sha(lib.read_bytes())
    layer = {'file_format_version': '1.0.0', 'layer': {
        'name': 'VK_LAYER_VKSHADE_post_processing', 'type': 'GLOBAL',
        'library_path': '../lib64/libvkshade.so', 'api_version': '1.3.223',
        'implementation_version': '1', 'description': 'Private Rewired original FX runtime',
        'functions': {'vkGetInstanceProcAddr': 'vkShade_GetInstanceProcAddr', 'vkGetDeviceProcAddr': 'vkShade_GetDeviceProcAddr'},
        'enable_environment': {'ENABLE_VKSHADE': '1'}, 'disable_environment': {'DISABLE_VKSHADE': '1'}}}
    files = {'layer/vkShade.json': json.dumps(layer, indent=2) + '\n',
             'DrawnShade.conf': configuration(preset, destination),
             'DrawnShade-original.ini': preset.decode('utf-8-sig')}
    for name, data in files.items():
        target = destination / name
        target.parent.mkdir(exist_ok=True, parents=True)
        target.write_text(data)
        hashes[name] = sha(target.read_bytes())
    for license in sorted((ROOT / 'rewired-visuals/runtime').glob('*LICENSE*')):
        target = destination / 'licenses' / license.name
        target.parent.mkdir(exist_ok=True)
        shutil.copy2(license, target)
        hashes[target.relative_to(destination).as_posix()] = sha(target.read_bytes())
    shutil.copy2(SOURCES, destination / 'sources.json')
    installed = {
        'format': 1, 'runtime_commit': manifest['runtime']['commit'],
        'runtime_patch_revision': manifest['runtime']['patch_revision'],
        'runtime_patch_sha256': sha((ROOT / 'rewired-visuals/runtime/vkshade-compat.patch').read_bytes()),
        'files': hashes,
    }
    (destination / 'manifest.json').write_text(json.dumps(installed, indent=2) + '\n')
    return hashes


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', type=Path, required=True)
    parser.add_argument('--preset', type=Path, required=True)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--cache', type=Path)
    args = parser.parse_args()
    files = provision(args.destination, args.preset, args.library, args.cache)
    print(f'Provisioned {len(files)} verified files at {args.destination}')
