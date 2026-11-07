#include "fls_parser.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace firmware {

FlsInfo FlsParser::inspect(const std::filesystem::path& path,
                           std::size_t header_size,
                           std::uintmax_t max_file_size) {
    if (!std::filesystem::exists(path))
        throw std::runtime_error("Firmware file does not exist: " + path.string());
    if (!std::filesystem::is_regular_file(path))
        throw std::runtime_error("Firmware path is not a regular file: " + path.string());

    const auto size = std::filesystem::file_size(path);
    if (size == 0)
        throw std::runtime_error("Firmware file is empty.");
    if (size > max_file_size)
        throw std::runtime_error("Firmware file exceeds the configured safety limit.");

    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Unable to open firmware file: " + path.string());

    const auto count = static_cast<std::size_t>(
        size < header_size ? size : header_size);
    std::vector<std::uint8_t> header(count);
    input.read(reinterpret_cast<char*>(header.data()),
               static_cast<std::streamsize>(header.size()));
    if (!input && !input.eof())
        throw std::runtime_error("Failed while reading firmware header.");

    FlsInfo result;
    result.path = path;
    result.file_size = size;
    result.header = std::move(header);
    result.classification = "Unclassified FLS-like binary";
    result.note =
        "Header captured only. Container fields, checksums, segment boundaries, "
        "load addresses, and entry points have not yet been validated.";
    return result;
}

std::string hex_bytes(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i) output << ' ';
        output << std::setw(2) << static_cast<unsigned int>(bytes[i]);
    }
    return output.str();
}

} // namespace firmware
