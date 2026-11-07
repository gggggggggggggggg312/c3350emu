#pragma once

#include <string>
#include <vector>

namespace platform {

struct ResearchFact {
    std::string label;
    std::string value;
    std::string confidence;
};

const std::vector<ResearchFact>& gt_c3350_research_profile();

} // namespace platform
