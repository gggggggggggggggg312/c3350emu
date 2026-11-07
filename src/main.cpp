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
        << "  " << program << " --inspect-firmware <file.fls>\n\n"
        << "Platform information is a research profile, not a complete hardware spec.\n"
        << "The firmware inspector does not yet decode FLS records or boot firmware.\n";
}

static void print_platform_info() {
    std::cout << "GT-C3350 provisional hardware research profile\n"
              << "==============================================\n";
    for (const auto& fact : platform::gt_c3350_research_profile()) {
        std::cout << fact.label << ": " << fact.value
                  << " [" << fact.confidence << "]\n";
    }
    std::cout << "\nNo physical memory addresses or peripheral register offsets "
                 "are assumed.\n";
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        print_help(argv[0]);
        return 0;
    }

    if (argc == 2 && std::string(argv[1]) == "--platform-info") {
        print_platform_info();
        return 0;
    }

    if (argc == 3 && std::string(argv[1]) == "--inspect-firmware") {
        try {
            const auto info = firmware::FlsParser::inspect(argv[2]);
            std::cout << "C3350EMU firmware inspection\n"
                      << "File: " << info.path.string() << "\n"
                      << "Size: " << info.file_size << " bytes\n"
                      << "Classification: " << info.classification << "\n"
                      << "Header: " << firmware::hex_bytes(info.header) << "\n"
                      << "Note: " << info.note << "\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Firmware inspection failed: " << error.what() << "\n";
            return 1;
        }
    }

    if (argc != 1) {
        print_help(argv[0]);
        return 2;
    }

    std::cout << "C3350EMU research emulator\n"
              << "ARM-state interpreter: initial instruction subset implemented\n"
              << "Firmware boot: not implemented\n"
              << "Use --platform-info for current hardware research.\n"
              << "Use --inspect-firmware <file.fls> to inspect a firmware file.\n";
    return 0;
}
