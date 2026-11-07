#include "firmware/fls_parser.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
int main() {
    const auto dir=std::filesystem::temp_directory_path();
    const auto path=dir/"c3350emu_test_firmware.fls";
    const auto report=dir/"c3350emu_test_firmware.analysis.txt";
    { std::ofstream o(path,std::ios::binary); const unsigned char b[]={0x46,0x4c,0x53,0,0x11,0x22,0x41,0x42,0x43,0x44,0x45}; o.write(reinterpret_cast<const char*>(b),sizeof(b)); }
    const auto info=firmware::FlsParser::inspect(path);
    assert(info.file_size==11 && info.header.size()==11);
    assert(firmware::hex_bytes(info.header)=="46 4c 53 00 11 22 41 42 43 44 45");
    const auto made=firmware::FlsParser::analyze(path,report);
    assert(made==report && std::filesystem::exists(report));
    std::ifstream in(report); std::string contents((std::istreambuf_iterator<char>(in)),{});
    assert(contents.find("CRC-32/ISO-HDLC")!=std::string::npos);
    assert(contents.find("PRINTABLE STRINGS")!=std::string::npos);
    assert(contents.find("does not decode the Samsung FLS")!=std::string::npos);
    std::filesystem::remove(path); std::filesystem::remove(report);
    bool rejected=false; try{(void)firmware::FlsParser::inspect(path);}catch(const std::runtime_error&){rejected=true;}
    assert(rejected);
    return 0;
}
