#include <iostream>

#include "planner/framework.h"
#include "planner/phase.h"
#include "util/cJSON.h"
#include "util/ini.h"

int main(int argc, char *argv[])
{
    std::string lib_name = argv[1];
    std::string root_path = "library/" + lib_name;

    planner::Framework framework(root_path);
    if (argc == 2)
    {
        for (size_t i = 0; i < framework.GetNumPhases(); i++)
        {
            planner::Phase &phase = framework.GetPhase(i);
            auto task_allocation = util::ReadIniConfig(root_path + "/" + phase.GetPhaseName() + ".ini");
            for (auto &[task_name, allocation] : task_allocation)
            {
                std::string node_name = allocation.at("node");
                std::string time_str = allocation.at("time");
                TimeStamp time = std::stoull(time_str);
                if (!phase.ScheduleTask(time, node_name, task_name))
                    std::cerr << "Failed to schedule task " << task_name << " at time " << time << " on node " << node_name << std::endl;
                else
                    std::cout << "Scheduled task " << task_name << " at time " << time << " on node " << node_name << std::endl;
            }
        }
        auto [total_time, total_noc_energy, total_dram_energy] = framework.DataflowSchedule();
        std::cout << "Total dataflow overhead: " << total_time << " ticks, " << total_noc_energy << " pJ NOC energy, " << total_dram_energy << " pJ DRAM energy" << std::endl;
    }
    else if (argc == 3)
    {
        uint64_t phase_id = std::stoull(argv[2]);
        planner::Phase &phase = framework.GetPhase(phase_id);

        auto task_allocation = util::ReadIniConfig(root_path + "/" + phase.GetPhaseName() + ".ini");
        for (auto &[task_name, allocation] : task_allocation)
        {
            std::string node_name = allocation.at("node");
            std::string time_str = allocation.at("time");
            TimeStamp time = std::stoull(time_str);
            if (!phase.ScheduleTask(time, node_name, task_name))
                std::cerr << "Failed to schedule task " << task_name << " at time " << time << " on node " << node_name << std::endl;
            else
                std::cout << "Scheduled task " << task_name << " at time " << time << " on node " << node_name << std::endl;
        }

        auto [time, noc_energy, dram_energy] = phase.EstimateDataflowOverhead(planner::DataflowScheduler::Strict, 300);
        std::cout << "Dataflow overhead: " << time << " ticks, " << noc_energy << " pJ NOC energy, " << dram_energy << " pJ DRAM energy" << std::endl;
    }
    return 0;
}