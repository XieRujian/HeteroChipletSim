#include <cstring>
#include <iostream>
#include <map>

#include "planner/framework.h"
#include "planner/phase.h"
#include "util/ini.h"

int main(int argc, char *argv[])
{
    std::string lib_name = argv[1];
    std::string root_path = "library/" + lib_name;
    planner::Framework framework(root_path);
    uint32_t itr = std::stoull(argv[2]);
    if (strcmp(argv[3], "fixed_num") == 0)
    {
        std::map<ArchName, uint32_t> arch_num;
        for (uint64_t i = 4; i < argc; i += 2)
            arch_num.emplace(ArchName(argv[i]), std::stoull(argv[i + 1]));
        framework.FindArchFixedNum(itr, arch_num);
    }
    else if (strcmp(argv[3], "fixed_prob") == 0)
    {
        std::map<ArchName, double> arch_prob;
        for (uint64_t i = 4; i < argc; i += 2)
            arch_prob.emplace(ArchName(argv[i]), std::stod(argv[i + 1]));
        framework.FindArchFixedProb(itr, arch_prob);
    }

    // std::cout << "Dataflow overhead: " << ffn_1.EstimateDataflowOverhead() << std::endl;
    return 0;
}