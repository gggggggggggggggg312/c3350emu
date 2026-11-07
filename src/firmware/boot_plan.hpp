#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

class Memory;

namespace firmware {
struct BootSegment {
    std::filesystem::path file;
    std::uint64_t offset = 0;
    std::size_t length = 0;
    std::uint32_t load_address = 0;
};
enum class MemoryInitKind { Copy, Zero };
struct MemoryInitOperation { MemoryInitKind kind = MemoryInitKind::Zero; std::uint32_t source = 0; std::uint32_t destination = 0; std::size_t length = 0; };
struct BootPlan {
    std::uint32_t ram_base = 0;
    std::size_t ram_size = 0;
    std::uint32_t entry = 0;
    std::uint64_t instruction_limit = 0;
    std::filesystem::path trace_file;
    std::vector<std::uint32_t> breakpoints;
    std::vector<BootSegment> segments;
    std::vector<MemoryInitOperation> memory_initialization;
};
BootPlan parse_boot_plan(const std::filesystem::path& path);
void apply_memory_initialization(const BootPlan& plan, Memory& memory);
}
