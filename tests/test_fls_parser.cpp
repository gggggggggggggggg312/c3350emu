#include "firmware/fls_parser.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    const auto dir = std::filesystem::temp_directory_path();
    const auto path = dir / "c3350emu_test_firmware.fls";
    const auto report = dir / "c3350emu_test_firmware.analysis.txt";

    try {
        {
            std::ofstream out(path, std::ios::binary);
            require(static_cast<bool>(out), "Could not create temporary firmware file.");
            const unsigned char bytes[] = {
                0x46, 0x4c, 0x53, 0x00, 0x11, 0x22,
                0x41, 0x42, 0x43, 0x44, 0x45
            };
            out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
            require(static_cast<bool>(out), "Could not write temporary firmware file.");
        }

        const auto info = firmware::FlsParser::inspect(path);
        require(info.file_size == 11, "Unexpected firmware file size.");
        require(info.header.size() == 11, "Unexpected inspected header size.");
        require(firmware::hex_bytes(info.header) ==
                    "46 4c 53 00 11 22 41 42 43 44 45",
                "Header bytes did not match the test input.");

        const auto made = firmware::FlsParser::analyze(path, report);
        require(made == report, "Analyzer returned an unexpected report path.");
        require(std::filesystem::exists(report), "Analyzer did not create its report.");

        std::string contents;
        {
            std::ifstream in(report, std::ios::binary);
            require(static_cast<bool>(in), "Could not open generated analysis report.");
            contents.assign(std::istreambuf_iterator<char>(in),
                            std::istreambuf_iterator<char>());
            require(!in.bad(), "Error while reading generated analysis report.");
        } // Close the report before deleting it (required on Windows).

        require(contents.find("CRC-32/ISO-HDLC") != std::string::npos,
                "Report is missing the CRC-32 section.");
        require(contents.find("PRINTABLE STRINGS") != std::string::npos,
                "Report is missing the printable-strings section.");
        require(contents.find("does not decode the Samsung FLS") != std::string::npos,
                "Report is missing the format limitation note.");

        std::filesystem::remove(report);
        std::filesystem::remove(path);

        bool rejected = false;
        try {
            (void)firmware::FlsParser::inspect(path);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        require(rejected, "Missing firmware file was not rejected.");

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fls_parser_tests failed: " << error.what() << '\n';
        std::error_code ignored;
        std::filesystem::remove(report, ignored);
        std::filesystem::remove(path, ignored);
        return 1;
    }
}
