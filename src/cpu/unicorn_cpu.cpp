#include "unicorn_cpu.hpp"
#include <unicorn/unicorn.h>
#include <unicorn/arm.h>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <sstream>

namespace cpu {
namespace {
constexpr std::uint32_t PageSize = 0x1000;
const int ArmRegisters[16] = {
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
    UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11,
    UC_ARM_REG_R12, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC
};
void check(uc_err error, const char* operation) {
    if (error != UC_ERR_OK) {
        throw std::runtime_error(std::string(operation) + ": " + uc_strerror(error));
    }
}
}
UnicornRunResult run_unicorn_image(
    const std::vector<std::uint8_t>& image,
    std::uint32_t ram_base,
    std::uint32_t ram_size,
    std::uint32_t entry,
    std::uint64_t instruction_limit) {
    if (image.empty() || instruction_limit == 0 || ram_size == 0)
        throw std::runtime_error("Image, RAM size, and instruction limit must be nonzero.");
    if ((ram_base & (PageSize - 1u)) != 0)
        throw std::runtime_error("RAM base must be 4 KiB aligned.");
    if (image.size() > ram_size)
        throw std::runtime_error("Image does not fit in configured RAM.");
    const std::uint64_t end = static_cast<std::uint64_t>(ram_base) + ram_size;
    if (end > 0x100000000ull || entry < ram_base || entry >= end)
        throw std::runtime_error("RAM range or entry point is invalid.");
    const std::uint64_t mapped_size =
        (static_cast<std::uint64_t>(ram_size) + PageSize - 1u) & ~(static_cast<std::uint64_t>(PageSize) - 1u);
    if (static_cast<std::uint64_t>(ram_base) + mapped_size > 0x100000000ull)
        throw std::runtime_error("Page-aligned RAM mapping exceeds the 32-bit address space.");

    uc_engine* engine = nullptr;
    const bool thumb = (entry & 1u) != 0;
    const auto mode = static_cast<uc_mode>(UC_MODE_LITTLE_ENDIAN | (thumb ? UC_MODE_THUMB : UC_MODE_ARM));
    check(uc_open(UC_ARCH_ARM, mode, &engine), "uc_open");
    try {
        check(uc_mem_map(engine, ram_base, static_cast<std::size_t>(mapped_size), UC_PROT_ALL), "uc_mem_map");
        check(uc_mem_write(engine, ram_base, image.data(), image.size()), "uc_mem_write");
        std::uint32_t cpsr = thumb ? 0xF3u : 0xD3u;
        check(uc_reg_write(engine, UC_ARM_REG_CPSR, &cpsr), "set CPSR");
        const std::uint32_t start = entry & ~1u;
        const auto error = uc_emu_start(engine, start, static_cast<std::uint64_t>(end), 0, instruction_limit);

        UnicornRunResult result;
        result.instructions = instruction_limit;
        for (unsigned i = 0; i < 16; ++i)
            check(uc_reg_read(engine, ArmRegisters[i], &result.registers[i]), "read ARM register");
        check(uc_reg_read(engine, UC_ARM_REG_CPSR, &result.cpsr), "read CPSR");
        result.pc = result.registers[15];
        if (error == UC_ERR_OK) result.stop_reason = "execution stopped at end address or instruction limit";
        else result.stop_reason = uc_strerror(error);
        uc_close(engine);
        return result;
    } catch (...) {
        uc_close(engine);
        throw;
    }
}
}
