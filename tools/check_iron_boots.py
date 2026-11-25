#!/usr/bin/env python3
"""Iron Boots regression on a disposable copied save that already owns them.

Requires the test driver and interactive Ordon Village. Does not create a save.
"""
import json
from pathlib import Path
import sys
import time
import playcheck_rewired as pad


def check(output):
    state = pad.state()
    assert state['stage'] == 'F_SP103' and state['player'] and not state['event']
    assert state['inventory'].count(0x45) == 1, 'Use an ordinary save owning one Iron Boots item'
    results = []

    def grant(value):
        pad.control(grant=value)
        time.sleep(.3)
        pad.control()
        time.sleep(.4)

    def record(name, expected):
        state = pad.state()
        row = {'case': name, 'bootsEquipped': state['bootsEquipped'],
               'membership': state['bootsSet'], 'activeSlot': state['activeSlot']}
        results.append(row)
        output.write_text(json.dumps(results, indent=2) + '\n')
        assert row['bootsEquipped'] == expected, row
        return state

    grant(42)
    record('unassigned', False)
    for slot in (3, 0, 1, 2, 4):
        grant(37 + slot)
        inputs = {'buttons': (8192, 16384, 2048, 4096)[slot]} if slot < 4 else {'rt': 28000}
        pad.control(**inputs)
        time.sleep(1.3)
        record(f'slot_{slot}_held_on', True)
        pad.control()
        time.sleep(.7)
        record(f'slot_{slot}_released_on', True)
        pad.pulse(4, hold=.2, settle=1.3)  # Physical X: draw sword, change native projection.
        record(f'slot_{slot}_sword_keeps_boots', True)
        pad.control(**inputs)
        time.sleep(1.3)
        record(f'slot_{slot}_held_off', False)
        pad.control()
        time.sleep(.7)
        record(f'slot_{slot}_released_off', False)
    grant(40)
    pad.pulse(4096, hold=.2, settle=1.3)
    record('down_equipped_before_clear', True)
    grant(42)
    record('clear_assignment_removes_boots', False)
    print(f'PASS: {len(results)} Iron Boots checks')


if __name__ == '__main__':
    try:
        check(Path(sys.argv[1]))
    finally:
        pad.control()
