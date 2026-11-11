#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace cpu {
struct UnicornRunResult {
    std::uint32_t pc = 0;
    std::uint32_t cpsr = 0;
    std::uint64_t instructions = 0;
    std::uint32_t registers[16]{};
    std::string stop_reason;
};

// Executes a raw image with Unicorn. The image is placed at ram_base.
// This is an execution backend, not yet a complete C3350 boot environment.
UnicornRunResult run_unicorn_image(
    const std::vector<std::uint8_t>& image,
    std::uint32_t ram_base,
    std::uint32_t ram_size,
    std::uint32_t entry,
    std::uint64_t instruction_limit);
}
