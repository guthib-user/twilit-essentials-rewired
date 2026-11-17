#!/usr/bin/env python3
"""Focused controller checks on the disposable Rewired fixture at Ordon Village.

Start the modern candidate with --load-save 1 --stage F_SP103,0,0,0.
Uses the virtual pad and a simulated portal unlock only in the test build.
"""
import json
from pathlib import Path
import sys
import time

import playcheck_rewired as pad

LB, RB, START, BACK = 512, 1024, 64, 2
DPAD = (8192, 16384, 2048, 4096)
TRIGGER = 30000

def pulse(buttons=0, **inputs):
    return pad.pulse(buttons, hold=.23, settle=.8, **inputs)

def check():
    for _ in range(60):
        try:
            state = pad.state()
            if (state['stage'] == 'F_SP103' and state['player'] and
                not state['event'] and not state['uiVisible'] and state['window'] == 0 and
                state['proc'] in (1, 2, 3) and state['mapEligible']):
                break
        except (RuntimeError, KeyError):
            pass
        time.sleep(.2)
    else:
        raise RuntimeError('Candidate is not in interactive Ordon Village gameplay')
    # The engine can expose the player before the first menu-init/fade
    # interval finishes. Only send game inputs after that startup settles.
    pad.control()
    time.sleep(1.5)
    evidence = {}
    def record(name, state, **expected):
        evidence[name] = {key: state.get(key) for key in expected}
        assert evidence[name] == expected, (name, evidence[name], expected)
    pad.control(grant=27)
    time.sleep(.5)
    pad.control()
    record('world_map_open', pulse(LB), window=4, canWarp=1, portalView=0)
    for direction in DPAD:
        record(f'world_map_no_legacy_close_{direction}', pulse(direction), window=4)
    record('mark', pulse(8), window=4, portalView=0)
    record('marker_style', pulse(4), window=4, portalView=0)
    record('style_modal_rt_does_not_warp', pulse(rt=TRIGGER), window=4, portalView=0)
    record('style_modal_back', pulse(BACK), window=4, portalView=0)
    record('world_map_rt_portals', pulse(rt=TRIGGER), window=4, portalView=1)
    record('world_map_rt_back', pulse(rt=TRIGGER), window=4, portalView=0)
    record('standalone_map_lb_stays_open', pulse(LB), window=4)
    for level in range(3):
        state = pulse(BACK)
        if state['window'] == 0:
            break
        record(f'standalone_map_back_level_{level}', state, window=4, portalView=0)
    record('standalone_map_b_closes', state, window=0)
    record('pause_open', pulse(START), window=3, collectionPage=0)
    record('rt_does_not_move_outer_tab', pulse(rt=TRIGGER), window=3, collectionPage=0)
    record('lt_does_not_move_outer_tab', pulse(lt=TRIGGER), window=3, collectionPage=0)
    assignments=pad.state()['assignments']
    pad.control(rt=TRIGGER)
    time.sleep(.2)
    pad.control(buttons=RB, rt=TRIGGER)
    time.sleep(.24)
    pad.control(rt=TRIGGER)
    time.sleep(1)
    record('rb_next_tab_no_held_assignment',pad.state(),window=3,collectionPage=1,assignments=assignments)
    pad.control()
    time.sleep(.25)
    record('lb_previous_tab', pulse(LB), window=3, collectionPage=0)
    record('rb_next_tab', pulse(RB), window=3, collectionPage=1)
    for slot in range(5):
        button = {'buttons': DPAD[slot]} if slot < 4 else {'rt': TRIGGER}
        before = pad.state()['assignments']
        after = pulse(**button)
        assert after['assignments'][slot] != before[slot], (slot, before, after['assignments'])
        record(f'items_slot_{slot}', after, window=3, collectionPage=1)
    record('pause_map_tab', pulse(RB), window=4)
    record('pause_map_rt_portals', pulse(rt=TRIGGER), window=4, portalView=1)
    record('pause_map_rt_back', pulse(rt=TRIGGER), window=4, portalView=0)
    record('rb_from_map_to_collection', pulse(RB), window=3, collectionPage=2)
    record('rt_collection_inner_next', pulse(rt=TRIGGER), window=3, collectionPage=2)
    record('lt_collection_inner_previous', pulse(lt=TRIGGER), window=3, collectionPage=2)
    record('rb_leaves_collection', pulse(RB), window=3, collectionPage=4)
    record('lb_returns_to_collection', pulse(LB), window=3, collectionPage=2)
    record('lb_leaves_collection', pulse(LB), window=4)
    record('pause_map_lt_does_not_change_tab', pulse(lt=TRIGGER), window=4, portalView=0)
    record('pause_map_lb_previous_tab', pulse(LB), window=3, collectionPage=1)
    record('items_lb_previous_tab', pulse(LB), window=3, collectionPage=0)
    record('pause_back', pulse(BACK), window=0)
    record('wheel_open', pulse(RB), window=2)
    pad.control(lt=TRIGGER)
    time.sleep(.35)
    wheel_l=pad.state()
    assert wheel_l['window']==2 and wheel_l['nativeHold'] & 0x40
    evidence['wheel_lt_direct_select']={'window':2,'native_L':True}
    pad.control()
    time.sleep(.25)
    for slot in range(5):
        button = {'buttons': DPAD[slot]} if slot < 4 else {'rt': TRIGGER}
        before = pad.state()['assignments']
        after = pulse(**button)
        assert after['assignments'][slot] != before[slot], (slot, before, after['assignments'])
        record(f'wheel_slot_{slot}', after, window=2)
    record('wheel_back', pulse(BACK), window=0)
    return evidence

if __name__ == '__main__':
    result = check()
    print(f'PASS: {len(result)} controller checks')
    if len(sys.argv) > 1:
        Path(sys.argv[1]).write_text(json.dumps(result, indent=2)+'\n')
