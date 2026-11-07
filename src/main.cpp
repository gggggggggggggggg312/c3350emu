#include "cpu/cpu.hpp"
#include "firmware/fls_parser.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

static void print_help(const char* program) {
    std::cout
        << "C3350EMU - Samsung GT-C3350 research emulator\\n\\n"
        << "Usage:\\n"
        << "  " << program << " --help\\n"
        << "  " << program << " --inspect-firmware <file.fls>\\n\\n"
        << "The firmware inspector currently reports file metadata and raw header "
           "bytes. It does not yet decode FLS segments or boot firmware.\\n";
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        print_help(argv[0]);
        return 0;
    }

    if (argc == 3 && std::string(argv[1]) == "--inspect-firmware") {
        try {
            const auto info = firmware::FlsParser::inspect(argv[2]);
            std::cout << "C3350EMU firmware inspection\\n"
                      << "File: " << info.path.string() << "\\n"
                      << "Size: " << info.file_size << " bytes\\n"
                      << "Classification: " << info.classification << "\\n"
                      << "Header: " << firmware::hex_bytes(info.header) << "\\n"
                      << "Note: " << info.note << "\\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Firmware inspection failed: " << error.what() << "\\n";
            return 1;
        }
    }

    if (argc != 1) {
        print_help(argv[0]);
        return 2;
    }

    CPU cpu;
    cpu.reset();
    std::cout << "C3350EMU research scaffold\\n"
              << "CPU execution: not implemented\\n"
              << "Firmware boot: not implemented\\n"
              << "Use --inspect-firmware <file.fls> to inspect a firmware file.\\n";
    return 0;
}
