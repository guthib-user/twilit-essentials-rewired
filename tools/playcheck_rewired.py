from pathlib import Path
import json,time,os
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/'test-profile-rewired'
def state():
    for _ in range(30):
        try:return json.loads((P/'state.json').read_text())
        except (OSError,ValueError):time.sleep(.02)
    raise RuntimeError('No valid game state')
def alive():
    pid=int((ROOT/'build/rewired-test-pid.txt').read_text())
    try:
        os.kill(pid,0)
        return Path(f'/proc/{pid}/stat').read_text().split()[2]!='Z'
    except (ProcessLookupError,FileNotFoundError):return False
def control(buttons=0,rt=0,lx=0,ly=0,lt=0,grant=0,rx=0,ry=0):
    assert alive(),'Candidate has exited'
    tmp=P/'input.next';tmp.write_text(f'{buttons} {rt} {lx} {ly} {lt} {grant} {rx} {ry}');tmp.replace(P/'input.txt')
def pulse(buttons=0,rt=0,lx=0,ly=0,lt=0,hold=.18,settle=.5,grant=0,rx=0,ry=0):
    control(buttons,rt,lx,ly,lt,grant,rx,ry);time.sleep(hold);control(grant=grant);time.sleep(settle);return state()
def snap():
    before={f.name:f.stat().st_mtime_ns for f in P.glob('capture-*.ppm')}
    (P/'capture.request').touch()
    for _ in range(50):
        time.sleep(.15)
        for f in P.glob('capture-*.ppm'):
            if f.name not in before or f.stat().st_mtime_ns!=before[f.name]:
                try:
                    from PIL import Image
                    im=Image.open(f);im.load();target=f.with_suffix('.png');im.save(target);return str(target)
                except OSError:pass
        assert alive(),'Candidate exited during screenshot'
    raise RuntimeError('No new capture')
if __name__=='__main__':
    import sys
    if sys.argv[1] in ('boot','boot-save'):
        time.sleep(2)
        for i in range(26):
            s=state()
            if s.get('player')and not s.get('event')and (sys.argv[1]=='boot-save' or s.get('stage')in('R_SP01','F_SP103')):break
            pulse(1 if i%2==0 else 64,settle=.6)
        print('boot',state())
        if sys.argv[1]=='boot':
            control(grant=1);time.sleep(.5);control(grant=2);time.sleep(3.5);control(grant=3);time.sleep(.7);control()
        print('world',state());print(snap())
    elif sys.argv[1]=='state':print(state())
    elif sys.argv[1]=='snap':print(snap())
    elif sys.argv[1]=='quit':control(grant=-1)
