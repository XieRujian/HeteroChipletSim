#ifndef PHASE_H
#define PHASE_H

#include <map>
#include <memory>
#include <set>
#include <string>

#include "core/network.h"
#include "core/arch.h"
#include "core/data_block.h"
#include "math/ilp_solver.h"
#include "util/common.h"

#include "dataflow_scheduler.h"
#include "data_distributor.h"

namespace planner
{
    class Framework;
    class TaskScheduler;

    class Phase
    {
    private:
        Framework *framework_ptr_;
        std::string phase_name_;
        TimeStamp prefix_transmission_time_ = 4;
        TimeStamp suffix_transmission_time_ = 4;
        std::vector<Tick> compute_ticks_;
        std::map<TaskName, core::Task> tasks_;
        std::map<DataBlockName, core::DataBlock> data_blocks_;

        std::map<std::pair<TimeStamp, NodeId>, Tick> used_compute_ticks_;

        void CalculateMinimalComputeTime(Tick base_time_slot_ticks_);

    private:
        double compute_omega_ = 10;
        double buffer_omega_ = 100;
        double spatial_omega = 1e-4;
        double temporal_omega = 3e-5;
        double dependency_omega = 1;

    public:
        Phase(Framework *framework_ptr, const std::string &phase_name, const std::string &json_path);

        const std::string &GetPhaseName() const noexcept { return phase_name_; }

        const std::map<TaskName, core::Task> &GetTasks() const noexcept { return tasks_; }
        std::map<DataBlockName, core::DataBlock> &GetDataBlocks() noexcept { return data_blocks_; }
        const std::map<DataBlockName, core::DataBlock> &GetDataBlocks() const noexcept { return data_blocks_; }
        const core::Network &GetNetwork() const noexcept;
        core::Arch GetArch(NodeId node) const;
        core::Arch GetArch(ArchName arch_name) const;

        //void CancelTaskSchedule(TaskName task_name);
        //bool ScheduleTask(TimeStamp time, NodeId node, const TaskName &task_name);
        //bool ScheduleTask(TimeStamp time, const NodeName &node_name, const TaskName &task_name) { return ScheduleTask(time, GetNetwork().GetNodeId(node_name), task_name); }
        //bool CheckAllTasksScheduled() const;

        std::tuple<Tick, Energy, Energy> ScheduleAndEvaluate(DataflowScheduler::OverheadEstimateType type = DataflowScheduler::OverheadEstimateType::Strict, Second time_limit = 21600) const;
        //std::tuple<Tick, Energy, Energy> EstimateDataflowOverhead(DataflowScheduler::OverheadEstimateType type = DataflowScheduler::OverheadEstimateType::Strict, Second time_limit = 21600) const;
    };
}

#endif // PHASE_H