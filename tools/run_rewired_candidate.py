from pathlib import Path
import os, shutil, subprocess, time, json, sys
root=Path(__file__).resolve().parents[1]
profile=root/'test-profile-rewired'
def existing_candidate():
    for entry in Path('/proc').glob('[0-9]*/cmdline'):
        try:args=entry.read_bytes().split(b'\0')
        except OSError:continue
        if b'--user-dir' in args:
            index=args.index(b'--user-dir')
            if index+1<len(args) and args[index+1]==os.fsencode(str(profile)) and (args[0]==b'gamescope' or b'Dusklight-v2.0.3' in args[0] or Path(os.fsdecode(args[0])).name in ('dusklight','AppRun')):return True
    return False
for _ in range(60):
    if not existing_candidate():break
    time.sleep(.05)
else:raise RuntimeError('An isolated game is still open; close it before starting another recording candidate')
package=root/'build/rewired-test/mods/twilit_rewired_controls.dusk'
shutil.copy2(package,profile/'mods/twilit_rewired_controls.dusk')
config_path=profile/'config.json'
config=json.loads(config_path.read_text())
modern='--legacy' not in sys.argv
config['mod.local_twilit__rewired_controls.enabled']=True
config['mod.local_twilit__rewired_controls.modern-controls']=modern
config['mod.local_twilit__rewired_hud.enabled']=True
config['mod.com_dusklight_twilit__essentials.customZButtonEnabled']=modern
config['mod.com_dusklight_twilit__essentials.collectionAdvancedScreen']=True
config['mod.com_dusklight_twilit__essentials.collectionAdvancedDpadItems']=False
config_path.write_text(json.dumps(config,indent=2)+'\n')
(profile/'input.txt').write_text('0 0 0 0 0 0')
(profile/'capture.request').unlink(missing_ok=True)
for p in profile.glob('capture-*.ppm'):p.unlink()
(profile/'state.json').unlink(missing_ok=True)
log=(root/'build/rewired-integration.log').open('w')
runtime=os.environ.get('REWIRED_TEST_RUNTIME',str(Path.home()/'Applications/Dusklight/Dusklight-v2.0.3-linux-x86_64.AppImage'))
command=[runtime,'--user-dir',str(profile),'--backend','vulkan','--log-level','info',str(Path.home()/'Applications/Dusklight/Game data/Twilight Princess - Wii USA Rev 2.rvz')]
if '--headless' in sys.argv:
    width,height=('1920','1080') if '--full-graphics' in sys.argv else ('1280','720')
    command=['gamescope','--backend','headless','-w',width,'-h',height,'-W',width,'-H',height,'-r','60','--','env','SDL_VIDEODRIVER=x11']+command
p=subprocess.Popen(command,env=dict(os.environ,KCRUX_TEST_DIR=str(profile)),stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
(root/'build/rewired-test-pid.txt').write_text(str(p.pid))
print('Candidate PID',p.pid)
