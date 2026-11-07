#include "fls_structure.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace firmware {
namespace {
constexpr std::size_t kHeaderSize = 0xE0;
constexpr std::size_t kRecordSize = 0x20;
constexpr std::size_t kNameOffset = 4;
constexpr std::size_t kNameSize = 16;
constexpr std::size_t kMaxRecords = 4096;

std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t p) {
    return static_cast<std::uint32_t>(b[p]) |
        (static_cast<std::uint32_t>(b[p + 1]) << 8) |
        (static_cast<std::uint32_t>(b[p + 2]) << 16) |
        (static_cast<std::uint32_t>(b[p + 3]) << 24);
}
std::string hx(std::uint64_t v, int width = 8) {
    std::ostringstream s;
    s << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(width) << v;
    return s.str();
}
bool valid_name(const std::vector<std::uint8_t>& b, std::size_t p, std::string& name) {
    name.clear();
    bool ended = false;
    for (std::size_t i = 0; i < kNameSize; ++i) {
        const auto c = b[p + i];
        if (c == 0) { ended = true; continue; }
        if (ended || !((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            return false;
        name.push_back(static_cast<char>(c));
    }
    return !name.empty();
}
}
std::string decode_fls_structure(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path))
        throw std::runtime_error("FLS input is not a regular file: " + path.string());
    const auto size = std::filesystem::file_size(path);
    if (size < kHeaderSize || size > 256ULL * 1024ULL * 1024ULL)
        throw std::runtime_error("FLS size is outside the supported structural-decoder limits.");
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open FLS input.");
    std::vector<std::uint8_t> b(static_cast<std::size_t>(size));
    if (!in.read(reinterpret_cast<char*>(b.data()), static_cast<std::streamsize>(b.size())))
        throw std::runtime_error("Could not read the complete FLS file.");

    const auto candidate = u32(b, 0xC8);
    if (candidate >= kMaxRecords)
        throw std::runtime_error("Header record-count candidate is unreasonable.");

    // The sample stores 11 here but has 12 named 0x20-byte records (indices 0..11).
    // Accept count+1 only when every proposed record has a valid fixed-width name.
    const std::size_t count = static_cast<std::size_t>(candidate) + 1;
    if (count > (b.size() - kHeaderSize) / kRecordSize)
        throw std::runtime_error("Candidate directory extends beyond the file.");

    std::vector<std::string> names;
    names.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t p = kHeaderSize + i * kRecordSize;
        if (u32(b, p) != 0)
            throw std::runtime_error("Directory prefix validation failed at " + hx(p));
        std::string name;
        if (!valid_name(b, p + kNameOffset, name))
            throw std::runtime_error("Directory name validation failed at " + hx(p + kNameOffset));
        names.push_back(name);
    }

    std::ostringstream out;
    out << "C3350EMU FLS STRUCTURAL DECODE\n"
        << "==============================\n"
        << "File: " << path.string() << "\n"
        << "Size: " << size << " bytes\n"
        << "Header size observed: " << hx(kHeaderSize) << "\n"
        << "Directory start observed: " << hx(kHeaderSize) << "\n"
        << "Record size observed: " << hx(kRecordSize) << "\n"
        << "Header[0x00] version-like word: " << hx(u32(b, 0x00)) << "\n"
        << "Header[0xC8] count candidate: " << candidate
        << " (interpreted as highest index; records parsed = " << count << ")\n"
        << "Header[0xCC] raw value: " << hx(u32(b, 0xCC)) << "\n"
        << "Header[0xD0] raw value: " << hx(u32(b, 0xD0)) << "\n"
        << "Header[0xD4] raw value: " << hx(u32(b, 0xD4)) << "\n"
        << "Header[0xD8] raw value: " << hx(u32(b, 0xD8)) << "\n"
        << "Header[0xDC] raw value: " << hx(u32(b, 0xDC)) << "\n\n"
        << "[DIRECTORY RECORDS]\n";
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t p = kHeaderSize + i * kRecordSize;
        out << "#" << std::dec << i << " @ " << hx(p) << " name=" << names[i]
            << "\n  prefix=" << hx(u32(b, p))
            << " tail0=" << hx(u32(b, p + 20))
            << " tail1=" << hx(u32(b, p + 24))
            << " tail2=" << hx(u32(b, p + 28)) << "\n";
    }
    out << "\n[DECODER STATUS]\n"
        << "- Header and fixed-width directory layout matched the observed sample pattern.\n"
        << "- Names and three trailing 32-bit words are decoded as raw fields.\n"
        << "- Tail words are NOT yet identified as offsets, lengths, flags, checksums, or addresses.\n"
        << "- No payload was extracted and no load address or entry point was inferred.\n"
        << "- This is a structural decoder milestone, not a complete Samsung FLS specification.\n";
    return out.str();
}
} // namespace firmware
