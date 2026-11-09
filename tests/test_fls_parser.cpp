#include "firmware/fls_parser.hpp"
#include "memory/memory.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

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


        Memory staging_memory;
        staging_memory.map(0x10000000, 0x100, true, "test-staging-ram");
        const auto loaded = firmware::FlsParser::load_raw_segment(
            path, 6, 5, 0x10000010, staging_memory);
        require(loaded.bytes_loaded == 5, "Raw segment loader returned wrong length.");
        require(staging_memory.read8(0x10000010) == 0x41 &&
                staging_memory.read8(0x10000014) == 0x45,
                "Raw segment loader copied incorrect bytes.");

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
        }

        require(contents.find("CRC-32/ISO-HDLC") != std::string::npos,
                "Report is missing the CRC-32 section.");
        require(contents.find("PRINTABLE STRINGS") != std::string::npos,
                "Report is missing the printable-strings section.");
        require(contents.find("does not decode the Samsung FLS") != std::string::npos,
                "Report is missing the format limitation note.");

        // Build a small fixture matching the candidate 32-byte record pattern.
        std::vector<unsigned char> directory(0x120, 0);
        const auto add_record = [&directory](std::size_t offset, const std::string& name) {
            for (std::size_t i = 0; i < name.size(); ++i)
                directory[offset + 4 + i] = static_cast<unsigned char>(name[i]);
            directory[offset + 16] = 0x78;
            directory[offset + 17] = 0x56;
            directory[offset + 18] = 0x34;
            directory[offset + 19] = 0x12;
        };
        add_record(0xE0, "SLB");
        add_record(0x100, "CODE");
        {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            require(static_cast<bool>(out), "Could not recreate directory fixture.");
            out.write(reinterpret_cast<const char*>(directory.data()),
                      static_cast<std::streamsize>(directory.size()));
            require(static_cast<bool>(out), "Could not write directory fixture.");
        }
        const auto directory_report = firmware::FlsParser::inspect_directory(path);
        require(directory_report.find("Record @ 0x000000E0  name=SLB") != std::string::npos,
                "Directory scanner missed the SLB test record.");
        require(directory_report.find("Record @ 0x00000100  name=CODE") != std::string::npos,
                "Directory scanner missed the CODE test record.");
        require(directory_report.find("W4=0x12345678") != std::string::npos,
                "Directory scanner did not decode a little-endian word.");


        // Two files with a shared 4 KiB prefix and one differing byte.
        const auto compare_dir = dir / "c3350emu_compare_test";
        std::filesystem::create_directories(compare_dir);
        const auto compare_a = compare_dir / "a.fls";
        const auto compare_b = compare_dir / "b.fls";
        std::vector<unsigned char> compare_bytes(4096, 0x5A);
        {
            std::ofstream a(compare_a, std::ios::binary);
            std::ofstream b(compare_b, std::ios::binary);
            require(static_cast<bool>(a) && static_cast<bool>(b),
                    "Could not create header comparison fixtures.");
            a.write(reinterpret_cast<const char*>(compare_bytes.data()), compare_bytes.size());
            compare_bytes[123] = 0xA5;
            b.write(reinterpret_cast<const char*>(compare_bytes.data()), compare_bytes.size());
            require(static_cast<bool>(a) && static_cast<bool>(b),
                    "Could not write header comparison fixtures.");
        }
        const auto comparison =
            firmware::FlsParser::compare_headers_in_directory(compare_dir, 4096);
        require(comparison.find("first difference at 0x0000007B") != std::string::npos,
                "Header comparison did not report the first differing byte.");
        std::filesystem::remove_all(compare_dir);


        // Cross-sample correlation preserves raw words without inventing field meanings.
        const auto set_dir = dir / "c3350emu_set_test";
        std::filesystem::create_directories(set_dir);
        for (const auto& file_name : {"psi.fls", "slb.fls"}) {
            std::vector<unsigned char> sample(0x140, 0);
            const std::string record_name = "CODE";
            for (std::size_t i = 0; i < record_name.size(); ++i)
                sample[0xE4 + i] = static_cast<unsigned char>(record_name[i]);
            sample[0xF0] = 0x78; sample[0xF1] = 0x56;
            sample[0xF2] = 0x34; sample[0xF3] = 0x12;
            std::ofstream out(set_dir / file_name, std::ios::binary);
            require(static_cast<bool>(out), "Could not create firmware-set fixture.");
            out.write(reinterpret_cast<const char*>(sample.data()),
                      static_cast<std::streamsize>(sample.size()));
            require(static_cast<bool>(out), "Could not write firmware-set fixture.");
        }
        const auto set_report = firmware::FlsParser::analyze_firmware_set(set_dir);
        require(set_report.find("psi.fls") != std::string::npos &&
                set_report.find("slb.fls") != std::string::npos,
                "Firmware-set report omitted input files.");
        require(set_report.find("name=CODE") != std::string::npos &&
                set_report.find("psi.fls=0x12345678") != std::string::npos,
                "Firmware-set report did not correlate candidate record fields.");
        require(set_report.find("not assigned a load address") != std::string::npos,
                "Firmware-set report omitted its field-semantics warning.");
        std::filesystem::remove_all(set_dir);

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
