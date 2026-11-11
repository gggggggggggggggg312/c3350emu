#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
class Memory {
public:
 using DeviceRead=std::function<std::uint8_t(std::uint32_t)>;
 using DeviceWrite=std::function<void(std::uint32_t,std::uint8_t)>;
 struct Region{std::uint32_t base;std::size_t size;std::vector<std::uint8_t> bytes;bool writable;std::string name;DeviceRead device_read;DeviceWrite device_write;};
 void map(std::uint32_t,std::size_t,bool,const std::string&);
 void map_filled(std::uint32_t,std::size_t,bool,const std::string&,std::uint8_t);
 void map_device(std::uint32_t,std::size_t,const std::string&,DeviceRead,DeviceWrite);
 std::uint8_t read8(std::uint32_t)const;std::uint32_t read32le(std::uint32_t)const;
 void write8(std::uint32_t,std::uint8_t);void write32le(std::uint32_t,std::uint32_t);
 void write_block(std::uint32_t,const std::vector<std::uint8_t>&);std::size_t region_count()const noexcept;
private:
 const Region& find(std::uint32_t,std::size_t)const;Region& find_writable(std::uint32_t,std::size_t);
 std::vector<Region> regions_;
};
