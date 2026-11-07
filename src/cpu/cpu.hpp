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
    struct TlbEntry {
        bool valid = false;
        std::uint32_t virtual_page = 0;
        std::uint32_t physical_page = 0;
        std::uint32_t ttbr = 0;
        std::uint32_t context_id = 0;
        unsigned domain = 0;
        unsigned ap = 0;
    };
    void invalidate_tlb_all();
    void invalidate_tlb_mva(std::uint32_t address);
    struct CacheLine { bool valid = false; std::uint32_t tag = 0; std::array<std::uint8_t, 32> bytes{}; };
    static constexpr std::size_t CacheLineCount = 128;
    mutable std::array<CacheLine, CacheLineCount> instruction_cache_{};
    mutable std::array<CacheLine, CacheLineCount> data_cache_{};
    std::uint8_t cached_read8(std::uint32_t physical, bool instruction) const;
    void cached_write8(std::uint32_t physical, std::uint8_t value);
    void invalidate_instruction_cache(std::uint32_t address, bool all);
    void invalidate_data_cache(std::uint32_t address, bool all);
    std::uint32_t translate_address(std::uint32_t virtual_address, bool write) const;
    std::uint8_t read_virtual8(std::uint32_t address) const;
    std::uint32_t read_virtual32(std::uint32_t address) const;
    void write_virtual8(std::uint32_t address, std::uint8_t value);
    void write_virtual32(std::uint32_t address, std::uint32_t value);
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
    mutable std::array<TlbEntry, 64> tlb_{};
    Memory& memory_;
    std::array<std::uint32_t, 16> registers_{};
    std::uint32_t cpsr_ = 0;
    std::uint64_t instructions_executed_ = 0;
};
