#!/usr/bin/env python3
"""Generate an import-free AMD64 PE32+ fixture. No Windows toolchain needed."""
import pathlib, struct

def pe(code):
    b=bytearray(1024)
    def put(p,fmt,*v): struct.pack_into('<'+fmt,b,p,*v)
    b[:2]=b'MZ';put(0x3c,'I',0x80);b[0x80:0x84]=b'PE\0\0'
    put(0x84,'HHIIIHH',0x8664,1,0,0,0,240,0x22)
    o=0x98;put(o,'H',0x20b);put(o+4,'I',512);put(o+16,'II',4096,4096)
    put(o+24,'Q',0x140000000);put(o+32,'II',4096,512)
    put(o+40,'HHHHHH',6,0,0,0,6,0);put(o+56,'II',8192,512)
    put(o+68,'HH',3,0x100);put(o+72,'QQQQ',1048576,4096,1048576,4096);put(o+108,'I',16)
    s=o+240;b[s:s+8]=b'.text\0\0\0';put(s+8,'IIII',len(code),4096,512,512);put(s+36,'I',0x60000020)
    if len(code)>512: raise ValueError('Fixture too large')
    b[512:512+len(code)]=code
    return b

if __name__=='__main__':
    root=pathlib.Path(__file__).resolve().parents[1]
    # xor eax,eax; mov ecx,10; add eax,ecx; sub ecx,1; jne -7; ret
    code=bytes.fromhex('31c0 b90a000000 01c8 83e901 75f9 c3')
    (root/'tests'/'sum55.exe').write_bytes(pe(code))
    print('Generated tests/sum55.exe: expected RAX=55')
