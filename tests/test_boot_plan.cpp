#include "firmware/boot_plan.hpp"
#include "firmware/fls_parser.hpp"
#include "memory/memory.hpp"
#include "cpu/cpu.hpp"
#include "devices/lcd.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
void write_word(std::ofstream& out,std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(static_cast<char>(v>>(i*8)));}
}
int main(){
 const auto dir=std::filesystem::temp_directory_path()/"c3350emu_boot_plan_test";
 std::filesystem::create_directories(dir);
 const auto a=dir/"stage_a.fls",b=dir/"stage_b.fls",plan_path=dir/"boot.plan";
 try{
  {std::ofstream out(a,std::ios::binary);write_word(out,0xE3A0002Au);write_word(out,0xEA000002u);}
  {std::ofstream out(b,std::ios::binary);write_word(out,0xE3A01063u);}
  {std::ofstream out(plan_path);out<<"# Two loaded regions, one shared address space\n"
   <<"RAM 0x1000 0x100\nENTRY 0x1000\nLIMIT 3\nBREAK 0x1014\n"
   <<"SEGMENT \"stage_a.fls\" 0 8 0x1000\n"
   <<"SEGMENT \"stage_b.fls\" 0 4 0x1010\n"
   <<"ZERO 0x1020 4\nCOPY 0x1010 0x1020 4\nZERO 0x1024 4\n"
   <<"STACK 0x1100\nREG 0 0x1234\nREG 1 0x5678\n"
   <<"STUB 0x60000000 0x100 0\nLCD 0x2000\nPATCH 0x1028 00000000 78563412\n";}
  const auto p=firmware::parse_boot_plan(plan_path);
  require(p.segments.size()==2,"Plan did not parse both firmware regions");
  require(p.breakpoints.size()==1&&p.breakpoints[0]==0x1014u,"Breakpoint directive failed");
  require(p.has_lcd&&p.lcd_base==0x2000u,"LCD directive failed");
  require(p.patches.size()==1,"PATCH directive failed");
  Memory memory;memory.map(p.ram_base,p.ram_size,true,"boot-plan test RAM");
  for(const auto& seg:p.segments)firmware::FlsParser::load_raw_segment(seg.file,seg.offset,seg.length,seg.load_address,memory);
  firmware::apply_memory_initialization(p,memory);
  firmware::apply_firmware_patches(p,memory);
  require(memory.read32le(0x1028u)==0x12345678u,"Checked firmware patch was not applied");
  LcdController lcd;lcd.attach(memory,p.lcd_base);
  memory.write32le(p.lcd_base+0x08u,0u);memory.write32le(p.lcd_base+0x0cu,0x0003f000u);
  require(lcd.pixels_argb()[0]==0xffff0000u,"LCD RGB666 framebuffer did not receive a red pixel");
  require(memory.read32le(0x1020u)==0xE3A01063u,"COPY did not stage initialized data");
  require(memory.read32le(0x1024u)==0u,"ZERO did not clear BSS-like memory");
  require(p.has_stack_pointer&&p.stack_pointer==0x1100u,"STACK directive failed");
  require(p.initial_registers.size()==2,"REG directives failed");
  require(p.memory_stubs.size()==1&&p.memory_stubs[0].base==0x60000000u,"STUB directive failed");
  memory.map_filled(p.memory_stubs[0].base,p.memory_stubs[0].size,true,"test MMIO stub",p.memory_stubs[0].initial_value);
  require(memory.read32le(0x60000000u)==0u,"STUB initial value was not applied");
  memory.write32le(0x60000000u,0x12345678u);
  require(memory.read32le(0x60000000u)==0x12345678u,"STUB register backing did not retain writes");
  memory.write32le(0x60000000u,0x41u);
  memory.write32le(0x1030u,0xE5901000u);memory.write32le(0x1034u,0xE2811001u);memory.write32le(0x1038u,0xE5801000u);
  CPU stub_cpu(memory);stub_cpu.reset(0x1030u);stub_cpu.set_register(0,0x60000000u);stub_cpu.step();stub_cpu.step();stub_cpu.step();
  require(memory.read32le(0x60000000u)==0x42u,"CPU could not execute load/add/store against a mapped stub");
  CPU cpu(memory);cpu.reset(p.entry);for(const auto& reg:p.initial_registers)cpu.set_register(reg.index,reg.value);cpu.set_register(13,p.stack_pointer);
  require(cpu.get_register(0)==0x1234u&&cpu.get_register(1)==0x5678u,"Initial registers not applied");
  require(cpu.get_register(13)==0x1100u,"Initial stack pointer not applied");
  for(std::uint64_t i=0;i<p.instruction_limit&&cpu.instructions_executed()<p.instruction_limit;++i)cpu.step();
  require(cpu.get_register(0)==42,"First loaded region did not execute");
  require(cpu.get_register(1)==99,"Branch handoff to second loaded region failed");
  require(cpu.getPC()==0x1014,"CPU did not reach the end of the second region");
  bool overlap_rejected=false;
  {std::ofstream out(plan_path);out<<"RAM 0x1000 0x100\nENTRY 0x1000\nLIMIT 1\n"
   <<"SEGMENT \"stage_a.fls\" 0 8 0x1000\nSEGMENT \"stage_b.fls\" 0 4 0x1004\n";}
  try{(void)firmware::parse_boot_plan(plan_path);}catch(const std::runtime_error&){overlap_rejected=true;}
  require(overlap_rejected,"Overlapping load regions were not rejected");
  std::filesystem::remove_all(dir);return 0;
 }catch(...){std::error_code ec;std::filesystem::remove_all(dir,ec);throw;}
}
