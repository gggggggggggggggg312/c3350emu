#include "platform.hpp"

namespace platform {

const std::vector<ResearchFact>& gt_c3350_research_profile() {
    // Research findings only. No physical addresses or register offsets are inferred.
    static const std::vector<ResearchFact> facts = {
        {"Target", "Samsung GT-C3350", "Confirmed target"},
        {"Baseband platform", "Intel XMM2150 family", "Strong model/firmware evidence"},
        {"CPU family", "ARM11", "Platform-family documentation"},
        {"Instruction architecture", "ARMv6 family", "Derived from ARM11 family; validate firmware image"},
        {"Flash technology", "MuxOneNAND candidate", "Part-number evidence; exact fitted variant needs confirmation"},
        {"Flash part marking", "K5W1G12ACP(C3350)S", "Reported in a GT-C3350 service log"},
        {"Bootloader identifier", "2150_OD_KJ1_3.1", "Reported in a GT-C3350 flashing log"},
        {"Download protocol", "3.32", "Reported in a GT-C3350 flashing log"},
        {"Boot mode", "BB", "Reported in a GT-C3350 flashing log"},
        {"Flash ID", "003000EC", "Reported in a GT-C3350 flashing log"},
        {"Download baud rate", "921600", "Reported in a GT-C3350 flashing log"},
        {"Physical RAM map", "Unknown", "Not established"},
        {"CPU core revision and clock", "Unknown", "Not established"},
        {"OneNAND register map", "Unknown", "Exact fitted device documentation required"},
        {"Firmware load addresses", "Unknown", "Must be extracted from verified FLS records"}
    };
    return facts;
}

} // namespace platform
