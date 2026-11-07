#include "cpu.hpp"
#include <stdexcept>
namespace {
constexpr std::uint32_t N=1u<<31, Z=1u<<30, C=1u<<29, V=1u<<28, NZCV=N|Z|C|V;
std::uint32_t ror(std::uint32_t x,unsigned n){n&=31u;return n?((x>>n)|(x<<(32u-n))):x;}
}
CPU::CPU(Memory& m):memory_(m){}
void CPU::reset(std::uint32_t entry){registers_.fill(0);registers_[15]=entry;cpsr_=0xD3;instructions_executed_=0;}
std::uint32_t CPU::getPC()const{return registers_[15];}
std::uint32_t CPU::get_register(unsigned i)const{if(i>=16)throw std::out_of_range("ARM register index");return registers_[i];}
void CPU::set_register(unsigned i,std::uint32_t v){if(i>=16)throw std::out_of_range("ARM register index");registers_[i]=v;}
std::uint32_t CPU::get_cpsr()const{return cpsr_;}
std::uint64_t CPU::instructions_executed()const noexcept{return instructions_executed_;}
std::uint32_t CPU::read_operand_register(unsigned i)const{if(i>=16)throw std::out_of_range("ARM register index");return i==15?registers_[15]+8u:registers_[i];}
bool CPU::condition_passed(unsigned q)const{
 const bool n=cpsr_&N,z=cpsr_&Z,c=cpsr_&C,v=cpsr_&V;
 switch(q){case 0:return z;case 1:return !z;case 2:return c;case 3:return !c;case 4:return n;case 5:return !n;case 6:return v;case 7:return !v;case 8:return c&&!z;case 9:return !c||z;case 10:return n==v;case 11:return n!=v;case 12:return !z&&(n==v);case 13:return z||(n!=v);case 14:return true;default:return false;}
}
void CPU::step(){
 const auto address=registers_[15];const auto ins=memory_.read32le(address);registers_[15]=address+4u;++instructions_executed_;
 if(!condition_passed(ins>>28))return;
 if((ins&0x0E000000u)==0x0A000000u)execute_branch(ins);
 else if((ins&0x0C000000u)==0x04000000u)execute_single_transfer(ins);
 else if((ins&0x0C000000u)==0||((ins&0x0C000000u)==0x02000000u))execute_data_processing(ins);
 else throw std::runtime_error("Unsupported ARM instruction encoding");
}
void CPU::execute_branch(std::uint32_t ins){
 const auto off=static_cast<std::int32_t>(ins<<8)>>6;const auto target=registers_[15]+static_cast<std::uint32_t>(off);
 if(ins&(1u<<24))registers_[14]=registers_[15]-4u;registers_[15]=target;
}
void CPU::execute_data_processing(std::uint32_t ins){
 const bool imm=ins&(1u<<25),s=ins&(1u<<20);const unsigned op=(ins>>21)&15,rn=(ins>>16)&15,rd=(ins>>12)&15;
 std::uint32_t b=0;bool shcarry=cpsr_&C;
 if(imm){const unsigned rot=((ins>>8)&15)*2;b=ror(ins&255u,rot);if(rot)shcarry=b>>31;}
 else{
  if(ins&(1u<<4))throw std::runtime_error("Register-specified shifts not implemented");
  const unsigned rm=ins&15,type=(ins>>5)&3,amount=(ins>>7)&31;const auto val=read_operand_register(rm);
  if(!amount)b=val;
  else if(type==0){b=val<<amount;shcarry=(val>>(32-amount))&1u;}
  else if(type==1){b=val>>amount;shcarry=(val>>(amount-1))&1u;}
  else if(type==2){b=static_cast<std::uint32_t>(static_cast<std::int32_t>(val)>>amount);shcarry=(val>>(amount-1))&1u;}
  else{b=ror(val,amount);shcarry=b>>31;}
 }
 const std::uint32_t a=read_operand_register(rn);std::uint32_t r=0;bool arithmetic=false,carry=false,overflow=false;
 switch(op){
 case 0:r=a&b;break;case 1:r=a^b;break;
 case 2:r=a-b;arithmetic=true;carry=a>=b;overflow=(((a^b)&(a^r))>>31)!=0;break;
 case 3:r=b-a;arithmetic=true;carry=b>=a;overflow=(((b^a)&(b^r))>>31)!=0;break;
 case 4:{std::uint64_t t=std::uint64_t(a)+b;r=std::uint32_t(t);arithmetic=true;carry=t>>32;overflow=((~(a^b)&(a^r))>>31)!=0;break;}
 case 8:r=a&b;break;case 9:r=a^b;break;
 case 10:r=a-b;arithmetic=true;carry=a>=b;overflow=(((a^b)&(a^r))>>31)!=0;break;
 case 11:{std::uint64_t t=std::uint64_t(a)+b;r=std::uint32_t(t);arithmetic=true;carry=t>>32;overflow=((~(a^b)&(a^r))>>31)!=0;break;}
 case 12:r=a|b;break;case 13:r=b;break;case 14:r=a&~b;break;case 15:r=~b;break;
 default:throw std::runtime_error("Unsupported ARM data-processing opcode");
 }
 const bool test=op>=8&&op<=11;if(!test){if(rd==15)registers_[15]=r;else registers_[rd]=r;}
 if(s||test){cpsr_&=~NZCV;if(r&0x80000000u)cpsr_|=N;if(!r)cpsr_|=Z;if(arithmetic){if(carry)cpsr_|=C;if(overflow)cpsr_|=V;}else if(shcarry)cpsr_|=C;}
}
void CPU::execute_single_transfer(std::uint32_t ins){
 const bool reg=ins&(1u<<25),pre=ins&(1u<<24),up=ins&(1u<<23),byte=ins&(1u<<22),wb=ins&(1u<<21),load=ins&(1u<<20);
 const unsigned rn=(ins>>16)&15,rd=(ins>>12)&15;if(reg&&(ins&(1u<<4)))throw std::runtime_error("Register-shifted transfer offsets not implemented");
 const auto base=read_operand_register(rn);const auto off=reg?read_operand_register(ins&15):(ins&0xfffu);const auto adjusted=up?base+off:base-off;const auto address=pre?adjusted:base;
 if(load){const auto value=byte?memory_.read8(address):memory_.read32le(address);if(rd==15)registers_[15]=value;else registers_[rd]=value;}
 else{const auto value=read_operand_register(rd);if(byte)memory_.write8(address,std::uint8_t(value));else memory_.write32le(address,value);}
 if(!pre||wb)registers_[rn]=adjusted;
}
