#!/usr/bin/env python3
"""Run the small, SDK-independent core regression tests with temporary outputs."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

def main():
    root=Path(__file__).resolve().parents[1]
    compiler=shlex.split(os.environ.get('CXX','c++'))
    with tempfile.TemporaryDirectory(prefix='rewired-check-') as temporary:
        for name in ('kcrux_slots_test','kcrux_input_state_test'):
            binary=Path(temporary)/name
            subprocess.run([*compiler,'-std=c++20','-O2',str(root/'tests'/f'{name}.cpp'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)

if __name__=='__main__':
    main()
