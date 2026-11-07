#pragma once

#include <cstdint>

class CPU {
public:
    CPU();

    void reset();
    void step();

    [[nodiscard]] std::uint32_t getPC() const;
    [[nodiscard]] bool isInitialized() const;

private:
    std::uint32_t pc_;
    bool initialized_;
};
