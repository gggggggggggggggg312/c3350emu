#include "memory.hpp"

#include <limits>
#include <algorithm>
#include <stdexcept>

void Memory::map(std::uint32_t base, std::size_t size, bool writable,
                 const std::string& name) {
    map_filled(base, size, writable, name, 0);
}

void Memory::map_filled(std::uint32_t base, std::size_t size, bool writable,
                        const std::string& name, std::uint8_t initial_value) {
    if (size == 0) throw std::invalid_argument("Memory region cannot be empty.");
    const std::uint64_t end = static_cast<std::uint64_t>(base) + size;
    if (end > (std::uint64_t{1} << 32))
        throw std::invalid_argument("Memory region exceeds the 32-bit address space.");

    for (const auto& region : regions_) {
        const std::uint64_t other_end =
            static_cast<std::uint64_t>(region.base) + region.bytes.size();
        if (base < other_end && region.base < end)
            throw std::invalid_argument("Memory regions must not overlap.");
    }

    regions_.push_back(Region{base, std::vector<std::uint8_t>(size, initial_value),
                              writable, name});
}

const Memory::Region& Memory::find(std::uint32_t address,
                                   std::size_t width) const {
    const std::uint64_t end = static_cast<std::uint64_t>(address) + width;
    for (const auto& region : regions_) {
        const std::uint64_t region_end =
            static_cast<std::uint64_t>(region.base) + region.bytes.size();
        if (address >= region.base && end <= region_end)
            return region;
    }
    throw std::out_of_range("Unmapped memory access.");
}

Memory::Region& Memory::find_writable(std::uint32_t address,
                                      std::size_t width) {
    const auto& found = static_cast<const Memory&>(*this).find(address, width);
    for (auto& region : regions_) {
        if (&region == &found) {
            if (!region.writable)
                throw std::runtime_error("Attempted write to read-only memory.");
            return region;
        }
    }
    throw std::out_of_range("Unmapped memory access.");
}

std::uint8_t Memory::read8(std::uint32_t address) const {
    const auto& region = find(address, 1);
    return region.bytes[address - region.base];
}

std::uint32_t Memory::read32le(std::uint32_t address) const {
    const auto& region = find(address, 4);
    const auto offset = address - region.base;
    return static_cast<std::uint32_t>(region.bytes[offset]) |
           (static_cast<std::uint32_t>(region.bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(region.bytes[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(region.bytes[offset + 3]) << 24);
}

void Memory::write8(std::uint32_t address, std::uint8_t value) {
    auto& region = find_writable(address, 1);
    region.bytes[address - region.base] = value;
}

void Memory::write32le(std::uint32_t address, std::uint32_t value) {
    auto& region = find_writable(address, 4);
    const auto offset = address - region.base;
    for (unsigned i = 0; i < 4; ++i)
        region.bytes[offset + i] =
            static_cast<std::uint8_t>((value >> (i * 8)) & 0xff);
}

std::size_t Memory::region_count() const noexcept {
    return regions_.size();
}

void Memory::write_block(std::uint32_t address,
                         const std::vector<std::uint8_t>& data) {
    if (data.empty()) throw std::invalid_argument("Memory write block cannot be empty.");
    auto& region = find_writable(address, data.size());
    const auto offset = static_cast<std::size_t>(address - region.base);
    std::copy(data.begin(), data.end(), region.bytes.begin() +
              static_cast<std::ptrdiff_t>(offset));
}
