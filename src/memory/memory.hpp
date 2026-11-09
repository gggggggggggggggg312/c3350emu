#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class Memory {
public:
    struct Region {
        std::uint32_t base;
        std::vector<std::uint8_t> bytes;
        bool writable;
        std::string name;
    };

    void map(std::uint32_t base, std::size_t size, bool writable,
             const std::string& name);
    void map_filled(std::uint32_t base, std::size_t size, bool writable,
                    const std::string& name, std::uint8_t initial_value);
    std::uint8_t read8(std::uint32_t address) const;
    std::uint32_t read32le(std::uint32_t address) const;
    void write8(std::uint32_t address, std::uint8_t value);
    void write32le(std::uint32_t address, std::uint32_t value);
    void write_block(std::uint32_t address, const std::vector<std::uint8_t>& data);
    std::size_t region_count() const noexcept;

private:
    const Region& find(std::uint32_t address, std::size_t width) const;
    Region& find_writable(std::uint32_t address, std::size_t width);
    std::vector<Region> regions_;
};
