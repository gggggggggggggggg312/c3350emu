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
    void execute_halfword_transfer(std::uint32_t instruction);
    void execute_branch(std::uint32_t instruction);
    void execute_multiply(std::uint32_t instruction);
    void execute_long_multiply(std::uint32_t instruction);
    void execute_block_transfer(std::uint32_t instruction);
    void execute_thumb(std::uint16_t instruction);
    void execute_cp15_transfer(std::uint32_t instruction);
    void switch_mode(std::uint32_t new_mode);
    void execute_status_transfer(std::uint32_t instruction);
    void enter_svc_exception(std::uint32_t return_address);
    std::uint32_t* active_spsr();
    std::array<std::uint32_t, 7> user_bank_{};
    std::array<std::uint32_t, 7> fiq_bank_{};
    std::array<std::uint32_t, 2> irq_bank_{};
    std::array<std::uint32_t, 2> svc_bank_{};
    std::array<std::uint32_t, 2> abort_bank_{};
    std::array<std::uint32_t, 2> undefined_bank_{};
    std::uint32_t spsr_fiq_ = 0, spsr_irq_ = 0, spsr_svc_ = 0;
    std::uint32_t spsr_abort_ = 0, spsr_undefined_ = 0;
    std::uint32_t cp15_control_ = 0;
    std::uint32_t cp15_ttbr0_ = 0;
    std::uint32_t cp15_dacr_ = 0;
    std::uint32_t cp15_context_id_ = 0;
    Memory& memory_;
    std::array<std::uint32_t, 16> registers_{};
    std::uint32_t cpsr_ = 0;
    std::uint64_t instructions_executed_ = 0;
};
