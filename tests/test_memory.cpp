#include "memory/memory.hpp"

#include <cassert>
#include <stdexcept>

int main() {
    Memory memory;
    memory.map(0x1000, 0x100, true, "test-ram");
    memory.write32le(0x1004, 0x12345678);
    assert(memory.read32le(0x1004) == 0x12345678);
    assert(memory.read8(0x1004) == 0x78);

    bool overlap_rejected = false;
    try { memory.map(0x1080, 0x100, true, "overlap"); }
    catch (const std::invalid_argument&) { overlap_rejected = true; }
    assert(overlap_rejected);

    bool unmapped_rejected = false;
    try { (void)memory.read8(0x5000); }
    catch (const std::out_of_range&) { unmapped_rejected = true; }
    assert(unmapped_rejected);

    Memory deviceMemory;
    std::uint32_t deviceRegister = 0;
    deviceMemory.map_device(0x2000, 0x100, "test-device",
        [&](std::uint32_t off) { return static_cast<std::uint8_t>(deviceRegister >> ((off & 3u) * 8u)); },
        [&](std::uint32_t off, std::uint8_t value) { const auto shift=(off & 3u)*8u; deviceRegister=(deviceRegister & ~(0xffu<<shift)) | (static_cast<std::uint32_t>(value)<<shift); });
    deviceMemory.write32le(0x2000, 0xA1B2C3D4);
    assert(deviceMemory.read32le(0x2000) == 0xA1B2C3D4);

    Memory rom;
    rom.map(0x8000, 16, false, "test-rom");
    bool write_rejected = false;
    try { rom.write8(0x8000, 1); }
    catch (const std::runtime_error&) { write_rejected = true; }
    assert(write_rejected);
    return 0;
}
