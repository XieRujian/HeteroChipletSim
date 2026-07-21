#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <map>
#include <memory>
#include <vector>

#include "core/network.h"
#include "core/arch.h"
#include "core/data_block.h"
#include "core/task.h"
#include "util/common.h"

#include "dataflow_scheduler.h"

namespace planner
{
    class TaskScheduler
    {
        TimeStamp num_time_steps_;
        std::vector<std::pair<TimeSlotType, Tick>> time_slot_;
        TimeStamp schedule_begin_time_;
        TimeStamp schedule_end_time_;
        core::Network network_;
        std::map<DataBlockName, core::DataBlock> data_blocks_;
        std::map<TaskName, core::Task> tasks_;

        std::shared_ptr<DataflowScheduler> dataflow_scheduler_;

    public:
        TaskScheduler(const std::vector<std::pair<TimeSlotType, Tick>> &time_slot,
                      TimeStamp schedule_begin_time, TimeStamp schedule_end_time,
                      const core::Network &network,
                      const std::map<DataBlockName, core::DataBlock> &data_blocks,
                      const std::map<TaskName, core::Task> &tasks,
                      double compute_omega = 10, double buffer_omega = 100,
                      double spatial_omega = 1e-4, double temporal_omega = 3e-5,
                      double dependency_omega = 1);

        void SetSearchResultPath(const std::string &path) { search_result_path_ = path; }
        bool CheckAllTasksScheduled() const;
        void SearchForTaskAllocation();

        friend class TaskInitializer;

    private:
        double compute_omega_;
        double buffer_omega_;
        double spatial_omega_;
        double temporal_omega_;
        double dependency_omega_;

        std::map<std::pair<TimeStamp, NodeId>, std::vector<TaskName>> task_schedule_;

        std::map<std::pair<TimeStamp, NodeId>, Tick> used_compute_cycles_;
        std::map<std::pair<TimeStamp, NodeId>, Size> used_buffer_capacity_;
        std::map<std::pair<TimeStamp, NodeId>, Size> shared_memory_byte_;
        std::vector<std::multiset<std::pair<TimeStamp, NodeId>>> produce_set_;
        std::vector<std::multiset<std::pair<TimeStamp, NodeId>>> consume_set_;

        std::string search_result_path_;

        void ReCaculateUsedResource();
        void CancelAllocation(core::Task &task);
        void AllocateTask(core::Task &task, TimeStamp time, NodeId node);
        std::vector<std::pair<TimeStamp, NodeId>> GetOverloadedTimeNode() const;
        std::tuple<std::vector<std::vector<TimeStamp>>, std::vector<std::vector<TimeStamp>>> GetDistanceMatrix() const;
        static TimeStamp GetMSTWeight(const std::vector<NodeId> &nodes, const std::vector<std::vector<TimeStamp>> &distance);
        double ProxyEvaluation() const;
        void InitializeAllocations();
        void AdjustAllocations();
        void RandomShuffle();

        bool CheckValidSolution() const;
        void SaveTaskAllocation() const;
        void LoadTaskAllocation();
    };
} // namespace planner

#endif // TASK_SCHEDULER_H