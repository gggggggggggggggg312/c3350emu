#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include "cpu/cpu.hpp"
#include "devices/lcd.hpp"
#include "firmware/boot_plan.hpp"
#include "firmware/fls_parser.hpp"
#include "memory/memory.hpp"
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
HWND outputBox=nullptr;
std::string selectedPlan;
std::string screenStatus="NO BOOT PLAN";
std::vector<std::uint32_t> screenPixels(LcdController::PixelCount,0xff101010u);
constexpr int ID_OPEN=1001,ID_RUN=1002;
void append(const std::string& s){
 int n=GetWindowTextLengthA(outputBox);SendMessageA(outputBox,EM_SETSEL,n,n);
 SendMessageA(outputBox,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(s.c_str()));
}
void open_plan(HWND w){
 char path[MAX_PATH]{};OPENFILENAMEA ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=w;
 ofn.lpstrFilter="C3350EMU boot plans\0*.plan\0All files\0*.*\0";
 ofn.lpstrFile=path;ofn.nMaxFile=MAX_PATH;ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
 if(!GetOpenFileNameA(&ofn))return;
 selectedPlan=path;screenStatus="PLAN LOADED";
 append("Boot plan: "+selectedPlan+"\r\nPress Run to load its segments and execute the CPU.\r\n\r\n");
 InvalidateRect(w,nullptr,TRUE);
}
void run_plan(HWND w){
 try{
  if(selectedPlan.empty())throw std::runtime_error("Open a .plan file first.");
  const auto plan=firmware::parse_boot_plan(selectedPlan);
  screenStatus="LOADING FIRMWARE";InvalidateRect(w,nullptr,TRUE);UpdateWindow(w);
  Memory memory;
  memory.map(plan.ram_base,plan.ram_size,true,"boot-plan RAM");
  for(const auto& stub:plan.memory_stubs)
   memory.map_filled(stub.base,stub.size,true,"experimental memory stub",stub.initial_value);
  for(const auto& segment:plan.segments)
   firmware::FlsParser::load_raw_segment(segment.file,segment.offset,segment.length,segment.load_address,memory);
  LcdController lcd;
  if(plan.has_lcd)lcd.attach(memory,plan.lcd_base);
  firmware::apply_memory_initialization(plan,memory);
  firmware::apply_firmware_patches(plan,memory);
  CPU cpu(memory);cpu.reset(plan.entry);
  if(plan.has_stack_pointer)cpu.set_register(13,plan.stack_pointer);
  for(const auto& reg:plan.initial_registers)cpu.set_register(reg.index,reg.value);
  screenStatus="CPU RUNNING";InvalidateRect(w,nullptr,TRUE);UpdateWindow(w);
  std::string reason="instruction limit reached";
  std::uint64_t steps=0;
  while(steps<plan.instruction_limit){
   if(std::find(plan.breakpoints.begin(),plan.breakpoints.end(),cpu.getPC())!=plan.breakpoints.end()){reason="breakpoint reached";break;}
   try{cpu.step();}catch(const std::exception& e){reason=e.what();break;}
   ++steps;
  }
  if(plan.has_lcd)screenPixels=lcd.pixels_argb();
  screenStatus=plan.has_lcd?"CPU STOPPED / LCD MODEL":"CPU STOPPED / NO LCD";
  std::ostringstream out;
  out<<"BOOT-PLAN EXECUTION\r\nSegments loaded: "<<plan.segments.size()
     <<"\r\nPatches applied: "<<plan.patches.size()
     <<"\r\nLCD: "<<(plan.has_lcd?"mapped at 0x":"not configured");
  if(plan.has_lcd)out<<std::hex<<plan.lcd_base;
  out<<"\r\nInstructions: "<<std::dec<<cpu.instructions_executed()
     <<"\r\nStop reason: "<<reason<<"\r\nPC: 0x"<<std::hex<<cpu.getPC()
     <<" CPSR: 0x"<<cpu.get_cpsr()<<"\r\n";
  for(unsigned r=0;r<16;++r)out<<"R"<<std::dec<<r<<"=0x"<<std::hex<<cpu.get_register(r)<<((r%4==3)?"\r\n":"  ");
  out<<"\r\nPatch writes affect emulated RAM only; original firmware files are unchanged.\r\n"
     <<"LCD registers are an experimental C3350EMU interface, not yet a verified Samsung hardware map.\r\n"
     <<"TRACE directives are not displayed by this GUI yet.\r\n\r\n";
  append(out.str());InvalidateRect(w,nullptr,TRUE);
 }catch(const std::exception& e){screenStatus="BOOT FAILED";append(std::string("Boot failed: ")+e.what()+"\r\n\r\n");InvalidateRect(w,nullptr,TRUE);}
}
void paint_screen(HWND w){
 PAINTSTRUCT ps{};HDC dc=BeginPaint(w,&ps);
 RECT frame{20,95,300,375};HBRUSH outer=CreateSolidBrush(RGB(70,70,70));FillRect(dc,&frame,outer);DeleteObject(outer);
 RECT lcd{38,113,282,357};HBRUSH black=CreateSolidBrush(RGB(0,0,0));FillRect(dc,&lcd,black);DeleteObject(black);
 BITMAPINFO bmi{};bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bmi.bmiHeader.biWidth=128;bmi.bmiHeader.biHeight=-128;
 bmi.bmiHeader.biPlanes=1;bmi.bmiHeader.biBitCount=32;bmi.bmiHeader.biCompression=BI_RGB;
 StretchDIBits(dc,42,117,236,236,0,0,128,128,screenPixels.data(),&bmi,DIB_RGB_COLORS,SRCCOPY);
 SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));
 RECT label{20,385,300,430};DrawTextA(dc,screenStatus.c_str(),-1,&label,DT_CENTER|DT_WORDBREAK);
 EndPaint(w,&ps);
}
LRESULT CALLBACK window_proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 if(m==WM_CREATE){
  CreateWindowA("BUTTON","Open boot plan...",WS_CHILD|WS_VISIBLE,20,18,160,32,w,(HMENU)ID_OPEN,nullptr,nullptr);
  CreateWindowA("BUTTON","Run firmware",WS_CHILD|WS_VISIBLE,192,18,150,32,w,(HMENU)ID_RUN,nullptr,nullptr);
  CreateWindowA("STATIC","128 x 128 RGB565 experimental LCD",WS_CHILD|WS_VISIBLE,20,70,300,20,w,nullptr,nullptr,nullptr);
  outputBox=CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","C3350EMU boot runtime\r\nOpen a .plan file. Configure LCD with an LCD base directive.\r\n",WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,325,70,650,520,w,nullptr,nullptr,nullptr);
  return 0;
 }
 if(m==WM_COMMAND){if(LOWORD(wp)==ID_OPEN)open_plan(w);else if(LOWORD(wp)==ID_RUN)run_plan(w);return 0;}
 if(m==WM_SIZE&&outputBox){MoveWindow(outputBox,325,70,LOWORD(lp)-345,HIWORD(lp)-90,TRUE);InvalidateRect(w,nullptr,TRUE);return 0;}
 if(m==WM_PAINT){paint_screen(w);return 0;}
 if(m==WM_DESTROY){PostQuitMessage(0);return 0;}
 return DefWindowProcA(w,m,wp,lp);
}
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR,int show){
 const char* cls="C3350EMU_GUI_WINDOW";WNDCLASSA wc{};wc.lpfnWndProc=window_proc;wc.hInstance=instance;
 wc.lpszClassName=cls;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
 RegisterClassA(&wc);HWND w=CreateWindowA(cls,"C3350EMU - Firmware Emulator",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1020,650,nullptr,nullptr,instance,nullptr);
 if(!w)return 1;ShowWindow(w,show);UpdateWindow(w);MSG msg{};
 while(GetMessageA(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageA(&msg);}return static_cast<int>(msg.wParam);
}
#else
#include <iostream>
int main(){std::cout<<"C3350EMU GUI is currently available on Windows. Use the C3350EMU CLI on this platform.\n";}
#endif
