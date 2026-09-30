#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace forge {
using Bytes = std::vector<uint8_t>;
inline std::string hex(uint64_t n) { std::ostringstream s; s << "0x" << std::hex << n; return s.str(); }
struct Fault : std::runtime_error { using std::runtime_error::runtime_error; };
inline uint64_t read(const Bytes& b, uint64_t p, unsigned n) {
    if (n > 8 || p > b.size() || n > b.size()-p) throw Fault("Truncated data at " + hex(p));
    uint64_t v=0; for(unsigned i=0;i<n;++i) v |= uint64_t(b[size_t(p)+i]) << (8*i); return v;
}
struct Region { uint64_t base; Bytes bytes; bool writable, executable; };
struct Import { uint64_t address; std::string name; };
struct Memory {
    std::vector<Import> imports;
    std::vector<Region> regions;
    void map(uint64_t base, size_t size, bool w, bool x) {
        if(!size || size > 64*1024*1024 || base > UINT64_MAX-size) throw Fault("Invalid mapping");
        for(auto& r:regions) if(base < r.base+r.bytes.size() && r.base < base+size) throw Fault("Overlapping mapping");
        regions.push_back({base,Bytes(size),w,x});
    }
    Region& at(uint64_t a, size_t n) {
        for(auto& r:regions) if(a>=r.base && a-r.base<=r.bytes.size() && n<=r.bytes.size()-(a-r.base)) return r;
        throw Fault("Unmapped guest address " + hex(a));
    }
    uint64_t get(uint64_t a,unsigned n,bool execute=false) {
        auto& r=at(a,n); if(execute&&!r.executable) throw Fault("Execution denied at "+hex(a)); return read(r.bytes,a-r.base,n);
    }
    void put(uint64_t a,uint64_t v,unsigned n) {
        auto& r=at(a,n); if(!r.writable) throw Fault("Write denied at "+hex(a));
        for(unsigned i=0;i<n;++i) r.bytes[size_t(a-r.base)+i]=uint8_t(v>>(8*i));
    }
    void copy(uint64_t a,const Bytes& b,size_t p,size_t n) {
        auto& r=at(a,n); if(p>b.size()||n>b.size()-p) throw Fault("Invalid source extent");
        std::copy_n(b.begin()+p,n,r.bytes.begin()+size_t(a-r.base));
    }
};
struct Image { uint64_t base,entry; unsigned sections; };
inline Image loadPE(const Bytes& b,Memory& mem) {
    if(b.size()>64*1024*1024) throw Fault("P1 file limit: 64 MiB");
    if(read(b,0,2)!=0x5a4d) throw Fault("Not an MZ executable");
    uint64_t p=read(b,0x3c,4);
    if(read(b,p,4)!=0x4550 || read(b,p+4,2)!=0x8664) throw Fault("Requires AMD64 PE executable");
    unsigned count=unsigned(read(b,p+6,2)), os=unsigned(read(b,p+20,2));
    if(!count||count>96||os<112) throw Fault("Invalid PE header sizes");
    if(read(b,p+22,2)&0x2000) throw Fault("DLL entry points are unsupported");
    uint64_t o=p+24; read(b,o+os-1,1);
    if(read(b,o,2)!=0x20b) throw Fault("Requires PE32+");
    uint64_t base=read(b,o+24,8), ep=read(b,o+16,4), size=read(b,o+56,4), headers=read(b,o+60,4);
    if(!size||size>32*1024*1024||base>UINT64_MAX-size||ep>=size||!ep||headers>size||headers>b.size()) throw Fault("Invalid PE image bounds (P1 image limit: 32 MiB)");
    unsigned dirs=unsigned(read(b,o+108,4));
    if(dirs>16 || 112+uint64_t(dirs)*8>os) throw Fault("Invalid data directory extent");
    for(unsigned i: {9u,13u,14u}) if(i<dirs && (read(b,o+112+i*8,4)||read(b,o+116+i*8,4))) {
        const char* name=i==1?"Windows DLL imports":i==9?"TLS callbacks":i==13?"delay imports":"managed .NET code";
        throw Fault(std::string("Not implemented in P1: ")+name);
    }
    Memory staged;
    staged.map(base,size_t(headers),false,false); staged.copy(base,b,0,size_t(headers));
    uint64_t table=o+os;
    for(unsigned i=0;i<count;++i) {
        uint64_t s=table+i*40; read(b,s+39,1);
        uint64_t vs=read(b,s+8,4), va=read(b,s+12,4), raw=read(b,s+16,4), off=read(b,s+20,4), flags=read(b,s+36,4);
        uint64_t span=std::max(vs,raw);
        if(!span) continue;
        if(va>=size||span>size-va||off>b.size()||raw>b.size()-off) throw Fault("Invalid PE section bounds");
        staged.map(base+va,size_t(span),(flags&0x80000000)!=0,(flags&0x20000000)!=0);
        if(raw) staged.copy(base+va,b,size_t(off),size_t(raw));
    }
    auto rva = [&](uint64_t v,unsigned n)->uint64_t {
        if(v>=size || n>size-v) throw Fault("Import RVA outside image");
        staged.at(base+v,n); return base+v;
    };
    auto str = [&](uint64_t v)->std::string {
        std::string out; for(unsigned j=0;j<256;++j){char ch=char(staged.get(rva(v+j,1),1));if(!ch)return out;out+=ch;}throw Fault("Unterminated import name");
    };
    if(dirs>1) {
        uint64_t dir=read(b,o+120,4), len=read(b,o+124,4);
        if(dir||len) {
            if(!dir||len<20||len>65536)throw Fault("Invalid import directory");
            rva(dir,unsigned(len)); bool ended=false;
            for(uint64_t k=0;k+20<=len;k+=20) {
                uint64_t d=rva(dir+k,20), lookup=staged.get(d,4), name=staged.get(d+12,4), iat=staged.get(d+16,4);
                if(!lookup&&!name&&!iat&&!staged.get(d+4,8)){ended=true;break;}
                if(!name||!iat)throw Fault("Invalid import descriptor");
                std::string dll=str(name);for(auto& ch:dll)ch=char(std::tolower(static_cast<unsigned char>(ch)));
                if(dll!="kernel32.dll")throw Fault("Unsupported DLL: "+dll);
                if(!lookup)lookup=iat;
                bool terminated=false;
                for(unsigned j=0;j<256;++j) {
                    auto thunk=staged.get(rva(lookup+uint64_t(j)*8,8),8);if(!thunk){terminated=true;break;}
                    if(thunk>>63)throw Fault("Ordinal imports not implemented");
                    if(thunk>UINT32_MAX-2)throw Fault("Invalid import name RVA");
                    std::string fn=str(thunk+2);
                    if(fn!="ExitProcess"&&fn!="OutputDebugStringA"&&fn!="GetTickCount64")throw Fault("Unsupported API: "+dll+"!"+fn);
                    if(staged.imports.size()>=256)throw Fault("Import count limit");
                    uint64_t stub=0x7fff00000000ull+staged.imports.size()*16;
                    staged.imports.push_back({stub,fn});Bytes ptr(8);for(unsigned n=0;n<8;++n)ptr[n]=uint8_t(stub>>(n*8));
                    staged.copy(rva(iat+uint64_t(j)*8,8),ptr,0,8);
                }
                if(!terminated)throw Fault("Import thunk list limit");
            }
            if(!ended)throw Fault("Unterminated import directory");
        }
    }
    staged.get(base+ep,1,true); mem=std::move(staged); return {base,base+ep,count};
}
struct CPU {
    Memory& mem;
    std::array<uint64_t,16> r{}; // RAX RCX RDX RBX RSP RBP RSI RDI R8..R15
    uint64_t ip=0,steps=0,lastIP=0; bool exited=false; uint32_t exitCode=0; std::string output;
    const std::chrono::steady_clock::time_point epoch=std::chrono::steady_clock::now();
    bool zf=false, sf=false, cf=false, of=false, pf=false,returned=false;
    std::vector<uint64_t> trace;
    explicit CPU(Memory& m):mem(m){}
    uint64_t fetch(unsigned n) {auto v=mem.get(ip,n,true); ip+=n; return v;}
    void push(uint64_t n) {uint64_t sp=r[4]-8; mem.put(sp,n,8); r[4]=sp;}
    uint64_t pop() {auto n=mem.get(r[4],8);r[4]+=8;return n;}
    void start(uint64_t entry) {mem.map(0x7000000000ull,1024*1024,true,false);r[4]=0x7000100000ull-32;push(0);ip=entry;}
    static uint64_t mask(unsigned w) {return w==64?UINT64_MAX:UINT32_MAX;}
    void writeReg(unsigned a,uint64_t v,unsigned w) {r[a]=v&mask(w);}
    uint64_t alu(uint64_t a,uint64_t b,unsigned w,bool sub,bool logic=false) {
        uint64_t m=mask(w),sign=uint64_t(1)<<(w-1);a&=m;b&=m;
        uint64_t v=(logic?(a^b):(sub?a-b:a+b))&m;
        zf=v==0;sf=(v&sign)!=0;cf=logic?false:(sub?a<b:v<a);
        of=logic?false:((sub?((a^b)&(a^v)):(~(a^b)&(a^v)))&sign)!=0;
        unsigned ones=0;for(unsigned i=0;i<8;++i) ones+=unsigned((v>>i)&1);pf=(ones%2)==0;return v;
    }
    bool condition(unsigned c) {switch(c){case 0:return of;case 1:return !of;case 2:return cf;case 3:return !cf;case 4:return zf;case 5:return !zf;case 6:return cf||zf;case 7:return !cf&&!zf;case 8:return sf;case 9:return !sf;case 10:return pf;case 11:return !pf;case 12:return sf!=of;case 13:return sf==of;case 14:return zf||sf!=of;default:return !zf&&sf==of;}}
    struct Operand { bool reg=false,rip=false; unsigned index=0; uint64_t base=0; };
    Operand decode(unsigned mr,unsigned rex) {
        unsigned mod=mr>>6,rm=mr&7;
        if(mod==3)return {true,false,rm|((rex&1)?8u:0u),0};
        Operand x;
        if(rm==4) {
            unsigned sib=unsigned(fetch(1)), bi=sib&7, ix=(sib>>3)&7;
            if(ix!=4 || (rex&2)) x.base+=r[ix|((rex&2)?8:0)]<<(sib>>6);
            if(mod==0&&bi==5)x.base+=uint64_t(int64_t(int32_t(fetch(4))));
            else x.base+=r[bi|((rex&1)?8:0)];
        } else if(mod==0&&rm==5){x.rip=true;x.base=uint64_t(int64_t(int32_t(fetch(4))));}
        else x.base=r[rm|((rex&1)?8:0)];
        if(mod==1)x.base+=uint64_t(int64_t(int8_t(fetch(1))));
        if(mod==2)x.base+=uint64_t(int64_t(int32_t(fetch(4))));
        return x;
    }
    uint64_t address(const Operand& x) const {return x.base+(x.rip?ip:0);}
    uint64_t value(const Operand& x,unsigned w){return x.reg?(r[x.index]&mask(w)):mem.get(address(x),w/8);}
    void store(const Operand& x,uint64_t v,unsigned w){if(x.reg)writeReg(x.index,v,w);else mem.put(address(x),v,w/8);}
    bool api() {
        for(const auto& imp:mem.imports)if(ip==imp.address) {
            if((r[4]&15)!=8)throw Fault("Windows x64 API stack alignment violation");
            if(imp.name=="ExitProcess"){exitCode=uint32_t(r[1]);exited=true;returned=true;return true;}
            if(imp.name=="GetTickCount64")r[0]=uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-epoch).count());
            if(imp.name=="OutputDebugStringA") {
                std::string line;bool end=false;for(unsigned j=0;j<4096;++j){char ch=char(mem.get(r[1]+j,1));if(!ch){end=true;break;}line+=ch;}
                if(!end)throw Fault("Debug string exceeds 4096 bytes");
                if(output.size()+line.size()>65536)throw Fault("Debug output limit");
                output+=line;
            }
            ip=pop();if(!ip)returned=true;return true;
        }
        return false;
    }
    void step() {
        lastIP=ip;if(trace.size()==32)trace.erase(trace.begin());trace.push_back(ip);
        if(api())return;
        unsigned rex=0,op=unsigned(fetch(1));if(op>=0x40&&op<=0x4f){rex=op;op=unsigned(fetch(1));}
        unsigned w=(rex&8)?64:32;
        if(op>=0xb8&&op<=0xbf){writeReg((op-0xb8)|((rex&1)?8:0),fetch(w/8),w);return;}
        if(op>=0x50&&op<=0x57){push(r[(op-0x50)|((rex&1)?8:0)]);return;}
        if(op>=0x58&&op<=0x5f){auto v=pop();r[(op-0x58)|((rex&1)?8:0)]=v;return;}
        if(op==0x90){if(rex&1)throw Fault("Extended XCHG not implemented");return;}
        if(op==0xc3){ip=pop();if(ip==0)returned=true;return;}
        if(op==0xe8||op==0xe9){auto d=int32_t(fetch(4));auto target=ip+uint64_t(int64_t(d));if(op==0xe8)push(ip);ip=target;return;}
        if(op==0xeb){auto d=int8_t(fetch(1));ip+=uint64_t(int64_t(d));return;}
        if(op>=0x70&&op<=0x7f){auto d=int8_t(fetch(1));if(condition(op&15))ip+=uint64_t(int64_t(d));return;}
        if(op==0x0f){unsigned op2=unsigned(fetch(1));if(op2>=0x80&&op2<=0x8f){auto d=int32_t(fetch(4));if(condition(op2&15))ip+=uint64_t(int64_t(d));return;}throw Fault("Unsupported 0F opcode "+hex(op2));}
        if(op==0xc7||op==0x8d||op==0xff||op==0x89||op==0x8b||op==0x31||op==0x33||op==0x01||op==0x03||op==0x29||op==0x2b||op==0x39||op==0x3b||op==0x83||op==0x81) {
            unsigned mr=unsigned(fetch(1)), group=(mr>>3)&7;
            Operand a=decode(mr,rex),b{true,false,group|((rex&4)?8u:0u),0};
            if(op==0x8d){if(a.reg)throw Fault("Invalid LEA");writeReg(b.index,address(a),w);return;}
            if(op==0xff){
                if(group!=2&&group!=4&&group!=6)throw Fault("Unsupported FF group");
                auto target=value(a,64);if(group==6){push(target);return;}if(group==2)push(ip);ip=target;return;
            }
            if(op==0xc7){if(group)throw Fault("Unsupported C7 group");auto v=uint64_t(int64_t(int32_t(fetch(4))));store(a,v,w);return;}
            if(op==0x83||op==0x81){uint64_t imm=op==0x83?uint64_t(int64_t(int8_t(fetch(1)))):uint64_t(int64_t(int32_t(fetch(4))));
                if(group!=0&&group!=5&&group!=7)throw Fault("Unsupported ALU immediate group");
                auto v=alu(value(a,w),imm,w,group!=0);if(group!=7)store(a,v,w);return;}
            bool reverse=op==0x8b||op==0x33||op==0x03||op==0x2b||op==0x3b;if(reverse)std::swap(a,b);
            if(op==0x89||op==0x8b){store(a,value(b,w),w);return;}
            bool cmp=op==0x39||op==0x3b;auto v=alu(value(a,w),value(b,w),w,cmp||op==0x29||op==0x2b,op==0x31||op==0x33);if(!cmp)store(a,v,w);return;
        }
        throw Fault("Unsupported opcode "+hex(op));
    }
    void run(uint64_t budget=1000000) {while(!returned&&steps<budget){++steps;step();}if(!returned)throw Fault("Instruction budget exhausted");}
    std::string state() const {std::ostringstream o;o<<"RIP="<<hex(ip)<<" last="<<hex(lastIP)<<" instructions="<<steps<<"\n";const char* names[]={"RAX","RCX","RDX","RBX","RSP","RBP","RSI","RDI","R8","R9","R10","R11","R12","R13","R14","R15"};for(unsigned i=0;i<16;++i)o<<names[i]<<"="<<hex(r[i])<<((i%4==3)?"\n":" ");o<<"Last instruction addresses:";for(auto a:trace)o<<" "<<hex(a);return o.str();}
};
inline std::string execute(const Bytes& file) {
    Memory m;std::ostringstream log;log<<"ForgeWin P1 — interpreter diagnostic\n";
    try {auto img=loadPE(file,m);log<<"AMD64 PE32+ base="<<hex(img.base)<<" entry="<<hex(img.entry)<<" sections="<<img.sections<<"\n";
        CPU c(m);c.start(img.entry);try{c.run();if(c.exited)log<<"ExitProcess code="<<c.exitCode<<"\n";else log<<"Guest entry returned RAX="<<c.r[0]<<"\n";}catch(const std::exception& e){log<<"STOP: "<<e.what()<<"\n";}log<<"Guest debug output: "<<c.output<<"\n";log<<c.state()<<"\n";
    }catch(const std::exception& e){log<<"LOAD STOP: "<<e.what()<<"\n";}return log.str();
}
}
