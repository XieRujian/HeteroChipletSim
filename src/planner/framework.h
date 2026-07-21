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
            Phase phase;
        };

    private:
        std::string root_path_;
        std::vector<PhaseConfig> phase_configs_;
        core::Network network_;

    public:
        Framework(const std::string &root_path);

        size_t GetNumPhases() const noexcept { return phase_configs_.size(); }
        Phase &GetPhase(size_t index) noexcept { return phase_configs_[index].phase; }
        const std::string &GetRootPath() const noexcept { return root_path_; }
        core::Network &GetNetwork() noexcept { return network_; }
        core::Arch &GetArch(NodeId node) { return network_.GetArch(node); }

        void TaskSchedule();
        std::tuple<Tick, Energy, Energy> DataflowSchedule();
    };
}

#endif // FRAMEWORK_H