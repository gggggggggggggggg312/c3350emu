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
 cpu.set_register(15,0x90000000u);bool threw=false;try{cpu.step();}catch(const std::out_of_range&){threw=true;}assert(threw);
 return 0;
}