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
        framework.TaskSchedule();
    else if (argc == 3)
    {
        uint64_t phase_id = std::stoull(argv[2]);
        planner::Phase &phase = framework.GetPhase(phase_id);
        // planner::Phase &attention = framework.GetPhases()[1];

        phase.ScheduleTaskAllocation();
    }

    // std::cout << "Dataflow overhead: " << ffn_1.EstimateDataflowOverhead() << std::endl;
    return 0;
}