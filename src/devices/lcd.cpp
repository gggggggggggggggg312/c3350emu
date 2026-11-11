#include "lcd.hpp"
LcdController::LcdController():framebuffer_(PixelCount,0){}
void LcdController::attach(Memory& m,std::uint32_t b){base_=b;m.map_device(b,RegisterBytes,"experimental RGB666 LCD controller",[this](std::uint32_t o){return read_byte(o);},[this](std::uint32_t o,std::uint8_t v){write_byte(o,v);});}
std::uint8_t LcdController::read_byte(std::uint32_t o)const{
 const auto reg=o&~3u;std::uint32_t v=0;
 if(reg==0x00)v=control_;else if(reg==0x04)v=static_cast<std::uint32_t>(Width)|(static_cast<std::uint32_t>(Height)<<16);
 else if(reg==0x08)v=pixel_index_;else if(reg==0x0c)v=pixel_index_<PixelCount?framebuffer_[pixel_index_]:0;else if(reg==0x10)v=1;
 return static_cast<std::uint8_t>(v>>((o&3u)*8u));
}
void LcdController::write_byte(std::uint32_t o,std::uint8_t v){
 const auto reg=o&~3u,shift=(o&3u)*8u;
 if(reg==0x00)control_=(control_&~(0xffu<<shift))|(std::uint32_t(v)<<shift);
 else if(reg==0x08){pixel_index_=(pixel_index_&~(0xffu<<shift))|(std::uint32_t(v)<<shift);if(pixel_index_>=PixelCount)pixel_index_=0;}
 else if(reg==0x0c){pixel_latch_=(pixel_latch_&~(0xffu<<shift))|(std::uint32_t(v)<<shift);if(shift==24u){framebuffer_[pixel_index_]=pixel_latch_&0x3ffffu;pixel_index_=(pixel_index_+1u)%PixelCount;}}
}
std::vector<std::uint32_t> LcdController::pixels_argb()const{
 std::vector<std::uint32_t> out;out.reserve(PixelCount);
 for(auto p:framebuffer_){const auto r=(p>>12)&63u,g=(p>>6)&63u,b=p&63u;out.push_back(0xff000000u|((r*255u/63u)<<16)|((g*255u/63u)<<8)|(b*255u/63u));}
 return out;
}
