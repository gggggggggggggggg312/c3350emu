#include "cpu.hpp"

CPU::CPU()
    : pc_(0), initialized_(false) {
}

void CPU::reset() {
    pc_ = 0;
    initialized_ = false;
}

void CPU::step() {
    if (!initialized_) {
        return;
    }

    // Instruction execution will be implemented after the CPU
    // architecture, execution state, and memory map are verified.
}

std::uint32_t CPU::getPC() const {
    return pc_;
}

bool CPU::isInitialized() const {
    return initialized_;
}
