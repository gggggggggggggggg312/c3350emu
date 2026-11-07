#include "cpu/cpu.hpp"
#include <cassert>
#include <stdexcept>
int main(){
 Memory m;m.map(0x1000,0x1000,true,"RAM");
 const std::uint32_t p[]={0xE3A00001u,0xE2801002u,0xE5821000u,0xE5923000u};
 for(unsigned i=0;i<4;++i)m.write32le(0x1000+i*4,p[i]);
 CPU cpu(m);cpu.reset(0x1000);cpu.set_register(2,0x1100);
 cpu.step();assert(cpu.get_register(0)==1&&cpu.getPC()==0x1004);
 cpu.step();assert(cpu.get_register(1)==3);
 cpu.step();assert(m.read32le(0x1100)==3);
 cpu.step();assert(cpu.get_register(3)==3&&cpu.instructions_executed()==4);
 m.write32le(0x1010,0x03A04055u);cpu.set_register(15,0x1010);cpu.step();assert(cpu.get_register(4)==0);
 // MUL r6, r1, r2
 m.write32le(0x1020,0xE0060291u);
 cpu.set_register(1,7);cpu.set_register(2,6);cpu.set_register(15,0x1020);cpu.step();
 assert(cpu.get_register(6)==42);
 // MLA r7, r1, r2, r3 = 7*6+5
 m.write32le(0x1024,0xE0273291u);
 cpu.set_register(3,5);cpu.set_register(15,0x1024);cpu.step();
 assert(cpu.get_register(7)==47);
 // MULS r8, r1, r2 updates N/Z from the result.
 m.write32le(0x1028,0xE0180291u);
 cpu.set_register(1,0xffffffffu);cpu.set_register(2,2);cpu.set_register(15,0x1028);cpu.step();
 assert(cpu.get_register(8)==0xfffffffeu);
 assert((cpu.get_cpsr()&(1u<<31))!=0);
 // STMIA r4!, {r0-r2}; then LDMIA r5!, {r8-r10}.
 m.write32le(0x1030,0xE8A40007u);
 m.write32le(0x1034,0xE8B50700u);
 cpu.set_register(0,0x11);cpu.set_register(1,0x22);cpu.set_register(2,0x33);
 cpu.set_register(4,0x1200);cpu.set_register(5,0x1200);cpu.set_register(15,0x1030);
 cpu.step();assert(m.read32le(0x1200)==0x11&&m.read32le(0x1204)==0x22&&m.read32le(0x1208)==0x33);
 assert(cpu.get_register(4)==0x120c);
 cpu.set_register(15,0x1034);cpu.step();
 assert(cpu.get_register(8)==0x11&&cpu.get_register(9)==0x22&&cpu.get_register(10)==0x33);
 assert(cpu.get_register(5)==0x120c);
 // BX enters Thumb state; Thumb MOVS, ADDS and STR/LDR execute.
 m.write32le(0x1040,0xE12FFF11u); // BX r1
 m.write8(0x1100,0x05);m.write8(0x1101,0x21); // MOVS r1,#5
 m.write8(0x1102,0x03);m.write8(0x1103,0x30); // ADDS r0,#3
 m.write8(0x1104,0x00);m.write8(0x1105,0x60); // STR r0,[r0,#0]
 cpu.set_register(1,0x1101);cpu.set_register(15,0x1040);cpu.step();
 assert((cpu.get_cpsr()&(1u<<5))!=0&&cpu.getPC()==0x1100);
 cpu.step();assert(cpu.get_register(1)==5);
 cpu.step();assert(cpu.get_register(0)==3);
 // Thumb BX back to ARM.
 m.write8(0x1106,0x08);m.write8(0x1107,0x47); // BX r1
 cpu.set_register(1,0x1044);cpu.step();assert((cpu.get_cpsr()&(1u<<5))==0&&cpu.getPC()==0x1044);
 cpu.set_register(15,0x90000000u);bool threw=false;try{cpu.step();}catch(const std::out_of_range&){threw=true;}assert(threw);
 return 0;
}
