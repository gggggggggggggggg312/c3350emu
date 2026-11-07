#include "memory/lpddr1.hpp"

#include <cassert>

int main() {
    memory::Lpddr1Protocol lpddr;
    assert(!lpddr.ready());
    const auto trace = lpddr.initialize();
    assert(lpddr.ready());
    assert(trace.size() == 11);
    assert(trace[2].find("200 microseconds") != std::string::npos);
    assert(trace[5] == "Auto-refresh cycle 1");
    assert(trace[6] == "Auto-refresh cycle 2");
    assert(trace.back().find("refresh interval") != std::string::npos);
    return 0;
}
