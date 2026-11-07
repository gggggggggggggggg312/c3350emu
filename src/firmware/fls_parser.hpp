#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace firmware {

struct FlsInfo {
    std::filesystem::path path;
    std::uintmax_t file_size = 0;
    std::vector<std::uint8_t> header;
    std::string classification;
    std::string note;
};

class FlsParser {
public:
    static FlsInfo inspect(const std::filesystem::path& path,
                           std::size_t header_size = 64,
                           std::uintmax_t max_file_size = 256ULL * 1024ULL * 1024ULL);

    // Produces a dependency-free forensic report. Findings are candidates,
    // not a decoded Samsung FLS specification.
    static std::filesystem::path analyze(
        const std::filesystem::path& path,
        const std::filesystem::path& report_path = {},
        std::uintmax_t max_file_size = 256ULL * 1024ULL * 1024ULL);
};

std::string hex_bytes(const std::vector<std::uint8_t>& bytes);

} // namespace firmware
