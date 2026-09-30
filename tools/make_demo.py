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

    # Import test: stack memory, indexed array sum, RIP-relative IAT calls.
    code=bytearray()
    def emit(h): code.extend(bytes.fromhex(h))
    def rip(h,target):
        emit(h);code.extend(struct.pack('<i',target-(0x1000+len(code)+4)))
    emit('48 83 ec 38')                  # shadow space, local slots, alignment
    rip('ff15',0x2070)                   # GetTickCount64
    emit('4889442420')                   # save returned tick count to stack
    rip('488d15',0x2180)                 # lea rdx, [array]
    emit('31c0 b905000000')              # eax=0, ecx=5
    loop=len(code)
    emit('03448afc 83e901')              # add eax,[rdx+rcx*4-4]; sub ecx,1
    emit('75');code.append((loop-(len(code)+1))&255)
    emit('89442428')                     # store sum
    rip('488d0d',0x2140)                 # debug string argument
    rip('ff15',0x2068)                   # OutputDebugStringA
    emit('8b4c2428')                     # ecx=stack sum
    rip('ff15',0x2060)                   # ExitProcess(sum)
    emit('cc')                          # must never execute
    b=pe(code);b.extend(bytes(512));o=0x98
    struct.pack_into('<H',b,0x86,2);struct.pack_into('<I',b,o+56,0x3000)
    struct.pack_into('<II',b,o+120,0x2000,40)
    s=o+240+40;b[s:s+8]=b'.idata\0\0';struct.pack_into('<IIII',b,s+8,512,0x2000,512,1024);struct.pack_into('<I',b,s+36,0xc0000040)
    def at(rva): return 1024+rva-0x2000
    struct.pack_into('<IIIII',b,at(0x2000),0x2040,0,0,0x2080,0x2060)
    dll=b'KERNEL32.dll\0';b[at(0x2080):at(0x2080)+len(dll)]=dll
    for i,(rva,name) in enumerate([(0x2090,b'ExitProcess'),(0x20b0,b'OutputDebugStringA'),(0x20d0,b'GetTickCount64')]):
        struct.pack_into('<Q',b,at(0x2040)+8*i,rva);struct.pack_into('<Q',b,at(0x2060)+8*i,rva)
        b[at(rva)+2:at(rva)+3+len(name)]=name+b'\0'
    msg=b'ForgeWin P1: memory + Windows imports OK\0';b[at(0x2140):at(0x2140)+len(msg)]=msg
    struct.pack_into('<IIIII',b,at(0x2180),7,11,13,17,23)
    (root/'tests'/'winapi71.exe').write_bytes(b)
    print('Generated tests/winapi71.exe: expected ExitProcess code=71')
