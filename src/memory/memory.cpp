#include "memory.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>
void Memory::map(std::uint32_t b,std::size_t s,bool w,const std::string& n){map_filled(b,s,w,n,0);}
void Memory::map_filled(std::uint32_t b,std::size_t s,bool w,const std::string& n,std::uint8_t v){
 if(!s)throw std::invalid_argument("Memory region cannot be empty.");const std::uint64_t e=std::uint64_t(b)+s;if(e>(std::uint64_t{1}<<32))throw std::invalid_argument("Memory region exceeds the 32-bit address space.");
 for(const auto& r:regions_){const auto re=std::uint64_t(r.base)+r.size;if(b<re&&r.base<e)throw std::invalid_argument("Memory regions must not overlap.");}
 regions_.push_back(Region{b,s,std::vector<std::uint8_t>(s,v),w,n,{},{}});
}
void Memory::map_device(std::uint32_t b,std::size_t s,const std::string& n,DeviceRead rd,DeviceWrite wr){
 if(!s||!rd||!wr)throw std::invalid_argument("A memory device needs a non-empty range and read/write handlers.");const std::uint64_t e=std::uint64_t(b)+s;if(e>(std::uint64_t{1}<<32))throw std::invalid_argument("Device region exceeds the 32-bit address space.");
 for(const auto& r:regions_){const auto re=std::uint64_t(r.base)+r.size;if(b<re&&r.base<e)throw std::invalid_argument("Memory regions must not overlap.");}
 regions_.push_back(Region{b,s,{},true,n,std::move(rd),std::move(wr)});
}
const Memory::Region& Memory::find(std::uint32_t a,std::size_t w)const{const std::uint64_t e=std::uint64_t(a)+w;for(const auto& r:regions_){const auto re=std::uint64_t(r.base)+r.size;if(a>=r.base&&e<=re)return r;}throw std::out_of_range("Unmapped memory access.");}
Memory::Region& Memory::find_writable(std::uint32_t a,std::size_t w){const auto& f=static_cast<const Memory&>(*this).find(a,w);for(auto& r:regions_)if(&r==&f){if(!r.writable)throw std::runtime_error("Attempted write to read-only memory.");return r;}throw std::out_of_range("Unmapped memory access.");}
std::uint8_t Memory::read8(std::uint32_t a)const{const auto& r=find(a,1);const auto o=a-r.base;return r.device_read?r.device_read(o):r.bytes[o];}
std::uint32_t Memory::read32le(std::uint32_t a)const{return std::uint32_t(read8(a))|(std::uint32_t(read8(a+1))<<8)|(std::uint32_t(read8(a+2))<<16)|(std::uint32_t(read8(a+3))<<24);}
void Memory::write8(std::uint32_t a,std::uint8_t v){auto& r=find_writable(a,1);const auto o=a-r.base;if(r.device_write)r.device_write(o,v);else r.bytes[o]=v;}
void Memory::write32le(std::uint32_t a,std::uint32_t v){for(unsigned i=0;i<4;++i)write8(a+i,static_cast<std::uint8_t>(v>>(i*8)));}
std::size_t Memory::region_count()const noexcept{return regions_.size();}
void Memory::write_block(std::uint32_t a,const std::vector<std::uint8_t>& d){if(d.empty())throw std::invalid_argument("Memory write block cannot be empty.");auto& r=find_writable(a,d.size());const auto o=static_cast<std::size_t>(a-r.base);if(r.device_write){for(std::size_t i=0;i<d.size();++i)r.device_write(static_cast<std::uint32_t>(o+i),d[i]);}else std::copy(d.begin(),d.end(),r.bytes.begin()+static_cast<std::ptrdiff_t>(o));}
