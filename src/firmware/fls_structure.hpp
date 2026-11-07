#pragma once
#include <filesystem>
#include <string>

namespace firmware {
// Decodes the observed 0xE0 header / 0x20 directory layout.
// Numeric record-tail fields are deliberately left uninterpreted until validated.
std::string decode_fls_structure(const std::filesystem::path& path);
} // namespace firmware
