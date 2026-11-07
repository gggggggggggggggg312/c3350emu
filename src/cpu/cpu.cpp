#include "cpu.hpp"
#include <stdexcept>
namespace {
constexpr std::uint32_t N=1u<<31, Z=1u<<30, C=1u<<29, V=1u<<28, NZCV=N|Z|C|V;
std::uint32_t ror(std::uint32_t x,unsigned n){n&=31u;return n?((x>>n)|(x<<(32u-n))):x;}
}
CPU::CPU(Memory& m):memory_(m){}
void CPU::reset(std::uint32_t entry){registers_.fill(0);user_bank_.fill(0);fiq_bank_.fill(0);irq_bank_.fill(0);svc_bank_.fill(0);abort_bank_.fill(0);undefined_bank_.fill(0);spsr_fiq_=spsr_irq_=spsr_svc_=spsr_abort_=spsr_undefined_=0;cp15_control_=cp15_ttbr0_=cp15_dacr_=cp15_context_id_=0;cpsr_=0xD3;if(entry&1u){cpsr_|=(1u<<5);registers_[15]=entry&~1u;}else registers_[15]=entry;instructions_executed_=0;}
std::uint32_t CPU::translate_address(std::uint32_t va,bool write)const{
 if((cp15_control_&1u)==0u)return va;
 const std::uint32_t l1_base=cp15_ttbr0_&0xffffc000u;
 const std::uint32_t l1=memory_.read32le(l1_base+((va>>20)&0xfffu)*4u);
 const unsigned l1_type=l1&3u;
 std::uint32_t physical=0;
 unsigned domain=(l1>>5)&15u;
 unsigned ap=0;
 const bool privileged=(cpsr_&0x1fu)!=0x10u;
 if(l1_type==2u){
  ap=(l1>>10)&3u;
  physical=(l1&0xfff00000u)|(va&0x000fffffu);
 }else if(l1_type==1u){
  // Coarse page table: 256 second-level descriptors, each covering 4 KB.
  const std::uint32_t l2_base=l1&0xfffffc00u;
  const std::uint32_t l2=memory_.read32le(l2_base+(((va>>12)&0xffu)*4u));
  const unsigned l2_type=l2&3u;
  if(l2_type==1u){
   // Large page: 64 KB, with four 16 KB permission subregions.
   const unsigned subpage=(va>>14)&3u;
   ap=(l2>>(4u+subpage*2u))&3u;
   physical=(l2&0xffff0000u)|(va&0xffffu);
  }else if(l2_type==2u||l2_type==3u){
   // Small page: 4 KB, with four 1 KB permission subregions.
   const unsigned subpage=(va>>10)&3u;
   ap=(l2>>(4u+subpage*2u))&3u;
   physical=(l2&0xfffff000u)|(va&0xfffu);
  }else{
   throw std::runtime_error("MMU translation fault: invalid second-level descriptor");
  }
 }else{
  throw std::runtime_error("MMU translation fault: invalid first-level descriptor");
 }
 const unsigned domain_access=(cp15_dacr_>>(domain*2u))&3u;
 if(domain_access==0u||domain_access==2u)throw std::runtime_error("MMU domain fault");
 if(domain_access!=3u){
  bool allowed=false;
  if(ap==1u)allowed=privileged;
  else if(ap==2u)allowed=privileged||!write;
  else if(ap==3u)allowed=true;
  if(!allowed)throw std::runtime_error("MMU page/section permission fault");
 }
 return physical;
}
std::uint8_t CPU::read_virtual8(std::uint32_t a)const{return memory_.read8(translate_address(a,false));}
std::uint32_t CPU::read_virtual32(std::uint32_t a)const{return memory_.read32le(translate_address(a,false));}
void CPU::write_virtual8(std::uint32_t a,std::uint8_t v){memory_.write8(translate_address(a,true),v);}
void CPU::write_virtual32(std::uint32_t a,std::uint32_t v){memory_.write32le(translate_address(a,true),v);}
std::uint32_t CPU::getPC()const{return registers_[15];}
std::uint32_t CPU::get_register(unsigned i)const{if(i>=16)throw std::out_of_range("ARM register index");return registers_[i];}
void CPU::set_register(unsigned i,std::uint32_t v){if(i>=16)throw std::out_of_range("ARM register index");registers_[i]=v;}
std::uint32_t CPU::get_cpsr()const{return cpsr_;}
std::uint64_t CPU::instructions_executed()const noexcept{return instructions_executed_;}

std::uint32_t* CPU::active_spsr(){
 switch(cpsr_&0x1fu){
  case 0x11u:return &spsr_fiq_;
  case 0x12u:return &spsr_irq_;
  case 0x13u:return &spsr_svc_;
  case 0x17u:return &spsr_abort_;
  case 0x1bu:return &spsr_undefined_;
  default:return nullptr;
 }
}
void CPU::switch_mode(std::uint32_t next){
 const auto old=cpsr_&0x1fu;
 const auto valid=[](std::uint32_t m){return m==0x10u||m==0x11u||m==0x12u||m==0x13u||m==0x17u||m==0x1bu||m==0x1fu;};
 if(!valid(next))throw std::runtime_error("Invalid ARM processor mode");
 if(old==next)return;
 if(old==0x11u){for(unsigned i=0;i<7;++i)fiq_bank_[i]=registers_[8u+i];}
 else{
  for(unsigned i=0;i<5;++i)user_bank_[i]=registers_[8u+i];
  if(old==0x10u||old==0x1fu){user_bank_[5]=registers_[13];user_bank_[6]=registers_[14];}
  else{
   std::array<std::uint32_t,2>* b=nullptr;
   if(old==0x12u)b=&irq_bank_;else if(old==0x13u)b=&svc_bank_;else if(old==0x17u)b=&abort_bank_;else if(old==0x1bu)b=&undefined_bank_;
   if(b){(*b)[0]=registers_[13];(*b)[1]=registers_[14];}
  }
 }
 if(next==0x11u){for(unsigned i=0;i<7;++i)registers_[8u+i]=fiq_bank_[i];}
 else{
  for(unsigned i=0;i<5;++i)registers_[8u+i]=user_bank_[i];
  if(next==0x10u||next==0x1fu){registers_[13]=user_bank_[5];registers_[14]=user_bank_[6];}
  else{
   const std::array<std::uint32_t,2>* b=nullptr;
   if(next==0x12u)b=&irq_bank_;else if(next==0x13u)b=&svc_bank_;else if(next==0x17u)b=&abort_bank_;else if(next==0x1bu)b=&undefined_bank_;
   if(b){registers_[13]=(*b)[0];registers_[14]=(*b)[1];}
  }
 }
 cpsr_=(cpsr_&~0x1fu)|next;
}
void CPU::enter_svc_exception(std::uint32_t return_address){
 const auto saved=cpsr_;
 switch_mode(0x13u);
 spsr_svc_=saved;
 registers_[14]=return_address;
 cpsr_=(cpsr_|(1u<<7))&~(1u<<5);
 registers_[15]=0x08u;
}
void CPU::execute_status_transfer(std::uint32_t ins){
 const bool is_mrs=(ins&(1u<<21))==0;
 const bool use_spsr=(ins&(1u<<22))!=0;
 const unsigned rd=(ins>>12)&15u,rm=ins&15u;
 const auto mode=cpsr_&0x1fu;
 if(is_mrs){
  const auto* saved=use_spsr?active_spsr():nullptr;
  if(use_spsr&&!saved)throw std::runtime_error("MRS SPSR used in a mode without SPSR");
  if(rd==15u)throw std::runtime_error("MRS with PC destination is unpredictable");
  registers_[rd]=use_spsr?*saved:cpsr_;return;
 }
 const bool immediate=(ins&(1u<<25))!=0;
 const auto operand=immediate?ror(ins&0xffu,((ins>>8)&15u)*2u):read_operand_register(rm);
 const unsigned fields=(ins>>16)&15u;
 std::uint32_t mask=0;
 if(fields&1u)mask|=0x000000ffu;
 if(fields&2u)mask|=0x0000ff00u;
 if(fields&4u)mask|=0x00ff0000u;
 if(fields&8u)mask|=0xff000000u;
 if(use_spsr){
  auto* saved=active_spsr();
  if(!saved)throw std::runtime_error("MSR SPSR used in a mode without SPSR");
  *saved=(*saved&~mask)|(operand&mask);return;
 }
 if(mode==0x10u)mask&=0xf0000000u;
 else mask&=0xf00000ffu;
 const auto old=cpsr_;
 const auto next=(old&~mask)|(operand&mask);
 if((mask&0x1fu)!=0u&&(next&0x1fu)!=mode)switch_mode(next&0x1fu);
 cpsr_=(cpsr_&~mask)|(next&mask);
}
std::uint32_t CPU::read_operand_register(unsigned i)const{if(i>=16)throw std::out_of_range("ARM register index");return i==15?registers_[15]+8u:registers_[i];}
bool CPU::condition_passed(unsigned q)const{
 const bool n=cpsr_&N,z=cpsr_&Z,c=cpsr_&C,v=cpsr_&V;
 switch(q){case 0:return z;case 1:return !z;case 2:return c;case 3:return !c;case 4:return n;case 5:return !n;case 6:return v;case 7:return !v;case 8:return c&&!z;case 9:return !c||z;case 10:return n==v;case 11:return n!=v;case 12:return !z&&(n==v);case 13:return z||(n!=v);case 14:return true;default:return false;}
}
void CPU::step(){
 const auto address=registers_[15];
 if(cpsr_&(1u<<5)){
  const auto ins=static_cast<std::uint16_t>(read_virtual8(address) |
      (static_cast<std::uint16_t>(read_virtual8(address+1u))<<8));
  registers_[15]=address+2u;++instructions_executed_;
  execute_thumb(ins);return;
 }
 const auto ins=read_virtual32(address);registers_[15]=address+4u;++instructions_executed_;
 if(!condition_passed(ins>>28))return;
 if((ins&0x0FBF0FFFu)==0x010F0000u || (ins&0x0FB0FFF0u)==0x0120F000u || (ins&0x0FB0F000u)==0x0320F000u)
  execute_status_transfer(ins);
 else if((ins&0x0F000000u)==0x0F000000u)enter_svc_exception(registers_[15]);
 else if((ins&0x0F000010u)==0x0E000010u && ((ins>>8)&15u)==15u)
  execute_cp15_transfer(ins);
 else if((ins&0x0FFFFFF0u)==0x012FFF10u){
  const auto target=read_operand_register(ins&15u);
  if(target&1u){cpsr_|=(1u<<5);registers_[15]=target&~1u;}
  else{cpsr_&=~(1u<<5);registers_[15]=target&~3u;}
 }
 else if((ins&0x0FFF0FF0u)==0x016F0F10u){
  const unsigned rd=(ins>>12)&15u,rm=ins&15u;
  if(rd==15u||rm==15u)throw std::runtime_error("CLZ with PC register is unpredictable");
  const auto value=registers_[rm];
  unsigned count=0;
  if(value==0)count=32;
  else{
   std::uint32_t mask=0x80000000u;
   while((value&mask)==0u){++count;mask>>=1;}
  }
  registers_[rd]=count;
 }
 else if((ins&0x0FB00FF0u)==0x01000090u){
  const bool byte=(ins&(1u<<22))!=0;
  const unsigned rn=(ins>>16)&15u,rd=(ins>>12)&15u,rm=ins&15u;
  if(rn==15u||rd==15u||rm==15u)throw std::runtime_error("SWP with PC register is unpredictable");
  const auto address=registers_[rn],source=registers_[rm];
  if(byte){const auto old=read_virtual8(address);write_virtual8(address,static_cast<std::uint8_t>(source));registers_[rd]=old;}
  else{const auto old=read_virtual32(address);write_virtual32(address,source);registers_[rd]=old;}
 }
 else if((ins&0x0F8000F0u)==0x00800090u)execute_long_multiply(ins);
  else if((ins&0x0FC000F0u)==0x00000090u)execute_multiply(ins);
 else if((ins&0x0E000090u)==0x00000090u)execute_halfword_transfer(ins);
 else if((ins&0x0E000000u)==0x08000000u)execute_block_transfer(ins);
 else if((ins&0x0E000000u)==0x0A000000u)execute_branch(ins);
 else if((ins&0x0C000000u)==0x04000000u)execute_single_transfer(ins);
 else if((ins&0x0C000000u)==0||((ins&0x0C000000u)==0x02000000u))execute_data_processing(ins);
 else throw std::runtime_error("Unsupported ARM instruction encoding");
}
void CPU::execute_cp15_transfer(std::uint32_t ins){
 const bool read=(ins&(1u<<20))!=0;
 const unsigned opc1=(ins>>21)&7u,crn=(ins>>16)&15u,rd=(ins>>12)&15u;
 const unsigned opc2=(ins>>5)&7u,crm=ins&15u;
 if(opc1!=0u)throw std::runtime_error("Unsupported CP15 opc1 value");
 std::uint32_t* value=nullptr;
 if(crn==1u&&crm==0u&&opc2==0u)value=&cp15_control_;
 else if(crn==2u&&crm==0u&&opc2==0u)value=&cp15_ttbr0_;
 else if(crn==3u&&crm==0u&&opc2==0u)value=&cp15_dacr_;
 else if(crn==13u&&crm==0u&&opc2==0u)value=&cp15_context_id_;
 else if(crn==0u&&crm==0u&&opc2==0u){
  if(!read)throw std::runtime_error("CP15 MIDR is read-only");
  // ARM1176JZF-S r0p7 identification value; this is a selectable CPU model,
  // not proof that every GT-C3350 board uses this exact core revision.
  const std::uint32_t midr=0x410FB767u;
  if(rd==15u)cpsr_=(cpsr_&~NZCV)|(midr&NZCV);
  else registers_[rd]=midr;
  return;
 }else throw std::runtime_error("Unsupported CP15 register encoding");
 if(read){
  if(rd==15u)cpsr_=(cpsr_&~NZCV)|(*value&NZCV);
  else registers_[rd]=*value;
 }else *value=read_operand_register(rd);
}
void CPU::execute_branch(std::uint32_t ins){
 const auto off=static_cast<std::int32_t>(ins<<8)>>6;const auto target=registers_[15]+static_cast<std::uint32_t>(off);
 if(ins&(1u<<24))registers_[14]=registers_[15]-4u;
 registers_[15]=target;
}
void CPU::execute_long_multiply(std::uint32_t ins){
 const bool signed_multiply=(ins&(1u<<22))!=0;
 const bool accumulate=(ins&(1u<<21))!=0;
 const bool set_flags=(ins&(1u<<20))!=0;
 const unsigned rd_hi=(ins>>16)&15u,rd_lo=(ins>>12)&15u;
 const unsigned rs=(ins>>8)&15u,rm=ins&15u;
 if(rd_hi==15u||rd_lo==15u||rs==15u||rm==15u||rd_hi==rd_lo)
  throw std::runtime_error("Long multiply has invalid or overlapping registers");
 std::uint64_t result=0;
 if(signed_multiply){
  const auto a=static_cast<std::int64_t>(static_cast<std::int32_t>(registers_[rm]));
  const auto b=static_cast<std::int64_t>(static_cast<std::int32_t>(registers_[rs]));
  result=static_cast<std::uint64_t>(a*b);
 }else{
  result=static_cast<std::uint64_t>(registers_[rm])*registers_[rs];
 }
 if(accumulate){
  const std::uint64_t accumulator=(static_cast<std::uint64_t>(registers_[rd_hi])<<32)|registers_[rd_lo];
  result+=accumulator;
 }
 registers_[rd_lo]=static_cast<std::uint32_t>(result);
 registers_[rd_hi]=static_cast<std::uint32_t>(result>>32);
 if(set_flags){
  cpsr_&=~(N|Z);
  if(result&(std::uint64_t{1}<<63))cpsr_|=N;
  if(result==0)cpsr_|=Z;
 }
}
void CPU::execute_multiply(std::uint32_t ins){
 const bool accumulate=ins&(1u<<21),set_flags=ins&(1u<<20);
 const unsigned rd=(ins>>16)&15,rn=(ins>>12)&15,rs=(ins>>8)&15,rm=ins&15;
 if(rd==15||rs==15||rm==15||(accumulate&&rn==15))
  throw std::runtime_error("MUL/MLA with PC register is unsupported or unpredictable");
 std::uint32_t result=registers_[rm]*registers_[rs];
 if(accumulate)result+=registers_[rn];
 registers_[rd]=result;
 if(set_flags){
  cpsr_&=~(N|Z);
  if(result&0x80000000u)cpsr_|=N;
  if(result==0)cpsr_|=Z;
 }
}
void CPU::execute_data_processing(std::uint32_t ins){
 const bool imm=ins&(1u<<25),s=ins&(1u<<20);const unsigned op=(ins>>21)&15,rn=(ins>>16)&15,rd=(ins>>12)&15;
 std::uint32_t b=0;bool shcarry=cpsr_&C;
 if(imm){const unsigned rot=((ins>>8)&15)*2;b=ror(ins&255u,rot);if(rot)shcarry=b>>31;}
 else{
  const unsigned rm=ins&15,type=(ins>>5)&3;const auto val=read_operand_register(rm);
  if(ins&(1u<<4)){
   const unsigned rs=(ins>>8)&15;const unsigned amount=read_operand_register(rs)&0xffu;
   if(amount==0)b=val;
   else if(type==0){if(amount<32){b=val<<amount;shcarry=((val>>(32u-amount))&1u)!=0;}else if(amount==32){b=0;shcarry=(val&1u)!=0;}else{b=0;shcarry=false;}}
   else if(type==1){if(amount<32){b=val>>amount;shcarry=((val>>(amount-1u))&1u)!=0;}else if(amount==32){b=0;shcarry=(val>>31)!=0;}else{b=0;shcarry=false;}}
   else if(type==2){if(amount<32){b=static_cast<std::uint32_t>(static_cast<std::int32_t>(val)>>amount);shcarry=((val>>(amount-1u))&1u)!=0;}else{b=(val&0x80000000u)?0xffffffffu:0;shcarry=(val>>31)!=0;}}
   else {const unsigned n=amount&31u;if(n==0){b=val;shcarry=(val>>31)!=0;}else{b=ror(val,n);shcarry=(b>>31)!=0;}}
  }else{
   const unsigned amount=(ins>>7)&31u;
   if(!amount)b=val;
   else if(type==0){b=val<<amount;shcarry=((val>>(32u-amount))&1u)!=0;}
   else if(type==1){b=val>>amount;shcarry=((val>>(amount-1u))&1u)!=0;}
   else if(type==2){b=static_cast<std::uint32_t>(static_cast<std::int32_t>(val)>>amount);shcarry=((val>>(amount-1u))&1u)!=0;}
   else{b=ror(val,amount);shcarry=(b>>31)!=0;}
  }
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
 const bool test=op>=8&&op<=11;if(!test){
  if(rd==15&&s){
   const auto* saved=active_spsr();
   if(!saved)throw std::runtime_error("Exception return requested without an SPSR");
   const auto restored=*saved;
   const auto target=r;
   const auto target_mode=restored&0x1fu;
   if(target_mode!=(cpsr_&0x1fu))switch_mode(target_mode);
   cpsr_=restored;
   registers_[15]=(restored&(1u<<5))?(target&~1u):(target&~3u);
   return;
  }
  if(rd==15)registers_[15]=r;else registers_[rd]=r;
 }
 if(s||test){cpsr_&=~NZCV;if(r&0x80000000u)cpsr_|=N;if(!r)cpsr_|=Z;if(arithmetic){if(carry)cpsr_|=C;if(overflow)cpsr_|=V;}else if(shcarry)cpsr_|=C;}
}
void CPU::execute_single_transfer(std::uint32_t ins){
 const bool reg=ins&(1u<<25),pre=ins&(1u<<24),up=ins&(1u<<23),byte=ins&(1u<<22),wb=ins&(1u<<21),load=ins&(1u<<20);
 const unsigned rn=(ins>>16)&15,rd=(ins>>12)&15;
 if(reg&&(ins&(1u<<4)))throw std::runtime_error("Register-shifted transfer offsets are unpredictable");
 const auto base=read_operand_register(rn);
 std::uint32_t off=ins&0xfffu;
 if(reg){
  const auto value=read_operand_register(ins&15u);
  const unsigned type=(ins>>5)&3u,amount=(ins>>7)&31u;
  if(type==0u)off=amount?value<<amount:value;
  else if(type==1u)off=amount?value>>amount:0u;
  else if(type==2u)off=amount?static_cast<std::uint32_t>(static_cast<std::int32_t>(value)>>amount):((value&0x80000000u)?0xffffffffu:0u);
  else if(amount==0u)off=((cpsr_&C)?0x80000000u:0u)|(value>>1);
  else off=ror(value,amount);
 }
 const auto adjusted=up?base+off:base-off;const auto address=pre?adjusted:base;
 if(load){const auto value=byte?read_virtual8(address):read_virtual32(address);if(rd==15)registers_[15]=value;else registers_[rd]=value;}
 else{const auto value=read_operand_register(rd);if(byte)write_virtual8(address,std::uint8_t(value));else write_virtual32(address,value);}
 if(!pre||wb)registers_[rn]=adjusted;
}

void CPU::execute_halfword_transfer(std::uint32_t ins){
 const bool pre=ins&(1u<<24),up=ins&(1u<<23),immediate=ins&(1u<<22),writeback=ins&(1u<<21),load=ins&(1u<<20);
 const unsigned rn=(ins>>16)&15u,rd=(ins>>12)&15u,type=(ins>>5)&3u;
 if(type==0u)throw std::runtime_error("Invalid ARM halfword-transfer type");
 if(!load&&type!=1u)throw std::runtime_error("Signed ARM transfer is load-only");
 const std::uint32_t offset=immediate?((((ins>>8)&15u)<<4)|(ins&15u)):read_operand_register(ins&15u);
 const auto base=read_operand_register(rn),adjusted=up?base+offset:base-offset,address=pre?adjusted:base;
 const auto read16=[this](std::uint32_t a){return static_cast<std::uint16_t>(read_virtual8(a)|(static_cast<std::uint16_t>(read_virtual8(a+1u))<<8));};
 if(load){
  std::uint32_t value=0;
  if(type==1u)value=read16(address);
  else if(type==2u)value=static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(read_virtual8(address))));
  else value=static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(read16(address))));
  if(rd==15u)registers_[15]=value&~3u;else registers_[rd]=value;
 }else{
  const auto value=read_operand_register(rd);
  write_virtual8(address,static_cast<std::uint8_t>(value));write_virtual8(address+1u,static_cast<std::uint8_t>(value>>8));
 }
 if(!pre||writeback)registers_[rn]=adjusted;
}

void CPU::execute_block_transfer(std::uint32_t ins){
 const bool pre=ins&(1u<<24),up=ins&(1u<<23),writeback=ins&(1u<<21),load=ins&(1u<<20);
 const unsigned rn=(ins>>16)&15;
 const std::uint16_t list=static_cast<std::uint16_t>(ins&0xffffu);
 if(list==0)throw std::runtime_error("Empty ARM block-transfer register list is unsupported");
 unsigned count=0;
 for(unsigned bit=0;bit<16;++bit)if(list&(1u<<bit))++count;
 const std::uint32_t base=read_operand_register(rn);
 if(writeback&&(list&(1u<<rn)))
  throw std::runtime_error("Block transfer with writeback and base in register list is unsupported");
 std::uint32_t address;
 if(up) address=base+(pre?4u:0u);
 else address=base-(4u*count)+(pre?0u:4u);
 for(unsigned reg=0;reg<16;++reg){
  if(!(list&(1u<<reg)))continue;
  if(load){
   const auto value=read_virtual32(address);
   if(reg==15)registers_[15]=value;
   else registers_[reg]=value;
  }else{
   write_virtual32(address,read_operand_register(reg));
  }
  address+=4u;
 }
 if(writeback)registers_[rn]=up?base+4u*count:base-4u*count;
}

void CPU::execute_thumb(std::uint16_t ins){
 const auto set_nz=[this](std::uint32_t value){
  cpsr_&=~(N|Z);if(value&0x80000000u)cpsr_|=N;if(value==0)cpsr_|=Z;
 };
 const auto reg=[this](unsigned r)->std::uint32_t{
  return r==15?((registers_[15]+2u)&~3u):registers_[r];
 };
 // Format 1: immediate shifts.
 if((ins&0xE000u)==0x0000u && (ins&0x1800u)!=0x1800u){
  const unsigned op=(ins>>11)&3u,amount=(ins>>6)&31u,rs=(ins>>3)&7u,rd=ins&7u;
  const auto value=registers_[rs];std::uint32_t result=value;bool carry=(cpsr_&C)!=0;
  if(op==0){if(amount){carry=((value>>(32u-amount))&1u)!=0;result=value<<amount;}}
  else if(op==1){const unsigned n=amount?amount:32u;carry=((value>>(n-1u))&1u)!=0;result=n==32?0:value>>n;}
  else {const unsigned n=amount?amount:32u;carry=((value>>(n-1u))&1u)!=0;result=static_cast<std::uint32_t>(static_cast<std::int32_t>(value)>>n);}
  registers_[rd]=result;set_nz(result);cpsr_=(cpsr_&~C)|(carry?C:0);return;
 }
 // Format 2: add/subtract register or 3-bit immediate.
 if((ins&0xF800u)==0x1800u){
  const bool immediate=ins&(1u<<10),subtract=ins&(1u<<9);
  const unsigned rn=(ins>>6)&7u,rs=(ins>>3)&7u,rd=ins&7u;
  const auto a=registers_[rs],b=immediate?rn:registers_[rn];
  const std::uint32_t result=subtract?a-b:a+b;
  const bool carry=subtract?a>=b:(static_cast<std::uint64_t>(a)+b)>0xffffffffu;
  const bool overflow=subtract?(((a^b)&(a^result))>>31)!=0:((~(a^b)&(a^result))>>31)!=0;
  registers_[rd]=result;set_nz(result);cpsr_&=~(C|V);if(carry)cpsr_|=C;if(overflow)cpsr_|=V;return;
 }
 // MOV/CMP/ADD/SUB immediate.
 if((ins&0xE000u)==0x2000u){
  const unsigned op=(ins>>11)&3u,rd=(ins>>8)&7u;const auto imm=ins&0xffu;
  const auto a=registers_[rd];std::uint32_t result=imm;
  if(op==1)result=a-imm;else if(op==2)result=a+imm;else if(op==3)result=a-imm;
  if(op!=1)registers_[rd]=result;
  set_nz(result);
  if(op==1||op==2||op==3){const bool sub=(op==1||op==3);const bool carry=sub?a>=imm:(static_cast<std::uint64_t>(a)+imm)>0xffffffffu;const bool ov=sub?(((a^imm)&(a^result))>>31)!=0:((~(a^imm)&(a^result))>>31)!=0;cpsr_&=~(C|V);if(carry)cpsr_|=C;if(ov)cpsr_|=V;}
  return;
 }
 // ALU register operations.
 if((ins&0xFC00u)==0x4000u){
  const unsigned op=(ins>>6)&15u,rs=(ins>>3)&7u,rd=ins&7u;
  const auto a=registers_[rd],b=registers_[rs];std::uint32_t r=0;bool write=true,carry=(cpsr_&C)!=0;
  switch(op){
   case 0:r=a&b;break;case 1:r=a^b;break;
   case 2:{const unsigned n=b&0xffu;r=n>=32?0:a<<n;if(n)carry=((a>>(32u-n))&1u)!=0;break;}
   case 3:{const unsigned n=b&0xffu;r=n>=32?0:a>>n;if(n&&n<=32)carry=((a>>(n-1u))&1u)!=0;break;}
   case 4:{const unsigned n=b&0xffu;r=n>=32?static_cast<std::uint32_t>(static_cast<std::int32_t>(a)>>31):static_cast<std::uint32_t>(static_cast<std::int32_t>(a)>>n);if(n&&n<=32)carry=((a>>(n-1u))&1u)!=0;break;}
   case 5:{const std::uint64_t x=std::uint64_t(a)+b+((cpsr_&C)?1u:0u);r=static_cast<std::uint32_t>(x);cpsr_&=~C;if(x>>32)cpsr_|=C;break;}
   case 6:{const std::uint64_t sub=std::uint64_t(b)+((cpsr_&C)?0u:1u);r=a-static_cast<std::uint32_t>(sub);cpsr_&=~C;if(std::uint64_t(a)>=sub)cpsr_|=C;break;}
   case 8:r=a&b;write=false;break;
   case 9:r=0u-b;break;
   case 10:r=a-b;write=false;cpsr_&=~C;if(a>=b)cpsr_|=C;break;
   case 11:r=a+b;write=false;cpsr_&=~C;if(std::uint64_t(a)+b>0xffffffffu)cpsr_|=C;break;
   case 12:r=a|b;break;case 13:r=a*b;break;
   case 14:r=a&~b;break;case 15:r=~b;break;
   default:throw std::runtime_error("Unsupported Thumb ALU operation");
  }
  if(write)registers_[rd]=r;set_nz(r);
  if(op==2||op==3||op==4||op==7){cpsr_&=~C;if(carry)cpsr_|=C;}
  return;
 }
 // High-register ADD/CMP/MOV and BX.
 if((ins&0xFC00u)==0x4400u){
  const unsigned op=(ins>>8)&3u,rd=(ins&7u)|((ins>>4)&8u),rs=(ins>>3)&15u;
  const auto a=reg(rd),b=reg(rs);
  if(op==3){const auto target=b;if(target&1u){cpsr_|=(1u<<5);registers_[15]=target&~1u;}else{cpsr_&=~(1u<<5);registers_[15]=target&~3u;}return;}
  if(op==0){const auto result=a+b;if(rd==15)registers_[15]=result&~1u;else registers_[rd]=result;}
  else if(op==1){const auto result=a-b;set_nz(result);cpsr_&=~C;if(a>=b)cpsr_|=C;}
  else {if(rd==15)registers_[15]=b&~1u;else registers_[rd]=b;}
  return;
 }

 // Register-offset byte/word/halfword and signed-byte/halfword transfers.
 if((ins&0xF000u)==0x5000u){
  const unsigned op=(ins>>9)&7u,rm=(ins>>6)&7u,rn=(ins>>3)&7u,rd=ins&7u;
  const auto address=registers_[rn]+registers_[rm];
  const auto read16=[this](std::uint32_t a){return static_cast<std::uint16_t>(read_virtual8(a)|(static_cast<std::uint16_t>(read_virtual8(a+1u))<<8));};
  switch(op){
   case 0:write_virtual32(address,registers_[rd]);break;
   case 1:write_virtual8(address,static_cast<std::uint8_t>(registers_[rd]));write_virtual8(address+1u,static_cast<std::uint8_t>(registers_[rd]>>8));break;
   case 2:write_virtual8(address,static_cast<std::uint8_t>(registers_[rd]));break;
   case 3:registers_[rd]=static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(read_virtual8(address))));break;
   case 4:registers_[rd]=read_virtual32(address);break;
   case 5:registers_[rd]=read16(address);break;
   case 6:registers_[rd]=read_virtual8(address);break;
   case 7:registers_[rd]=static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(read16(address))));break;
  }
  return;
 }
 // Immediate halfword transfers.
 if((ins&0xF000u)==0x8000u){
  const bool load=ins&(1u<<11);const unsigned offset=((ins>>6)&31u)*2u,rn=(ins>>3)&7u,rd=ins&7u;
  const auto address=registers_[rn]+offset;
  if(load)registers_[rd]=static_cast<std::uint16_t>(read_virtual8(address)|(static_cast<std::uint16_t>(read_virtual8(address+1u))<<8));
  else{write_virtual8(address,static_cast<std::uint8_t>(registers_[rd]));write_virtual8(address+1u,static_cast<std::uint8_t>(registers_[rd]>>8));}
  return;
 }
 // Load address from PC or SP.
 if((ins&0xF000u)==0xA000u){
  const unsigned rd=(ins>>8)&7u;const auto base=(ins&(1u<<11))?registers_[13]:((registers_[15]+2u)&~3u);
  registers_[rd]=base+((ins&0xffu)<<2);return;
 }
 // Adjust stack pointer by a signed multiple of four.
 if((ins&0xFF00u)==0xB000u){
  const auto amount=(ins&0x7fu)<<2;
  if(ins&(1u<<7))registers_[13]-=amount;else registers_[13]+=amount;
  return;
 }
 // Thumb LDMIA/STMIA.
 if((ins&0xF000u)==0xC000u){
  const bool load=ins&(1u<<11);const unsigned rn=(ins>>8)&7u;const auto list=static_cast<std::uint8_t>(ins);
  if(list==0)throw std::runtime_error("Empty Thumb multiple-transfer register list is unsupported");
  auto address=registers_[rn];
  for(unsigned r=0;r<8;++r)if(list&(1u<<r)){
   if(load)registers_[r]=read_virtual32(address);else write_virtual32(address,registers_[r]);
   address+=4u;
  }
  registers_[rn]=address;return;
 }
 // Thumb long branch with link, two-halfword BL sequence.
 if((ins&0xF800u)==0xF000u){
  const auto high=static_cast<std::int32_t>(static_cast<std::int16_t>((ins&0x7ffu)<<5))>>5;
  registers_[14]=static_cast<std::uint32_t>(static_cast<std::int64_t>(registers_[15])+2+(static_cast<std::int64_t>(high)<<12));
  return;
 }
 if((ins&0xF800u)==0xF800u){
  const auto target=registers_[14]+((ins&0x7ffu)<<1);
  registers_[14]=registers_[15]|1u;
  registers_[15]=target&~1u;return;
 }
 // PC-relative literal load.
 if((ins&0xF800u)==0x4800u){
  const unsigned rd=(ins>>8)&7u;const auto address=((registers_[15]+2u)&~3u)+((ins&0xffu)<<2);
  registers_[rd]=read_virtual32(address);return;
 }
 // Immediate word/byte load and store.
 if((ins&0xE000u)==0x6000u){
  const bool byte=ins&(1u<<12),load=ins&(1u<<11);const unsigned imm=(ins>>6)&31u,rn=(ins>>3)&7u,rd=ins&7u;
  const auto address=registers_[rn]+(byte?imm:imm*4u);
  if(load)registers_[rd]=byte?read_virtual8(address):read_virtual32(address);
  else if(byte)write_virtual8(address,static_cast<std::uint8_t>(registers_[rd]));
  else write_virtual32(address,registers_[rd]);
  return;
 }
 // SP-relative word load/store.
 if((ins&0xF000u)==0x9000u){
  const bool load=ins&(1u<<11);const unsigned rd=(ins>>8)&7u;const auto address=registers_[13]+((ins&0xffu)<<2);
  if(load)registers_[rd]=read_virtual32(address);else write_virtual32(address,registers_[rd]);return;
 }
 // PUSH/POP register list.
 if((ins&0xF600u)==0xB400u){
  const bool pop=ins&(1u<<11),extra=ins&(1u<<8);const auto list=static_cast<std::uint8_t>(ins);
  if(!pop){
   unsigned count=0;for(unsigned i=0;i<8;++i)if(list&(1u<<i))++count;if(extra)++count;
   auto address=registers_[13]-4u*count;
   for(unsigned i=0;i<8;++i)if(list&(1u<<i)){write_virtual32(address,registers_[i]);address+=4;}
   if(extra)write_virtual32(address,registers_[14]);
   registers_[13]-=4u*count;
  }else{
   auto address=registers_[13];
   for(unsigned i=0;i<8;++i)if(list&(1u<<i)){registers_[i]=read_virtual32(address);address+=4;}
   if(extra){const auto value=read_virtual32(address);address+=4;registers_[15]=value&~1u;}
   registers_[13]=address;
  }
  return;
 }
 // Conditional branch.
 if((ins&0xF000u)==0xD000u){
  const unsigned cond=(ins>>8)&15u;
  if(cond==15u){enter_svc_exception(registers_[15]);return;}
  if(cond==14u)throw std::runtime_error("Reserved Thumb condition code");
  if(condition_passed(cond)){const auto off=static_cast<std::int32_t>(static_cast<std::int8_t>(ins&0xffu))*2;registers_[15]=static_cast<std::uint32_t>(static_cast<std::int64_t>(registers_[15])+2+off);}
  return;
 }
 // Unconditional branch.
 if((ins&0xF800u)==0xE000u){
  const auto off=static_cast<std::int32_t>(static_cast<std::int16_t>((ins&0x7ffu)<<5))>>4;
  registers_[15]=static_cast<std::uint32_t>(static_cast<std::int64_t>(registers_[15])+2+off);return;
 }
 throw std::runtime_error("Unsupported Thumb instruction encoding");
}
