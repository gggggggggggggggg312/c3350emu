#pragma once
#include "memory/memory.hpp"
#include <cstdint>
#include <vector>
// Generic development LCD interface; NOT a verified Samsung GT-C3350 register map.
class LcdController {
public:
 static constexpr std::uint32_t RegisterBytes=0x100;
 static constexpr std::size_t Width=128,Height=128,PixelCount=Width*Height;
 LcdController();
 void attach(Memory&,std::uint32_t base);
 std::uint32_t base()const noexcept{return base_;}
 std::vector<std::uint32_t> pixels_argb()const;
private:
 std::uint8_t read_byte(std::uint32_t)const;
 void write_byte(std::uint32_t,std::uint8_t);
 std::uint32_t base_=0,pixel_index_=0,control_=0;
 std::uint16_t pixel_latch_=0;
 std::vector<std::uint16_t> framebuffer_;
};
