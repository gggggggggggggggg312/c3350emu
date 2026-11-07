#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

class Memory;

namespace firmware {

struct LoadResult {
    std::filesystem::path source;
    std::uint64_t file_offset = 0;
    std::uint32_t load_address = 0;
    std::size_t bytes_loaded = 0;
    std::uint32_t crc32 = 0;
};

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

    // Prints candidate 32-byte directory records based on the observed sample pattern.
    // This is a heuristic, not a validated Samsung FLS specification.
    static std::string inspect_directory(
        const std::filesystem::path& path,
        std::uintmax_t max_file_size = 256ULL * 1024ULL * 1024ULL);

    // Compares the first N bytes of every .fls file in a directory.
    // Reports the shared prefix and first differing byte for each pair.
    static std::string compare_headers_in_directory(
        const std::filesystem::path& directory,
        std::size_t bytes_to_compare = 4096,
        std::uintmax_t max_file_size = 256ULL * 1024ULL * 1024ULL);

    // Loads an explicitly selected byte range into already-mapped writable RAM.
    // This is a raw segment staging operation, not an automatic FLS decoder.
    static LoadResult load_raw_segment(
        const std::filesystem::path& path,
        std::uint64_t file_offset,
        std::size_t length,
        std::uint32_t load_address,
        Memory& memory,
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
