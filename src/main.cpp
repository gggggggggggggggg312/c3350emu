#include "cpu/cpu.hpp"
#include "firmware/fls_parser.hpp"
#include "platform/platform.hpp"

#include <exception>
#include <iostream>
#include <string>

static void print_help(const char* program) {
    std::cout
        << "C3350EMU - Samsung GT-C3350 research emulator\n\n"
        << "Usage:\n"
        << "  " << program << " --help\n"
        << "  " << program << " --platform-info\n"
        << "  " << program << " --inspect-firmware <file.fls>\n"
        << "  " << program << " --analyze-firmware <file.fls> [report.txt]\n\n"
        << "The native analyzer creates a text report without Python or extra dependencies.\n"
        << "Its pattern matches are research leads, not verified FLS fields or addresses.\n";
}
static void print_platform_info() {
    std::cout << "GT-C3350 provisional hardware research profile\n"
              << "==============================================\n";
    for (const auto& fact : platform::gt_c3350_research_profile())
        std::cout << fact.label << ": " << fact.value << " [" << fact.confidence << "]\n";
    std::cout << "\nNo physical memory addresses or peripheral register offsets are assumed.\n";
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") { print_help(argv[0]); return 0; }
    if (argc == 2 && std::string(argv[1]) == "--platform-info") { print_platform_info(); return 0; }
    if ((argc == 3 || argc == 4) && std::string(argv[1]) == "--analyze-firmware") {
        try {
            const auto report=firmware::FlsParser::analyze(argv[2], argc==4 ? argv[3] : "");
            std::cout << "Analysis complete. Report: " << report.string() << "\n"
                      << "This is heuristic analysis, not a confirmed FLS decode.\n";
            return 0;
        } catch (const std::exception& e) { std::cerr << "Analysis failed: " << e.what() << "\n"; return 1; }
    }
    if (argc == 3 && std::string(argv[1]) == "--inspect-firmware") {
        try {
            const auto info=firmware::FlsParser::inspect(argv[2]);
            std::cout<<"File: "<<info.path.string()<<"\nSize: "<<info.file_size
                     <<" bytes\nClassification: "<<info.classification
                     <<"\nHeader: "<<firmware::hex_bytes(info.header)
                     <<"\nNote: "<<info.note<<"\n";
            return 0;
        } catch(const std::exception& e) { std::cerr<<"Firmware inspection failed: "<<e.what()<<"\n"; return 1; }
    }
    if(argc!=1) { print_help(argv[0]); return 2; }
    std::cout<<"C3350EMU research emulator\nARM-state interpreter: initial instruction subset implemented\n"
             <<"Firmware boot: not implemented\nUse --platform-info or --analyze-firmware <file.fls>.\n";
    return 0;
}
