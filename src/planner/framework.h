#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <memory>
#include <string>
#include <vector>

#include "core/network.h"
#include "core/arch.h"
#include "core/task.h"
#include "util/common.h"

#include "phase.h"

namespace planner
{
    class Framework
    {
    private:
        struct PhaseConfig
        {
            Second dataflow_schedule_time_limit;
            uint64_t repetition;
            Tick base_time_slot_ticks_;
            Phase phase;
            std::shared_ptr<DataDistributor> data_distributor_;
        };

    private:
        std::string root_path_;
        std::vector<PhaseConfig> phase_configs_;
        std::map<ArchName, core::Arch> library_;
        core::Network network_;

    public:
        Framework(const std::string &root_path);

        size_t GetNumPhases() const noexcept { return phase_configs_.size(); }
        Phase &GetPhase(size_t index) noexcept { return phase_configs_[index].phase; }
        const std::string &GetRootPath() const noexcept { return root_path_; }
        const core::Network &GetNetwork() const noexcept { return network_; }
        core::Arch GetArch(ArchName arch_name) const { return library_.at(arch_name); }
        core::Arch GetArch(NodeId node) const { return library_.at(network_.GetArch(node)); }

        void FindArchFixedProb(uint64_t itr, const std::map<ArchName, double> &arch_prob);
        void FindArchFixedNum(uint64_t itr, const std::map<ArchName, uint32_t> &arch_num);
        void Schedule();
        std::tuple<Tick, Energy, Energy> Evaluate() const;
    };
}

#endif // FRAMEWORK_H