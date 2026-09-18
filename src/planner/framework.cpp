#include <cassert>
#include <iostream>

#include "util/cJSON.h"
#include "util/thread.h"

#include "framework.h"
#include "phase.h"

namespace planner
{
    Framework::Framework(const std::string &root_path) : root_path_(root_path)
    {
        cJSON *framework_parse = cJSON_Parse(util::GetFileContent(root_path + "/framework.json").c_str());
        cJSON *topology_parse = nullptr;
        cJSON *arch_parse = nullptr;
        cJSON *phases_parse = nullptr;
        for (cJSON *item = framework_parse->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "topology")
                topology_parse = item;
            else if (key == "arch")
                arch_parse = item;
            else if (key == "phases")
                phases_parse = item;
        }

        assert(topology_parse != nullptr);
        network_.LoadTopology(topology_parse->valuestring);
        network_.LoadArch(arch_parse->valuestring);

        assert(phases_parse != nullptr);
        for (cJSON *phase = phases_parse->child; phase; phase = phase->next)
        {
            phase_configs_.push_back(PhaseConfig{21600, 1, Phase(this, std::string(phase->string))});
            std::string config_path;
            for (cJSON *item = phase->child; item; item = item->next)
            {
                std::string key(item->string);
                if (key == "dataflow_schedule_time_limit")
                    phase_configs_.back().dataflow_schedule_time_limit = item->valuedouble;
                else if (key == "repetition")
                    phase_configs_.back().repetition = item->valueint;
                else if (key == "config")
                    config_path = item->valuestring;
            }
            phase_configs_.back().phase.Load(config_path);
        }
    }

    void Framework::TaskSchedule()
    {
        util::task_manager.RunTasks<PhaseConfig>(
            [](PhaseConfig &phase_config)
            { phase_config.phase.ScheduleTaskAllocation(); }, phase_configs_);
    }

    std::tuple<Tick, Energy, Energy> Framework::DataflowSchedule()
    {
        Tick total_time = 0;
        Energy total_noc_energy = 0;
        Energy total_dram_energy = 0;
        for (auto &phase_config : phase_configs_)
        {
            // Estimate the dataflow overhead for each phase
            auto [time, noc_energy, dram_energy] = phase_config.phase.EstimateDataflowOverhead(DataflowScheduler::OverheadEstimateType::Strict, phase_config.dataflow_schedule_time_limit);
            total_time += time * phase_config.repetition;
            total_noc_energy += noc_energy * phase_config.repetition;
            total_dram_energy += dram_energy * phase_config.repetition;
        }
        return {total_time, total_noc_energy, total_dram_energy};
    }
}