#include "platform.hpp"

namespace platform {

const std::vector<ResearchFact>& gt_c3350_research_profile() {
    // Research findings only. No physical addresses or register offsets are inferred.
    static const std::vector<ResearchFact> facts = {
        {"Target", "Samsung GT-C3350", "Confirmed target"},
        {"Baseband platform", "Intel XMM2150 family", "Strong model/firmware evidence"},
        {"CPU family", "ARM11", "Platform-family documentation"},
        {"Instruction architecture", "ARMv6 family", "Derived from ARM11 family; validate firmware image"},
        {"Memory package", "K5W1G12ACP family; likely OneNAND + DDR MCP", "C3350 marking plus matching XMM2150 sibling schematic; exact C3350 variant unverified"},
        {"Memory interfaces", "Separate OneNAND and DDR interfaces", "Shown on related GT-S5222/XMM2150 schematic; not a C3350 board schematic"},
        {"DDR bus width", "16-bit on related XMM2150 board", "Sibling schematic signal group DDR_DQ(0:15); not independently verified on C3350"},
        {"Flash part marking", "K5W1G12ACP(C3350)S", "Reported in a GT-C3350 service log"},
        {"Bootloader identifier", "2150_OD_KJ1_3.1", "Reported in a GT-C3350 flashing log"},
        {"Download protocol", "3.32", "Reported in a GT-C3350 flashing log"},
        {"Boot mode", "BB", "Reported in a GT-C3350 flashing log"},
        {"Flash ID", "003000EC", "Reported in a GT-C3350 flashing log"},
        {"Download baud rate", "921600", "Reported in a GT-C3350 flashing log"},
        {"Memory hardware", "SRAM plus NAND/LPDDR1 subsystem", "Intel XMM 2130/2150 platform brief"},
        {"Physical RAM map", "Unknown", "No C3350-specific base address or capacity established"},
        {"RAM initialization", "Generic LPDDR1 protocol trace implemented; XMM2150 sequence unknown", "Reference model only; no controller register programming"},
        {"CPU core revision and clock", "Unknown", "Not established"},
        {"OneNAND register map", "Unknown", "Exact fitted device documentation required"},
        {"Firmware load addresses", "Unknown", "Must be extracted from verified FLS records"}
    };
    return facts;
}

} // namespace platform
