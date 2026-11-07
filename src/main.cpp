#include "cpu/cpu.hpp"
#include "firmware/fls_parser.hpp"
#include "platform/platform.hpp"
#include "memory/memory.hpp"

#include <exception>
#include <iostream>
#include <limits>
#include <string>

static void print_help(const char* program) {
    std::cout
        << "C3350EMU - Samsung GT-C3350 research emulator\n\n"
        << "Usage:\n"
        << "  " << program << " --help\n"
        << "  " << program << " --platform-info\n"
        << "  " << program << " --inspect-firmware <file.fls>\n"
        << "  " << program << " --analyze-firmware <file.fls> [report.txt]\n"
        << "  " << program << " --inspect-fls-directory <file.fls>\n"
        << "  " << program << " --compare-firmware-set <directory> [bytes]\\n"
        << "  " << program << " --load-raw-segment <file> <offset> <length> <ram-base> <ram-size> <load-address>\\n\\n"
        << "The native analyzer creates a text report without Python or extra dependencies.\n"
        << "The directory scanner reports candidate 32-byte records; its layout is not yet validated.\n"
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

    if (argc == 8 && std::string(argv[1]) == "--load-raw-segment") {
        try {
            const auto parse = [](const char* value, const char* label) -> std::uint64_t {
                const std::string text(value);
                std::size_t used = 0;
                const auto parsed = std::stoull(text, &used, 0);
                if (used != text.size())
                    throw std::runtime_error(std::string("Invalid ") + label + ".");
                return parsed;
            };
            const auto offset = parse(argv[3], "file offset");
            const auto length64 = parse(argv[4], "segment length");
            const auto ram_base64 = parse(argv[5], "RAM base");
            const auto ram_size64 = parse(argv[6], "RAM size");
            const auto load_address64 = parse(argv[7], "load address");
            if (length64 > std::numeric_limits<std::size_t>::max() ||
                ram_size64 == 0 || ram_size64 > std::numeric_limits<std::size_t>::max() ||
                ram_base64 > 0xffffffffu || load_address64 > 0xffffffffu)
                throw std::runtime_error("One or more address/size values are out of range.");

            Memory memory;
            memory.map(static_cast<std::uint32_t>(ram_base64),
                       static_cast<std::size_t>(ram_size64), true,
                       "user-configured staging RAM");
            const auto result = firmware::FlsParser::load_raw_segment(
                argv[2], offset, static_cast<std::size_t>(length64),
                static_cast<std::uint32_t>(load_address64), memory);
            std::cout << "Raw firmware segment staged successfully\n"
                      << "Source: " << result.source.string() << "\n"
                      << "File offset: 0x" << std::hex << result.file_offset << "\n"
                      << "Load address: 0x" << result.load_address << "\n"
                      << std::dec << "Bytes loaded: " << result.bytes_loaded << "\n"
                      << "Calculated CRC-32: 0x" << std::hex << result.crc32 << "\n"
                      << "RAM: user-configured only; not a verified GT-C3350 physical map.\n"
                      << "FLS segment boundaries and true load address remain unverified.\n";
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Firmware staging failed: " << e.what() << "\n";
            return 1;
        }
    }
\n    if ((argc == 3 || argc == 4) && std::string(argv[1]) == "--compare-firmware-set") {
        try {
            std::size_t bytes = 4096;
            if (argc == 4) {
                const std::string value(argv[3]);
                std::size_t parsed = 0;
                bytes = static_cast<std::size_t>(std::stoull(value, &parsed, 0));
                if (parsed != value.size()) throw std::runtime_error("Invalid comparison length.");
            }
            std::cout << firmware::FlsParser::compare_headers_in_directory(argv[2], bytes);
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Header comparison failed: " << e.what() << "\n";
            return 1;
        }
    }
    if (argc == 3 && std::string(argv[1]) == "--inspect-fls-directory") {
        try {
            std::cout << firmware::FlsParser::inspect_directory(argv[2]);
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Directory inspection failed: " << e.what() << "\n";
            return 1;
        }
    }
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
