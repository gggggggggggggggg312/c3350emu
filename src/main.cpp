#include <iostream>
#include "cpu/cpu.hpp"

int main() {
    CPU cpu;
    cpu.reset();

    std::cout << "C3350EMU CPU scaffold\n";
    std::cout << "Architecture: not yet confirmed\n";
    std::cout << "PC: 0x" << std::hex << cpu.getPC() << '\n';

    return 0;
}
