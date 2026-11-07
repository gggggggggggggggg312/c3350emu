#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include "cpu/cpu.hpp"
#include "memory/memory.hpp"
#include "platform/battery.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
HWND outputBox=nullptr, ramBaseBox=nullptr, ramSizeBox=nullptr, entryBox=nullptr;
std::string selectedFile;
platform::Battery battery(1000u, 100u);
constexpr int ID_OPEN=1001, ID_RUN=1002, ID_BASE=1003, ID_SIZE=1004, ID_ENTRY=1005, ID_CHARGE=1006, ID_TICK=1007;
void append(const std::string& text){
 int n=GetWindowTextLengthA(outputBox);SendMessageA(outputBox,EM_SETSEL,n,n);
 SendMessageA(outputBox,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(text.c_str()));
}
std::uint64_t number(HWND box,const char* label){
 char value[64]{};GetWindowTextA(box,value,sizeof(value));std::size_t used=0;
 auto n=std::stoull(value,&used,0);if(used==0||value[used]!='\0')throw std::runtime_error(std::string("Invalid ")+label);
 return n;
}
void show_battery(){
 std::ostringstream out;
 out<<"Battery: "<<unsigned(battery.percentage())<<"% | "<<battery.voltage_mv()<<" mV | "
    <<battery.remaining_mah()<<"/"<<battery.capacity_mah()<<" mAh | "
    <<(battery.charging()?"charging":(battery.full()?"full":"not charging"))<<"\\r\\n";
 append(out.str());
}
void open_file(HWND window){
 char path[MAX_PATH]{};OPENFILENAMEA ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=window;
 ofn.lpstrFilter="Firmware and binary files\0*.fls;*.bin;*.img;*.*\0All files\0*.*\0";
 ofn.lpstrFile=path;ofn.nMaxFile=MAX_PATH;ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
 if(!GetOpenFileNameA(&ofn))return;
 selectedFile=path;
 std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in){append("Could not open file.\r\n");return;}
 const auto end=in.tellg();if(end<0){append("Could not read file size.\r\n");return;}
 const auto size=static_cast<std::uint64_t>(end);std::vector<std::uint8_t> head(static_cast<std::size_t>(std::min<std::uint64_t>(size,64)));
 in.seekg(0);if(!head.empty())in.read(reinterpret_cast<char*>(head.data()),static_cast<std::streamsize>(head.size()));
 std::ostringstream out;out<<"Selected: "<<selectedFile<<"\r\nSize: "<<size<<" bytes\r\nHeader:";
 for(std::size_t i=0;i<head.size();++i){if(i%16==0)out<<"\r\n";out<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(head[i])<<" ";}
 out<<"\r\nNote: header inspection only. FLS is not automatically decoded.\r\n\r\n";append(out.str());
}
void run_image(){
 try{
  if(selectedFile.empty())throw std::runtime_error("Choose a file first.");
  const auto base=number(ramBaseBox,"RAM base"),size=number(ramSizeBox,"RAM size"),entry=number(entryBox,"entry point");
  if(base>0xffffffffu||!size||size>(std::uint64_t{1}<<32)-base||entry>0xffffffffu)
   throw std::runtime_error("RAM or entry values are outside the 32-bit address space.");
  std::ifstream in(selectedFile,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("Cannot open selected file.");
  const auto end=in.tellg();if(end<=0||static_cast<std::uint64_t>(end)>size)throw std::runtime_error("File is empty or larger than configured RAM.");
  std::vector<std::uint8_t> image(static_cast<std::size_t>(end));in.seekg(0);
  if(!in.read(reinterpret_cast<char*>(image.data()),static_cast<std::streamsize>(image.size())))throw std::runtime_error("File read failed.");
  if(entry<base||entry+4>base+image.size()||(entry&3u))throw std::runtime_error("ARM entry must be aligned and inside the loaded image.");
  Memory memory;memory.map(static_cast<std::uint32_t>(base),static_cast<std::size_t>(size),true,"GUI-configured staging RAM");
  memory.write_block(static_cast<std::uint32_t>(base),image);CPU cpu(memory);cpu.reset(static_cast<std::uint32_t>(entry));
  std::string reason="instruction limit reached";
  for(std::uint64_t i=0;i<100000;++i){try{cpu.step();}catch(const std::exception& e){reason=e.what();break;}}
  std::ostringstream out;out<<"\r\nRAW ARM EXECUTION\r\nInstructions: "<<cpu.instructions_executed()
   <<"\r\nStop reason: "<<reason<<"\r\nPC: 0x"<<std::hex<<cpu.getPC()<<" CPSR: 0x"<<cpu.get_cpsr()<<"\r\n";
  for(unsigned r=0;r<16;++r)out<<"R"<<std::dec<<r<<"=0x"<<std::hex<<cpu.get_register(r)<<((r%4==3)?"\r\n":"  ");
  out<<"This is a raw-image experiment, not an automatic FLS decode or verified phone boot.\r\n\r\n";append(out.str());
 }catch(const std::exception& e){append(std::string("Run failed: ")+e.what()+"\r\n\r\n");}
}
LRESULT CALLBACK window_proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 if(m==WM_CREATE){
  CreateWindowA("BUTTON","Open firmware / binary...",WS_CHILD|WS_VISIBLE,12,12,190,30,w,(HMENU)ID_OPEN,nullptr,nullptr);
  CreateWindowA("STATIC","RAM base",WS_CHILD|WS_VISIBLE,218,17,65,20,w,nullptr,nullptr,nullptr);
  ramBaseBox=CreateWindowA("EDIT","0x01000000",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,282,13,100,25,w,(HMENU)ID_BASE,nullptr,nullptr);
  CreateWindowA("STATIC","RAM size",WS_CHILD|WS_VISIBLE,394,17,60,20,w,nullptr,nullptr,nullptr);
  ramSizeBox=CreateWindowA("EDIT","0x01000000",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,455,13,100,25,w,(HMENU)ID_SIZE,nullptr,nullptr);
  CreateWindowA("STATIC","Entry",WS_CHILD|WS_VISIBLE,567,17,40,20,w,nullptr,nullptr,nullptr);
  entryBox=CreateWindowA("EDIT","0x01000000",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,610,13,100,25,w,(HMENU)ID_ENTRY,nullptr,nullptr);
  CreateWindowA("BUTTON","Run raw ARM (100k instructions)",WS_CHILD|WS_VISIBLE,720,12,235,30,w,(HMENU)ID_RUN,nullptr,nullptr);
  CreateWindowA("BUTTON","Toggle charging",WS_CHILD|WS_VISIBLE,12,48,130,28,w,(HMENU)ID_CHARGE,nullptr,nullptr);
  CreateWindowA("BUTTON","Advance 1 minute",WS_CHILD|WS_VISIBLE,150,48,145,28,w,(HMENU)ID_TICK,nullptr,nullptr);
  outputBox=CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","C3350EMU GUI research console\r\nSelect a raw executable image or an FLS file to inspect its header.\r\n",WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,12,82,943,563,w,nullptr,nullptr,nullptr);
  return 0;
 }
 if(m==WM_COMMAND){if(LOWORD(wp)==ID_OPEN)open_file(w);else if(LOWORD(wp)==ID_RUN)run_image();else if(LOWORD(wp)==ID_CHARGE){battery.set_charging(!battery.charging());show_battery();}else if(LOWORD(wp)==ID_TICK){battery.advance_seconds(60);show_battery();}return 0;}
 if(m==WM_SIZE&&outputBox){MoveWindow(outputBox,12,82,LOWORD(lp)-24,HIWORD(lp)-94,TRUE);return 0;}
 if(m==WM_DESTROY){PostQuitMessage(0);return 0;}
 return DefWindowProcA(w,m,wp,lp);
}
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR,int show){
 const char* cls="C3350EMU_GUI_WINDOW";WNDCLASSA wc{};wc.lpfnWndProc=window_proc;wc.hInstance=instance;
 wc.lpszClassName=cls;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
 RegisterClassA(&wc);HWND w=CreateWindowA(cls,"C3350EMU - Samsung GT-C3350 Research GUI",WS_OVERLAPPEDWINDOW,
 CW_USEDEFAULT,CW_USEDEFAULT,980,700,nullptr,nullptr,instance,nullptr);
 if(!w)return 1;ShowWindow(w,show);UpdateWindow(w);MSG msg{};
 while(GetMessageA(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageA(&msg);}return static_cast<int>(msg.wParam);
}
#else
#include <iostream>
int main(){std::cout<<"C3350EMU GUI is currently available on Windows. Use the C3350EMU CLI on this platform.\n";}
#endif
