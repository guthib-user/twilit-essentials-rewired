#!/usr/bin/env python3
"""Pinned v2.0.3 Linux renderer capacity patch, applied ONLY to a copied runtime.

Equivalent source change: aurora/lib/gfx/resources.hpp VertexBufferSize 5->20,
IndexBufferSize 2->8, StorageBufferSize 8->32 MiB. Uniform/texture remain 24 MiB.
All allocation sizes, mapped ranges, ByteBuffer capacities, staging offsets and
copy offsets must agree. This does not bypass overflow checks or alter ownership.
Unknown binaries are rejected, rather than guessing offsets on an update.
"""
import hashlib,json,struct,sys
from pathlib import Path

SOURCE_SHA256='f07da293162ed4736b2877ce2f6ce2cdc26031aa40fbe2a09db0b12f9e5e2a9e'
MIB=1024*1024
# (virtual address of immediate/data, old MiB, new MiB, purpose)
PATCHES=[
 (0x2104023,5,20,'uniform copy offset'),
 (0x2104044,29,44,'index copy offset'),
 (0x2104065,31,52,'storage copy offset'),
 (0x21040aa,39,84,'texture copy offset'),
 (0x2106466,63,108,'whole staging map size'),
 (0x2107e6b,5,20,'vertex GPU allocation'),
 (0x2107ecf,2,8,'index GPU allocation'),
 (0x2107f33,8,32,'storage GPU allocation'),
 (0x2107fd9,63,108,'staging GPU allocation'),
 (0x2108fd2,5,20,'vertex mapped range size'),
 (0x210901c,5,20,'uniform mapped range offset'),
 (0x210907a,29,44,'index mapped range offset'),
 (0x2109094,2,8,'index mapped range size'),
 (0x21090d8,31,52,'storage mapped range offset'),
 (0x21090f2,8,32,'storage mapped range size'),
 (0x2109136,39,84,'texture mapped range offset'),
 # Disassembly verifies these three constants have only begin_frame xrefs.
 (0x125af88,5,20,'vertex ByteBuffer capacity'),
 (0x1257108,2,8,'index ByteBuffer capacity'),
 (0x1259768,8,32,'storage ByteBuffer capacity'),
]

def patch(source:bytes):
    if hashlib.sha256(source).hexdigest()!=SOURCE_SHA256:
        raise ValueError('Unsupported Dusklight executable; no changes made')
    assert source[:6]==b'\x7fELF\x02\x01'
    phoff=struct.unpack_from('<Q',source,32)[0]
    phsize,phnum=struct.unpack_from('<HH',source,54)
    segments=[struct.unpack_from('<IIQQQQQQ',source,phoff+i*phsize) for i in range(phnum)]
    def offset(va):
        for typ,flags,off,addr,phys,filesz,memsz,align in segments:
            if typ==1 and addr<=va and va+4<=addr+filesz:return off+va-addr
        raise ValueError(f'Unmapped address {va:x}')
    result=bytearray(source);audit=[]
    for va,old,new,purpose in PATCHES:
        off=offset(va)
        if struct.unpack_from('<I',source,off)[0]!=old*MIB:
            raise ValueError(f'Original instruction mismatch: {purpose}')
        struct.pack_into('<I',result,off,new*MIB)
        audit.append(dict(address=hex(va),offset=off,oldMiB=old,newMiB=new,purpose=purpose))
    return bytes(result),audit

if __name__=='__main__':
    src,dst=map(Path,sys.argv[1:3])
    if src.resolve()==dst.resolve():raise ValueError('Patch requires a separate output')
    data,audit=patch(src.read_bytes());dst.parent.mkdir(parents=True,exist_ok=True)
    dst.write_bytes(data);dst.chmod(0o755)
    manifest=dict(source_sha256=SOURCE_SHA256,patched_sha256=hashlib.sha256(data).hexdigest(),patches=audit)
    dst.with_suffix('.capacity-patch.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(manifest['patched_sha256'])
