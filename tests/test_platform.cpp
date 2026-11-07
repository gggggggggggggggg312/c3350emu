#include "platform/platform.hpp"

#include <cassert>
#include <string>

int main() {
    const auto& facts = platform::gt_c3350_research_profile();
    assert(!facts.empty());

    bool found_target = false;
    bool found_unknown_ram = false;
    for (const auto& fact : facts) {
        if (fact.label == "Target" && fact.value == "Samsung GT-C3350")
            found_target = true;
        if (fact.label == "Physical RAM map" && fact.value == "Unknown")
            found_unknown_ram = true;
    }

    assert(found_target);
    assert(found_unknown_ram);
    return 0;
}
