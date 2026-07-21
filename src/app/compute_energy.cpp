#include <iostream>

#include "core/task/tasks.h"
#include "planner/framework.h"
#include "planner/phase.h"
#include "util/cJSON.h"
#include "util/ini.h"

int main(int argc, char *argv[])
{
    std::string lib_name = argv[1];
    std::string root_path = "library/" + lib_name;
    planner::Framework framework(root_path);

    // std::cout << framework.GetNetwork().GetArch(0)->Evaluate(core::Gemm(2, 828, 256)).energy_pj << std::endl;
    // std::cout << framework.GetNetwork().GetArch(0)->Evaluate(core::Gemm(2, 256, 1024)).energy_pj << std::endl;
    // std::cout << framework.GetNetwork().GetArch(0)->Evaluate(core::Gemm(2, 1024, 256)).energy_pj << std::endl;
    // std::cout << framework.GetNetwork().GetArch(0)->Evaluate(core::Gemm(2, 256, 28)).energy_pj << std::endl;

    for (size_t i = 0; i < framework.GetNumPhases(); i++)
    {
        planner::Phase &phase = framework.GetPhase(i);
        auto task_allocation = util::ReadIniConfig(root_path + "/" + phase.GetPhaseName() + ".ini");
        double energy = 0;
        for (auto &[task_name, allocation] : task_allocation)
        {
            std::string node_name = allocation.at("node");
            energy += phase.GetNetwork().GetArch(framework.GetNetwork().GetNodeId(node_name)).Evaluate(phase.GetTasks().at(task_name)).energy_pj;
        }
        std::cout << energy << std::endl;
    }

    return 0;
}