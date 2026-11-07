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
 m.write32le(0x108c,0xEE032F10u); // MRC p15,0,r3,c2,c0,0
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

 // ARM CPSR/SPSR access and banked privileged registers.
 m.write32le(0x10b8,0xE10F0000u); // MRS r0,CPSR
 m.write32le(0x10bc,0xE129F001u); // MSR CPSR_c,r1
 m.write32le(0x10c0,0xEF000123u); // SVC #0x123
 m.write32le(0x10c4,0xE1B0F00Eu); // MOVS pc,lr (exception return)
 cpu.reset(0x10b8);cpu.step();
 assert(cpu.get_register(0)==0xD3u);
 // Save distinct SVC stack/link values, switch to IRQ, then switch back.
 cpu.set_register(13,0x11110000u);cpu.set_register(14,0x22220000u);
 cpu.set_register(1,0x12u);cpu.set_register(15,0x10bc);cpu.step();
 assert((cpu.get_cpsr()&0x1fu)==0x12u);
 assert(cpu.get_register(13)==0u&&cpu.get_register(14)==0u);
 cpu.set_register(13,0x33330000u);cpu.set_register(14,0x44440000u);
 cpu.set_register(1,0x13u);cpu.set_register(15,0x10bc);cpu.step();
 assert(cpu.get_register(13)==0x11110000u&&cpu.get_register(14)==0x22220000u);
 // Enter SVC from System mode; SPSR_svc stores the old CPSR.
 cpu.set_register(1,0x1fu);cpu.set_register(15,0x10bc);cpu.step();
 assert((cpu.get_cpsr()&0x1fu)==0x1fu);
 cpu.set_register(15,0x10c0);cpu.step();
 assert((cpu.get_cpsr()&0x1fu)==0x13u&&cpu.getPC()==0x08u);
 assert(cpu.get_register(14)==0x10c4u&&(cpu.get_cpsr()&(1u<<7))!=0);
 // Inspect SPSR_svc, then execute MOVS pc,lr to restore System mode.
 m.write32le(0x10c8,0xE14F2000u); // MRS r2,SPSR
 cpu.set_register(15,0x10c8);cpu.step();
 assert((cpu.get_register(2)&0x1fu)==0x1fu);
 cpu.set_register(15,0x10c4);cpu.step();
 assert((cpu.get_cpsr()&0x1fu)==0x1fu&&cpu.getPC()==0x10c4u);
 // FIQ has its own R8-R14 bank; user/system and exception modes share R8-R12.
 cpu.set_register(8,0x8888u);cpu.set_register(13,0xAAAAu);
 cpu.set_register(1,0x11u);cpu.set_register(15,0x10bc);cpu.step();
 assert(cpu.get_register(8)==0u&&cpu.get_register(13)==0u);
 cpu.set_register(8,0xF888u);cpu.set_register(13,0xFDDDu);
 cpu.set_register(1,0x1fu);cpu.set_register(15,0x10bc);cpu.step();
 assert(cpu.get_register(8)==0x8888u&&cpu.get_register(13)==0xAAAAu);

 // ARM BLX immediate transfers into Thumb state and links back to ARM.
 m.write32le(0x1300u,0xFA000000u); // BLX to PC+8, H=0
 m.write8(0x1308u,0x07u);m.write8(0x1309u,0x20u); // Thumb MOVS r0,#7
 m.write8(0x130au,0x70u);m.write8(0x130bu,0x47u); // Thumb BX lr
 m.write32le(0x1304u,0xE3A01009u); // ARM MOV r1,#9 (return address)
 cpu.reset(0x1300u);cpu.step();
 assert((cpu.get_cpsr()&(1u<<5))!=0&&cpu.getPC()==0x1308u);
 assert(cpu.get_register(14)==0x1304u);
 cpu.step();assert(cpu.get_register(0)==7u);
 cpu.step();assert((cpu.get_cpsr()&(1u<<5))==0&&cpu.getPC()==0x1304u);
 cpu.step();assert(cpu.get_register(1)==9u);
 // BLX register works in ARM and Thumb state and records the return state.
 m.write32le(0x1320u,0xE12FFF31u); // BLX r1 from ARM
 m.write32le(0x1340u,0xE12FFF1Eu); // BX lr back to Thumb caller
 m.write8(0x1310u,0x88u);m.write8(0x1311u,0x47u); // Thumb BLX r1
 cpu.reset(0x1320u);cpu.set_register(1,0x1311u);cpu.step();
 assert((cpu.get_cpsr()&(1u<<5))!=0&&cpu.getPC()==0x1310u);
 assert(cpu.get_register(14)==0x1324u);
 cpu.set_register(1,0x1340u);cpu.step();
 assert((cpu.get_cpsr()&(1u<<5))==0&&cpu.getPC()==0x1340u);
 assert(cpu.get_register(14)==0x1313u);
 cpu.step();assert((cpu.get_cpsr()&(1u<<5))!=0&&cpu.getPC()==0x1312u);
 cpu.set_register(15,0x90000000u);bool threw=false;try{cpu.step();}catch(const std::out_of_range&){threw=true;}assert(threw);
 // ARMv6 byte-reversal instructions used by low-level runtime code.
 m.write32le(0x1400u,0xE6BF0F30u); // REV r0,r0
 m.write32le(0x1404u,0xE6BF1FB0u); // REV16 r1,r1
 m.write32le(0x1408u,0xE6FF2FB0u); // REVSH r2,r2
 cpu.reset(0x1400u);cpu.set_register(0,0x12345678u);
 cpu.set_register(1,0x12345678u);cpu.set_register(2,0x00008001u);
 cpu.step();assert(cpu.get_register(0)==0x78563412u);
 cpu.step();assert(cpu.get_register(1)==0x34127856u);
 cpu.step();assert(cpu.get_register(2)==0xFFFF8001u);
 // Thumb REV family uses the same byte-order transformations.
 m.write8(0x1410u,0x08u);m.write8(0x1411u,0xBAu); // REV r0,r1
 m.write8(0x1412u,0x48u);m.write8(0x1413u,0xBAu); // REV16 r0,r1
 m.write8(0x1414u,0xC8u);m.write8(0x1415u,0xBAu); // REVSH r0,r1
 cpu.reset(0x1411u);cpu.set_register(1,0x12345678u);
 cpu.step();assert(cpu.get_register(0)==0x78563412u);
 cpu.step();assert(cpu.get_register(0)==0x34127856u);
 cpu.set_register(1,0x00008001u);cpu.step();assert(cpu.get_register(0)==0xFFFF8001u);
 // ARM MMU: program TTBR0, DACR and SCTLR, then fetch and access data
 // through a virtual section mapped to a different physical section.
 Memory mmu_memory;
 mmu_memory.map(0x00000000u,0x4000u,true,"L1 translation table and setup RAM");
 mmu_memory.map(0x00200000u,0x1000u,true,"Physical section");
 mmu_memory.write32le(0x00000004u,0x00200402u); // VA 0x00100000 -> PA 0x00200000, domain 0, AP=01
 mmu_memory.write32le(0x00000100u,0xEE020F10u); // MCR p15,0,r0,c2,c0,0 (TTBR0)
 mmu_memory.write32le(0x00000104u,0xEE032F10u); // MCR p15,0,r2,c3,c0,0 (DACR)
 mmu_memory.write32le(0x00000108u,0xEE013F10u); // MCR p15,0,r3,c1,c0,0 (SCTLR)
 mmu_memory.write32le(0x00200000u,0xE3A0002Au); // MOV r0,#42
 mmu_memory.write32le(0x00200004u,0xE5810000u); // STR r0,[r1]
 mmu_memory.write32le(0x00200008u,0xE5912000u); // LDR r2,[r1]
 CPU mmu_cpu(mmu_memory);
 mmu_cpu.reset(0x00000100u);
 mmu_cpu.set_register(0,0u); // TTBR0 = 0
 mmu_cpu.set_register(2,1u); // Domain 0 = client
 mmu_cpu.set_register(3,1u); // Enable MMU
 mmu_cpu.set_register(1,0x00100100u);
 mmu_cpu.step();mmu_cpu.step();mmu_cpu.step();
 assert(mmu_cpu.getPC()==0x0000010cu);
 mmu_cpu.set_register(15,0x00100000u);
 mmu_cpu.step(); // virtual instruction fetch
 assert(mmu_cpu.get_register(0)==42u);
 mmu_cpu.step(); // virtual data write
 assert(mmu_memory.read32le(0x00200100u)==42u);
 mmu_cpu.step(); // virtual data read
 assert(mmu_cpu.get_register(2)==42u);
 // A missing first-level descriptor must stop execution with a translation fault.
 mmu_memory.write32le(0x00000004u,0u);
 mmu_cpu.set_register(15,0x00100000u);
 bool mmu_fault=false;
 try{mmu_cpu.step();}catch(const std::runtime_error& e){
  mmu_fault=std::string(e.what()).find("MMU translation fault")!=std::string::npos;
 }
 assert(mmu_fault);
 // Coarse-table translation: map a 4 KB virtual page to physical RAM.
 Memory page_memory;
 page_memory.map(0x00000000u,0x5000u,true,"L1/L2 translation tables and setup RAM");
 page_memory.map(0x00200000u,0x1000u,true,"Mapped 4 KB physical page");
 page_memory.write32le(0x00000004u,0x00004001u); // L1 coarse descriptor, domain 0
 page_memory.write32le(0x00004000u,0x00200012u); // L2 small-page descriptor, AP=01
 page_memory.write32le(0x00000100u,0xEE020F10u); // MCR TTBR0
 page_memory.write32le(0x00000104u,0xEE032F10u); // MCR DACR
 page_memory.write32le(0x00000108u,0xEE013F10u); // MCR SCTLR
 page_memory.write32le(0x00200000u,0xE3A0002Au); // MOV r0,#42
 page_memory.write32le(0x00200004u,0xE5810000u); // STR r0,[r1]
 page_memory.write32le(0x00200008u,0xE5912000u); // LDR r2,[r1]
 CPU page_cpu(page_memory);
 page_cpu.reset(0x00000100u);
 page_cpu.set_register(0,0u);page_cpu.set_register(2,1u);page_cpu.set_register(3,1u);
 page_cpu.set_register(1,0x00100100u);
 page_cpu.step();page_cpu.step();page_cpu.step();
 page_cpu.set_register(15,0x00100000u);
 page_cpu.step();assert(page_cpu.get_register(0)==42u);
 page_cpu.step();assert(page_memory.read32le(0x00200100u)==42u);
 page_cpu.step();assert(page_cpu.get_register(2)==42u);
 // AP=00 denies access in a client domain.
 page_memory.write32le(0x00004000u,0x00200002u);
 page_cpu.set_register(15,0x00100000u);
 bool permission_fault=false;
 try{page_cpu.step();}catch(const std::runtime_error& e){
  permission_fault=std::string(e.what()).find("permission fault")!=std::string::npos;
 }
 assert(permission_fault);
 // Large-page translation: a 64 KB mapping through a coarse L2 table.
 Memory large_memory;
 large_memory.map(0x00000000u,0x5000u,true,"Translation tables and setup RAM");
 large_memory.map(0x00300000u,0x10000u,true,"Mapped 64 KB large page");
 large_memory.write32le(0x00000004u,0x00004001u);
 large_memory.write32le(0x00004000u,0x00300011u); // Large page, AP=01
 large_memory.write32le(0x00000100u,0xEE020F10u);
 large_memory.write32le(0x00000104u,0xEE032F10u);
 large_memory.write32le(0x00000108u,0xEE013F10u);
 large_memory.write32le(0x00300000u,0xE3A0002Au);
 CPU large_cpu(large_memory);
 large_cpu.reset(0x00000100u);
 large_cpu.set_register(0,0u);large_cpu.set_register(2,1u);large_cpu.set_register(3,1u);
 large_cpu.step();large_cpu.step();large_cpu.step();
 large_cpu.set_register(15,0x00100000u);
 large_cpu.step();
 assert(large_cpu.get_register(0)==42u);
 // Unified TLB: remapping a page does not change a cached translation
 // until an architected invalidation instruction is executed.
 Memory tlb_memory;
 tlb_memory.map(0x00000000u,0x4000u,true,"TLB tables and maintenance code");
 tlb_memory.map(0x00200000u,0x1000u,true,"Original physical page");
 tlb_memory.map(0x00300000u,0x1000u,true,"Replacement physical page");
 tlb_memory.write32le(0x00000004u,0x00200402u); // VA 0x00100000 -> PA 0x00200000
 tlb_memory.write32le(0x00000000u,0x00000402u); // Identity map maintenance code
 tlb_memory.write32le(0x00000100u,0xEE020F10u); // MCR TTBR0
 tlb_memory.write32le(0x00000104u,0xEE032F10u); // MCR DACR
 tlb_memory.write32le(0x00000108u,0xEE013F10u); // MCR SCTLR
 tlb_memory.write32le(0x0000110cu,0xEE080F17u); // MCR p15,0,r0,c8,c7,0: invalidate unified TLB
 tlb_memory.write32le(0x00200000u,0xE3A0002Au); // MOV r0,#42
 tlb_memory.write32le(0x00300000u,0xE3A00063u); // MOV r0,#99
 CPU tlb_cpu(tlb_memory);
 tlb_cpu.reset(0x00000100u);
 tlb_cpu.set_register(0,0u);tlb_cpu.set_register(2,1u);tlb_cpu.set_register(3,1u);
 tlb_cpu.step();tlb_cpu.step();tlb_cpu.step();
 tlb_cpu.set_register(15,0x00100000u);tlb_cpu.step();
 assert(tlb_cpu.get_register(0)==42u);
 // Change the page table. The cached mapping still points at the old page.
 tlb_memory.write32le(0x00000004u,0x00300402u);
 tlb_cpu.set_register(15,0x00100000u);tlb_cpu.step();
 assert(tlb_cpu.get_register(0)==42u);
 // Execute TLB invalidate-all from the identity-mapped maintenance region.
 tlb_cpu.set_register(0,0u);tlb_cpu.set_register(15,0x0000110cu);tlb_cpu.step();
 tlb_cpu.set_register(15,0x00100000u);tlb_cpu.step();
 assert(tlb_cpu.get_register(0)==99u);
 // Separate ARM instruction/data caches: stale lines remain stale until
 // the corresponding CP15 maintenance operation invalidates them.
 Memory cache_memory;
 cache_memory.map(0x1000u,0x2000u,true,"Cache test RAM");
 cache_memory.write32le(0x1000u,0xEE010F10u); // MCR SCTLR from r0
 cache_memory.write32le(0x1004u,0xEE070F15u); // Invalidate all I-cache
 cache_memory.write32le(0x1008u,0xEE070F16u); // Invalidate all D-cache
 cache_memory.write32le(0x1100u,0xE3A00001u); // MOV r0,#1
 cache_memory.write32le(0x1104u,0xE5912000u); // LDR r2,[r1]
 cache_memory.write32le(0x1200u,5u);
 CPU cache_cpu(cache_memory);
 cache_cpu.reset(0x1000u);
 cache_cpu.set_register(0,0x00001004u); // Enable I-cache and D-cache
 cache_cpu.step();
 cache_cpu.set_register(15,0x1100u);cache_cpu.step();
 assert(cache_cpu.get_register(0)==1u);
 // Modify backing RAM directly: the I-cache must still return old code.
 cache_memory.write32le(0x1100u,0xE3A00002u);
 cache_cpu.set_register(15,0x1100u);cache_cpu.step();
 assert(cache_cpu.get_register(0)==1u);
 // Invalidate I-cache, then the changed instruction becomes visible.
 cache_cpu.set_register(15,0x1004u);cache_cpu.step();
 cache_cpu.set_register(15,0x1100u);cache_cpu.step();
 assert(cache_cpu.get_register(0)==2u);
 // D-cache is independent and also requires its own invalidation.
 cache_cpu.set_register(1,0x1200u);cache_cpu.set_register(15,0x1104u);cache_cpu.step();
 assert(cache_cpu.get_register(2)==5u);
 cache_memory.write32le(0x1200u,9u);
 cache_cpu.set_register(15,0x1104u);cache_cpu.step();
 assert(cache_cpu.get_register(2)==5u);
 cache_cpu.set_register(0,0x1200u);
 cache_cpu.set_register(15,0x1008u);cache_cpu.step();
 cache_cpu.set_register(15,0x1104u);cache_cpu.step();
 assert(cache_cpu.get_register(2)==9u);
 // ARMv6 word-exclusive accesses: LDREX establishes a reservation;
 // STREX succeeds once, then fails after the reservation has been consumed.
 Memory exclusive_memory;
 exclusive_memory.map(0x1000u,0x1000u,true,"Exclusive access test RAM");
 exclusive_memory.write32le(0x1000u,0xE1901F9Fu); // LDREX r1,[r0]
 exclusive_memory.write32le(0x1004u,0xE1802F93u); // STREX r2,r3,[r0]
 exclusive_memory.write32le(0x1008u,0xE1804F93u); // STREX r4,r3,[r0], reservation already consumed
 exclusive_memory.write32le(0x1100u,0x11223344u);
 CPU exclusive_cpu(exclusive_memory);
 exclusive_cpu.reset(0x1000u);
 exclusive_cpu.set_register(0,0x1100u);
 exclusive_cpu.set_register(3,0xAABBCCDDu);
 exclusive_cpu.step();
 assert(exclusive_cpu.get_register(1)==0x11223344u);
 exclusive_cpu.step();
 assert(exclusive_cpu.get_register(2)==0u);
 assert(exclusive_memory.read32le(0x1100u)==0xAABBCCDDu);
 exclusive_cpu.set_register(3,0x55667788u);
 exclusive_cpu.step();
 assert(exclusive_cpu.get_register(4)==1u);
 assert(exclusive_memory.read32le(0x1100u)==0xAABBCCDDu);
 // An ordinary CPU store clears an outstanding exclusive reservation.
 exclusive_memory.write32le(0x1010u,0xE1901F9Fu); // LDREX r1,[r0]
 exclusive_memory.write32le(0x1014u,0xE5803000u); // STR r3,[r0]
 exclusive_memory.write32le(0x1018u,0xE1802F93u); // STREX r2,r3,[r0]
 exclusive_cpu.set_register(15,0x1010u);
 exclusive_cpu.step();exclusive_cpu.step();exclusive_cpu.step();
 assert(exclusive_cpu.get_register(2)==1u);

 // Data Abort: an unmapped LDR enters Abort mode at vector 0x10.
 Memory abort_memory;
 abort_memory.map(0x00000000u,0x2000u,true,"Abort vectors and test code");
 abort_memory.write32le(0x00001000u,0xE5910000u); // LDR r0,[r1]
 abort_memory.write32le(0x00000010u,0xE3A0202Au); // Abort handler: MOV r2,#42
 CPU abort_cpu(abort_memory);
 abort_cpu.reset(0x1000u);
 abort_cpu.set_register(1,0x90000000u);
 abort_cpu.step();
 assert(abort_cpu.getPC()==0x10u);
 assert((abort_cpu.get_cpsr()&0x1fu)==0x17u);
 assert(abort_cpu.get_data_fault_address()==0x90000000u);
 assert(abort_cpu.get_data_fault_status()==0x8u);
 assert(abort_cpu.get_register(14)==0x1008u);
 abort_cpu.step();
 assert(abort_cpu.get_register(2)==42u);
 // Prefetch Abort: fetching an instruction from unmapped memory enters 0x0C.
 CPU prefetch_cpu(abort_memory);
 prefetch_cpu.reset(0x90000000u);
 prefetch_cpu.step();
 assert(prefetch_cpu.getPC()==0x0Cu);
 assert((prefetch_cpu.get_cpsr()&0x1fu)==0x17u);
 assert(prefetch_cpu.get_instruction_fault_address()==0x90000000u);
 assert(prefetch_cpu.get_instruction_fault_status()==0x8u);
 assert(prefetch_cpu.get_register(14)==0x90000004u);
 return 0;
}
