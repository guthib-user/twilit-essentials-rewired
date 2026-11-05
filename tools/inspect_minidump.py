#!/usr/bin/env python3
"""Read-only AMD64 minidump triage: exception context and module-mapped stack words.

Usage: python3 tools/inspect_minidump.py /path/to/crash.dmp
This scans plausible stack addresses; it is NOT a validated stack unwinder.
Keep dumps local because they can contain private process memory.
"""
import struct,sys,json
from pathlib import Path
b=Path(sys.argv[1]).read_bytes()
u32=lambda o:struct.unpack_from('<I',b,o)[0]
u64=lambda o:struct.unpack_from('<Q',b,o)[0]
streams={u32(o):(u32(o+4),u32(o+8)) for o in range(u32(12),u32(12)+u32(8)*12,12)}
mods=[]
for o in range(streams[4][1]+4,streams[4][1]+4+u32(streams[4][1])*108,108):
 r=u32(o+20);name=b[r+4:r+4+u32(r)].decode('utf-16le');mods.append((u64(o),u32(o+8),name))
e=streams[6][1];tid=u32(e);c=u32(e+164);rsp=u64(c+152);print('thread',tid,'rsp',hex(rsp),'rip',hex(u64(c+248)))
for o in range(streams[3][1]+4,streams[3][1]+4+u32(streams[3][1])*48,48):
 if u32(o)!=tid:continue
 start=u64(o+24);size=u32(o+32);rva=u32(o+36)
 for j in range(max(0,rsp-start),size-7,8):
  addr=u64(rva+j)
  for base,sz,name in mods:
   if base<=addr<base+sz and ('dusklight' in name.lower() or '.so' in name):print(hex(start+j-rsp),hex(addr-base),name)
