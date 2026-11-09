#include <cassert>
#include <iostream>

#include "util/cJSON.h"
#include "util/ini.h"
#include "util/thread.h"
#include "util/random.h"

#include "framework.h"
#include "phase.h"

namespace planner
{
    Framework::Framework(const std::string &root_path) : root_path_(root_path)
    {
        cJSON *framework_parse = cJSON_Parse(util::GetFileContent(root_path + "/framework.json").c_str());
        cJSON *topology_parse = nullptr;
        cJSON *library_parse = nullptr;
        cJSON *phases_parse = nullptr;
        for (cJSON *item = framework_parse->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "topology")
                topology_parse = item;
            else if (key == "library")
                library_parse = item;
            else if (key == "phases")
                phases_parse = item;
        }

        assert(topology_parse != nullptr);
        network_.LoadTopology(topology_parse->valuestring);

        assert(library_parse != nullptr);
        auto library_configs = util::ReadIniConfig(library_parse->valuestring);
        for (auto &[arch_name, arch_config] : library_configs)
            library_.emplace(arch_name, core::Arch(arch_name, arch_config));

        assert(phases_parse != nullptr);
        for (cJSON *phase = phases_parse->child; phase; phase = phase->next)
        {
            Second dataflow_schedule_time_limit = 21600;
            uint64_t repetition = 1;
            std::string config_path;
            Tick base_time_slot_ticks_ = 0;
            std::shared_ptr<DataDistributor> data_distributor_;
            for (cJSON *item = phase->child; item; item = item->next)
            {
                std::string key(item->string);
                if (key == "dataflow_schedule_time_limit")
                    dataflow_schedule_time_limit = item->valueint;
                else if (key == "repetition")
                    repetition = item->valueint;
                else if (key == "config")
                    config_path = item->valuestring;
                else if (key == "base_time_slot_ticks")
                    base_time_slot_ticks_ = item->valueint;
                else if (key == "data_distributor")
                    data_distributor_ = CreateDataDistributor(item);
            }
            phase_configs_.push_back(PhaseConfig{dataflow_schedule_time_limit, repetition, base_time_slot_ticks_, Phase(this, std::string(phase->string), config_path), data_distributor_});
        }
    }

    std::tuple<Tick, Energy, Energy> Framework::Evaluate() const
    {
        Tick total_time = 0;
        Energy total_noc_energy = 0;
        Energy total_dram_energy = 0;
        for (auto &phase_config : phase_configs_)
        {
            // Estimate the dataflow overhead for each phase
            auto [time, noc_energy, dram_energy] = phase_config.phase.ScheduleAndEvaluate(DataflowScheduler::OverheadEstimateType::Strict, phase_config.dataflow_schedule_time_limit);
            total_time += time * phase_config.repetition;
            total_noc_energy += noc_energy * phase_config.repetition;
            total_dram_energy += dram_energy * phase_config.repetition;
        }
        return {total_time, total_noc_energy, total_dram_energy};
    }

    void Framework::Schedule()
    {
        util::task_manager.RunTasks<PhaseConfig>(
            [this](PhaseConfig &config)
            {
                config.data_distributor_->DistributeDataBlocks(library_, network_, config.phase.GetDataBlocks(), config.base_time_slot_ticks_);
            },
            phase_configs_);
    }

    void Framework::FindArchFixedProb(uint64_t itr, const std::map<ArchName, double> &arch_prob)
    {
        std::vector<ArchName> names;
        std::vector<double> probs;
        for (auto &[arch, prob] : arch_prob)
        {
            names.push_back(arch);
            probs.push_back(prob);
        }
        for (uint64_t i = 0; i < itr; i++)
        {
            for (NodeId node = 0; node < network_.GetNumNodes(); node++)
                if (network_.GetArch(node) != "Storage")
                    network_.SetArch(node, names[util::weighted_choice(probs)]);
            Schedule(), Evaluate();
        }
    }

    void Framework::FindArchFixedNum(uint64_t itr, const std::map<ArchName, uint32_t> &arch_num)
    {
        std::vector<ArchName> archs;
        for (auto &[arch_name, arch_num] : arch_num)
            for (uint32_t i = 0; i < arch_num; i++)
                archs.push_back(arch_name);
        for (uint64_t i = 0; i < itr; i++)
        {
            util::shuffle(archs);
            for (NodeId node = 0, arch_id = 0; node < network_.GetNumNodes(); node++)
                if (network_.GetArch(node) != "Storage")
                    network_.SetArch(node, archs[arch_id++ % archs.size()]);
            Schedule(), Evaluate();
        }
    }
}