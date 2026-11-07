#include "lpddr1.hpp"

namespace memory {

std::vector<std::string> Lpddr1Protocol::initialize() {
    ready_ = false;
    std::vector<std::string> trace = {
        "Configure memory type and timing parameters (board-specific values unknown)",
        "Issue NOP",
        "Wait at least 200 microseconds after power-up (trace only; no real sleep)",
        "Issue NOP",
        "Precharge all banks",
        "Auto-refresh cycle 1",
        "Auto-refresh cycle 2",
        "Load extended mode register (value unknown)",
        "Load mode register (value unknown)",
        "Enter normal operation",
        "Configure refresh interval (board-specific value unknown)"
    };
    ready_ = true;
    return trace;
}

bool Lpddr1Protocol::ready() const noexcept {
    return ready_;
}

} // namespace memory
