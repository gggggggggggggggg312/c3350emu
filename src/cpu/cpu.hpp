#pragma once
#include "memory/memory.hpp"
#include <array>
#include <cstdint>
class CPU {
public:
    explicit CPU(Memory& memory);
    void reset(std::uint32_t entry_point = 0);
    void step();
    [[nodiscard]] std::uint32_t getPC() const;
    [[nodiscard]] std::uint32_t get_register(unsigned index) const;
    void set_register(unsigned index, std::uint32_t value);
    [[nodiscard]] std::uint32_t get_cpsr() const;
    [[nodiscard]] std::uint64_t instructions_executed() const noexcept;
private:
    bool condition_passed(unsigned condition) const;
    std::uint32_t read_operand_register(unsigned index) const;
    void execute_data_processing(std::uint32_t instruction);
    void execute_single_transfer(std::uint32_t instruction);
    void execute_branch(std::uint32_t instruction);
    void execute_multiply(std::uint32_t instruction);
    void execute_block_transfer(std::uint32_t instruction);
    void execute_thumb(std::uint16_t instruction);
    Memory& memory_;
    std::array<std::uint32_t, 16> registers_{};
    std::uint32_t cpsr_ = 0;
    std::uint64_t instructions_executed_ = 0;
};
