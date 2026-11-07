#pragma once

#include <string>
#include <vector>

// Protocol-level LPDDR1 initialization reference.
// This is NOT an XMM2150 controller/register implementation.
namespace memory {

class Lpddr1Protocol {
public:
    static constexpr unsigned required_power_up_wait_us = 200;

    // Returns a documented generic initialization trace. No real delay is
    // performed and no hardware registers or memory addresses are accessed.
    std::vector<std::string> initialize();
    bool ready() const noexcept;

private:
    bool ready_ = false;
};

} // namespace memory
