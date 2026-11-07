#include "firmware/fls_parser.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>

int main() {
    const auto path = std::filesystem::temp_directory_path() /
                      "c3350emu_test_firmware.fls";
    {
        std::ofstream output(path, std::ios::binary);
        const unsigned char sample[] = {0x46, 0x4c, 0x53, 0x00, 0x11, 0x22};
        output.write(reinterpret_cast<const char*>(sample), sizeof(sample));
    }

    const auto info = firmware::FlsParser::inspect(path);
    assert(info.file_size == 6);
    assert(info.header.size() == 6);
    assert(info.classification == "Unclassified FLS-like binary");
    assert(firmware::hex_bytes(info.header) == "46 4c 53 00 11 22");

    std::filesystem::remove(path);

    bool missing_rejected = false;
    try { (void)firmware::FlsParser::inspect(path); }
    catch (const std::runtime_error&) { missing_rejected = true; }
    assert(missing_rejected);
    return 0;
}
