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
 m.write8(0x1104,0x10);m.write8(0x1105,0x60); // STR r0,[r2,#0]
 m.write8(0x1106,0x13);m.write8(0x1107,0x68); // LDR r3,[r2,#0]
 cpu.set_register(1,0x1101);cpu.set_register(15,0x1040);cpu.step();
 assert((cpu.get_cpsr()&(1u<<5))!=0&&cpu.getPC()==0x1100);
 cpu.step();assert(cpu.get_register(1)==5);
 cpu.step();assert(cpu.get_register(0)==3);
 // Thumb BX back to ARM.
 m.write8(0x1108,0x08);m.write8(0x1109,0x47); // BX r1
 cpu.set_register(2,0x120c);cpu.step();assert(m.read32le(0x120c)==3);
 cpu.step();assert(cpu.get_register(3)==3);
 cpu.set_register(1,0x1044);cpu.step();assert((cpu.get_cpsr()&(1u<<5))==0&&cpu.getPC()==0x1044);
 // Thumb register-offset STR/LDR.
 m.write8(0x1120,0x10);m.write8(0x1121,0x50); // STR r0,[r2,r0]
 m.write8(0x1122,0x13);m.write8(0x1123,0x58); // LDR r3,[r2,r0]
 cpu.set_register(0,3);cpu.set_register(2,0x1300);cpu.set_register(15,0x1120);
 cpu.set_register(15,0x1120); // resume in Thumb state after BX setup below
 cpu.set_register(1,0x1121); // no-op marker to keep the test values explicit
 // Enter Thumb state through the ARM BX instruction.
 cpu.set_register(15,0x1040);cpu.set_register(1,0x1121);cpu.step();
 cpu.step();assert(m.read8(0x1303)==3);
 cpu.step();assert(cpu.get_register(3)==3);
 // ARM halfword/signed transfers and register-controlled LSL.
 m.write32le(0x1050,0xE1C010B0u); // STRH r1,[r0]
 m.write32le(0x1054,0xE1D020B0u); // LDRH r2,[r0]
 m.write32le(0x1058,0xE1D030D0u); // LDRSB r3,[r0]
 m.write32le(0x105c,0xE1D040F0u); // LDRSH r4,[r0]
 m.write32le(0x1060,0xE1A05716u); // MOV r5,r6,LSL r7
 cpu.set_register(0,0x1400);cpu.set_register(1,0x0000ff80u);cpu.set_register(15,0x1050);
 cpu.step();assert(m.read8(0x1400)==0x80&&m.read8(0x1401)==0xff);
 cpu.step();assert(cpu.get_register(2)==0xff80u);
 cpu.step();assert(cpu.get_register(3)==0xffffff80u);
 cpu.step();assert(cpu.get_register(4)==0xffffff80u);
 cpu.set_register(6,3);cpu.set_register(7,2);cpu.step();assert(cpu.get_register(5)==12u);
 // ARM LDR/STR register offsets with immediate barrel shifts.
 m.write32le(0x1064,0xE7801102u); // STR r1,[r0,r2,LSL #2]
 m.write32le(0x1068,0xE7903102u); // LDR r3,[r0,r2,LSL #2]
 cpu.set_register(0,0x1500);cpu.set_register(1,0x12345678u);cpu.set_register(2,3);cpu.set_register(15,0x1064);
 cpu.step();assert(m.read32le(0x150c)==0x12345678u);
 cpu.set_register(2,3);cpu.step();assert(cpu.get_register(3)==0x12345678u);
 // LSR #1 offset: 8 >> 1 = 4.
 m.write32le(0x106c,0xE78011A2u); // STR r1,[r0,r2,LSR #1]
 cpu.set_register(0,0x1500);cpu.set_register(1,0xabcdef01u);cpu.set_register(2,8);cpu.set_register(15,0x106c);
 cpu.step();assert(m.read32le(0x1504)==0xabcdef01u);
 // ARM CLZ counts leading zero bits (including the zero-input case).
 m.write32le(0x1070,0xE16F5F16u); // CLZ r5,r6
 m.write32le(0x1074,0xE16F7F16u); // CLZ r7,r6
 cpu.set_register(6,0x00010000u);cpu.set_register(15,0x1070);cpu.step();assert(cpu.get_register(5)==15u);
 cpu.set_register(6,0);cpu.step();assert(cpu.get_register(7)==32u);
 // ARM SWP and SWPB exchange memory with a register.
 m.write32le(0x1078,0xE1003091u); // SWP r3,r1,[r0]
 m.write32le(0x107c,0xE1404091u); // SWPB r4,r1,[r0]
 m.write32le(0x1600,0x11223344u);cpu.set_register(0,0x1600);cpu.set_register(1,0xaabbccddu);cpu.set_register(15,0x1078);
 cpu.step();assert(cpu.get_register(3)==0x11223344u&&m.read32le(0x1600)==0xaabbccddu);
 cpu.set_register(1,0x55u);cpu.set_register(15,0x107c);cpu.step();
 assert(cpu.get_register(4)==0xddu&&m.read32le(0x1600)==0xaabbcc55u);
 // CP15 system-control register transfers: MCR/MRC c1, c2, c3 and c13.
 // MCR p15,0,r0,c1,c0,0 ; MRC p15,0,r1,c1,c0,0
 m.write32le(0x1080,0xEE010F10u);
 m.write32le(0x1084,0xEE111F10u);
 cpu.set_register(0,0x00001005u);cpu.set_register(15,0x1080);cpu.step();
 cpu.set_register(15,0x1084);cpu.step();assert(cpu.get_register(1)==0x00001005u);
 // TTBR0 and Domain Access Control register round trips.
 m.write32le(0x1088,0xEE020F10u); // MCR p15,0,r0,c2,c0,0
 m.write32le(0x108c,0xEE123F10u); // MRC p15,0,r3,c2,c0,0
 cpu.set_register(0,0x00104000u);cpu.set_register(15,0x1088);cpu.step();
 cpu.set_register(15,0x108c);cpu.step();assert(cpu.get_register(3)==0x00104000u);
 m.write32le(0x1090,0xEE030F10u); // MCR p15,0,r0,c3,c0,0
 m.write32le(0x1094,0xEE134F10u); // MRC p15,0,r4,c3,c0,0
 cpu.set_register(0,3u);cpu.set_register(15,0x1090);cpu.step();
 cpu.set_register(15,0x1094);cpu.step();assert(cpu.get_register(4)==3u);
 // MIDR is readable and fixed for the selected ARM1176JZF-S r0p7 model.
 m.write32le(0x1098,0xEE101F10u); // MRC p15,0,r1,c0,c0,0
 cpu.set_register(15,0x1098);cpu.step();assert(cpu.get_register(1)==0x410FB767u);
 // ARM long multiply family: UMULL, UMLAL, SMULL and SMLAL.
 // UMULL r1,r2,r3,r4: 0x12345678 * 0x10 = 0x00000001 0x23456780
 m.write32le(0x10a0,0xE0821493u);
 cpu.set_register(3,0x12345678u);cpu.set_register(4,0x10u);cpu.set_register(15,0x10a0);cpu.step();
 assert(cpu.get_register(1)==0x23456780u&&cpu.get_register(2)==1u);
 // UMLAL adds the existing 64-bit value (5:6) to the product.
 m.write32le(0x10a4,0xE0A21493u);
 cpu.set_register(1,6u);cpu.set_register(2,5u);cpu.set_register(15,0x10a4);cpu.step();
 assert(cpu.get_register(1)==0x23456786u&&cpu.get_register(2)==6u);
 // SMULL: -2 * 3 = 0xffffffff fffffffa.
 m.write32le(0x10a8,0xE0C21493u);
 cpu.set_register(3,0xfffffffeu);cpu.set_register(4,3u);cpu.set_register(15,0x10a8);cpu.step();
 assert(cpu.get_register(1)==0xfffffffau&&cpu.get_register(2)==0xffffffffu);
 // SMLAL adds a signed product to a 64-bit accumulator.
 m.write32le(0x10ac,0xE0E21493u);
 cpu.set_register(1,5u);cpu.set_register(2,0u);cpu.set_register(3,0xfffffffeu);cpu.set_register(4,3u);cpu.set_register(15,0x10ac);cpu.step();
 assert(cpu.get_register(1)==0xffffffffu&&cpu.get_register(2)==0xffffffffu);
 // Long multiply flags: N/Z reflect the full 64-bit result.
 m.write32le(0x10b0,0xE0921493u); // UMULLS r1,r2,r3,r4
 cpu.set_register(3,0u);cpu.set_register(4,0xffffffffu);cpu.set_register(15,0x10b0);cpu.step();
 assert(cpu.get_register(1)==0u&&cpu.get_register(2)==0u&&(cpu.get_cpsr()&(1u<<30))!=0);
 cpu.set_register(15,0x90000000u);bool threw=false;try{cpu.step();}catch(const std::out_of_range&){threw=true;}assert(threw);
 return 0;
}
