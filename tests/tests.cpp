#include "../core/forge.hpp"
#include <fstream>
#include <iostream>
#include <cassert>
using namespace forge;
Bytes fixture(){std::ifstream f("tests/sum55.exe",std::ios::binary);return Bytes(std::istreambuf_iterator<char>(f),{});}
void patch(Bytes& b,size_t p,uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)b[p+i]=uint8_t(v>>(i*8));}
void failsLoad(Bytes b){Memory m;bool failed=false;try{loadPE(b,m);}catch(const Fault&){failed=true;}assert(failed);assert(m.regions.empty());}
uint64_t run(Bytes code){auto b=fixture();std::copy(code.begin(),code.end(),b.begin()+512);Memory m;auto im=loadPE(b,m);CPU c(m);c.start(im.entry);c.run(100);return c.r[0];}
int main(){
 auto b=fixture();assert(!b.empty());Memory m;auto img=loadPE(b,m);CPU c(m);c.start(img.entry);c.run();assert(c.r[0]==55&&c.steps==33);
 assert(run({0x48,0xb8,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xb8,42,0,0,0,0xc3})==42); // 32-bit zero extension
 assert(run({0x48,0xb8,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x48,0x83,0xc0,1,0x72,6,0xb8,99,0,0,0,0xc3,0xb8,7,0,0,0,0xc3})==7); // carry
 assert(run({0xe8,1,0,0,0,0xc3,0xb8,42,0,0,0,0xc3})==42); // call/ret
 assert(run({0x41,0xb8,42,0,0,0,0x44,0x89,0xc0,0xc3})==42); // R8D
 auto bad=b;bad.resize(70);failsLoad(bad);
 bad=b;patch(bad,0x98+112+8,0x1000,4);failsLoad(bad); // imports
 bad=b;patch(bad,0x98+240+20,0xfffffff0,4);failsLoad(bad); // raw file overflow
 bad=b;patch(bad,0x98+240+12,0x100,4);failsLoad(bad); // header overlap
 bad=b;patch(bad,0x98+240+36,0x40000020,4);failsLoad(bad); // non-executable entry
 bad=b;bad[512]=0xcc;assert(execute(bad).find("Unsupported opcode")!=std::string::npos);
 bad=b;bad[512]=0xeb;bad[513]=0xfe;Memory loop;auto li=loadPE(bad,loop);CPU lc(loop);lc.start(li.entry);bool budget=false;try{lc.run(50);}catch(const Fault&){budget=true;}assert(budget&&lc.steps==50);
 bool protectedWrite=false;try{m.put(img.entry,0,1);}catch(const Fault&){protectedWrite=true;}assert(protectedWrite);
 // Malformed input mutations must never escape bounds or crash under sanitizers.
 uint32_t rng=0x55aa;for(unsigned i=0;i<3000;++i){bad=b;for(unsigned j=0;j<4;++j){rng=rng*1664525+1013904223;size_t at=rng%512;rng=rng*1664525+1013904223;bad[at]=uint8_t(rng>>24);}Memory mm;try{loadPE(bad,mm);}catch(const Fault&){} }
 std::ifstream wf("tests/winapi71.exe",std::ios::binary);Bytes wb(std::istreambuf_iterator<char>(wf),{});
 Memory wm;auto wi=loadPE(wb,wm);assert(wm.imports.size()==3);CPU wc(wm);wc.start(wi.entry);wc.run();assert(wc.exited&&wc.exitCode==71);assert(wc.output=="ForgeWin P1: memory + Windows imports OK");
 auto unknown=wb;unknown[1024+0x92]='X';failsLoad(unknown);
 auto truncated=wb;patch(truncated,0x98+124,20,4);failsLoad(truncated);
 auto badString=wb;patch(badString,1024+0x0c,0x2fff,4);failsLoad(badString);
 // RIP-relative address uses the end of the whole instruction, including immediate.
 Memory rm;rm.map(0x1000,128,true,true);Bytes rc={0xc7,0x05,0x36,0,0,0,42,0,0,0,0x8b,0x05,0x30,0,0,0,0xc3};rm.copy(0x1000,rc,0,rc.size());CPU rr(rm);rr.start(0x1000);rr.run();assert(rr.r[0]==42);
 assert(run({0x48,0x83,0xec,8,0x48,0xc7,0x04,0x24,0xff,0xff,0xff,0xff,0x48,0x8b,0x04,0x24,0x48,0x83,0xc4,8,0xc3})==UINT64_MAX);
 std::cout<<"PASS: P1 memory addressing, stack, SIB, RIP-relative immediate, import resolution, debug output, exit code 71\n";
 std::cout<<"PASS: sum, register widths, carry, calls, REX, rejection paths, permissions, budget, 3000 malformed PE mutations\n";
}
