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
        Tick base_time_slot_ticks_;
        TimeStamp prefix_transmission_time_;
        TimeStamp suffix_transmission_time_;
        std::vector<Tick> compute_ticks_;
        std::map<TaskName, core::Task> tasks_;
        std::map<DataBlockName, core::DataBlock> data_blocks_;

        std::map<std::pair<TimeStamp, NodeId>, Tick> used_compute_ticks_;
        std::shared_ptr<DataDistributor> data_distributor_;

        void CalculateMinimalComputeTime();

    private:
        double compute_omega_ = 10;
        double buffer_omega_ = 100;
        double spatial_omega = 1e-4;
        double temporal_omega = 3e-5;
        double dependency_omega = 1;
        std::shared_ptr<DataflowScheduler> dataflow_scheduler_;
        std::shared_ptr<TaskScheduler> task_scheduler_;

    public:
        Phase(Framework *framework_ptr, const std::string &phase_name) : framework_ptr_(framework_ptr), phase_name_(phase_name) {}
        void Load(std::string json_path);

        const std::string &GetPhaseName() const noexcept { return phase_name_; }

        //const std::vector<std::pair<TimeSlotType, Tick>> &GetTimeSlot() const noexcept { return time_slot_; }
        //TimeStamp GetNumTimeSteps() const noexcept { return num_time_steps_; }
        //TimeStamp GetScheduleBeginTime() const noexcept { return schedule_begin_time_; }
        //TimeStamp GetScheduleEndTime() const noexcept { return schedule_end_time_; }
        const std::map<TaskName, core::Task> &GetTasks() const noexcept { return tasks_; }
        const std::map<DataBlockName, core::DataBlock> &GetDataBlocks() const noexcept { return data_blocks_; }
        core::Network &GetNetwork() noexcept;
        const core::Arch &GetArch(NodeId node) const;

        void CancelTaskSchedule(TaskName task_name);
        bool ScheduleTask(TimeStamp time, NodeId node, const TaskName &task_name);
        bool ScheduleTask(TimeStamp time, const NodeName &node_name, const TaskName &task_name) { return ScheduleTask(time, GetNetwork().GetNodeId(node_name), task_name); }
        bool CheckAllTasksScheduled() const;

        void ScheduleTaskAllocation();
        std::tuple<Tick, Energy, Energy> EstimateDataflowOverhead(DataflowScheduler::OverheadEstimateType type = DataflowScheduler::OverheadEstimateType::Strict, Second time_limit = 21600);
    };
}

#endif // PHASE_H