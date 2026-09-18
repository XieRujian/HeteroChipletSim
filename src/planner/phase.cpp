#include <set>
#include <stdexcept>

#include "core/task/tasks.h"
#include "math/ilp_solver.h"
#include "util/cJSON.h"

#include "framework.h"
#include "phase.h"
#include "task_scheduler.h"

namespace planner
{
    core::Network &Phase::GetNetwork() noexcept { return framework_ptr_->GetNetwork(); }
    const core::Arch &Phase::GetArch(NodeId node) const { return framework_ptr_->GetArch(node); }

    void Phase::Load(std::string json_path)
    {
        cJSON *phase_parse = cJSON_Parse(util::GetFileContent(json_path).c_str());
        for (cJSON *item = phase_parse->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "tasks")
                for (cJSON *task_parse = item->child; task_parse; task_parse = task_parse->next)
                    tasks_.emplace(task_parse->string, core::Task(task_parse));
            else if (key == "data_blocks")
                for (cJSON *data_block_parse = item->child; data_block_parse; data_block_parse = data_block_parse->next)
                {
                    core::DataBlock data_block(data_block_parse);
                    data_blocks_.emplace(data_block.GetName(), data_block);
                }
            else if (key == "prefix_transmission_time")
                prefix_transmission_time_ = item->valueint;
            else if (key == "suffix_transmission_time")
                suffix_transmission_time_ = item->valueint;
            else if (key == "base_time_slot_ticks")
                base_time_slot_ticks_ = item->valueint;
            else if (key == "compute_omega")
                compute_omega_ = item->valuedouble;
            else if (key == "buffer_omega")
                buffer_omega_ = item->valuedouble;
            else if (key == "spatial_omega")
                spatial_omega = item->valuedouble;
            else if (key == "temporal_omega")
                temporal_omega = item->valuedouble;
            else if (key == "dependency_omega")
                dependency_omega = item->valuedouble;
            else if (key == "data_distributor")
                data_distributor_ = std::shared_ptr<DataDistributor>(CreateDataDistributor(item));
        }

        DataBlockId data_block_id = 0;
        for (auto &[name, data_block] : data_blocks_)
        {
            data_block.SetId(data_block_id++);
            if (!data_block.GetSource().empty())
                GetNetwork().GetArch(GetNetwork().GetNodeIdByArch(data_block.GetSource()))->AddPreStoreDataBlock(name);
        }

        CalculateMinimalComputeTime();
    }

    void Phase::CalculateMinimalComputeTime()
    {
        std::vector<Tick> node_compute_time(GetNetwork().GetNumNodes(), 0);
        Tick max_node_compute_time = 0;
        compute_ticks_.clear();
        for (const auto &[name, task] : tasks_)
        {
            NodeId min_node = -1;
            Tick min_node_end_time = std::numeric_limits<Tick>::max();
            for (NodeId node = 0; node < GetNetwork().GetNumNodes(); node++)
            {
                auto task_overhead = GetArch(task.GetScheduledNode()).Evaluate(task);
                if (node_compute_time[node] + task_overhead.execution_ticks < min_node_end_time)
                {
                    min_node_end_time = node_compute_time[node] + task_overhead.execution_ticks;
                    min_node = node;
                }
            }

            if (min_node_end_time > base_time_slot_ticks_)
            {
                compute_ticks_.push_back(max_node_compute_time);
                node_compute_time.assign(GetNetwork().GetNumNodes(), 0);
                for (NodeId node = 0; node < GetNetwork().GetNumNodes(); node++)
                {
                    auto task_overhead = GetArch(task.GetScheduledNode()).Evaluate(task);
                    if (task_overhead.execution_ticks < min_node_end_time)
                    {
                        min_node_end_time = task_overhead.execution_ticks;
                        min_node = node;
                    }
                }
                node_compute_time[min_node] = min_node_end_time;
                max_node_compute_time = min_node_end_time;
            }
            else
            {
                node_compute_time[min_node] = min_node_end_time;
                max_node_compute_time = std::max(max_node_compute_time, min_node_end_time);
            }
        }

        compute_ticks_.push_back(max_node_compute_time);
    }

    void Phase::CancelTaskSchedule(TaskName task_name)
    {
        auto task_it = tasks_.find(task_name);
        if (task_it == tasks_.end())
            return;
        core::Task &task = task_it->second;
        if (task.GetScheduledTime() != INVALID_TIME && task.GetScheduledNode() != INVALID_NODE)
        {
            auto task_overhead = GetArch(task.GetScheduledNode()).Evaluate(task);
            used_compute_ticks_[{task.GetScheduledTime(), task.GetScheduledNode()}] -= task_overhead.execution_ticks;
            task.SetScheduledTime(INVALID_TIME);
            task.SetScheduledNode(INVALID_NODE);
        }
    }

    bool Phase::ScheduleTask(TimeStamp time, NodeId node, const TaskName &task_name)
    {
        if (time < 0 || time >= (TimeStamp)compute_ticks_.size())
            return false;
        if (node < 0 || node >= GetNetwork().GetNumNodes())
            return false;

        auto task_it = tasks_.find(task_name);
        if (task_it == tasks_.end())
            return false;
        core::Task &task = task_it->second;
        if (task.GetScheduledTime() == time && task.GetScheduledNode() == node)
            return true;

        if (task.GetScheduledTime() != INVALID_TIME && task.GetScheduledNode() != INVALID_NODE)
            CancelTaskSchedule(task_name);

        auto task_overhead = GetArch(node).Evaluate(task);
        Tick &node_overhead = used_compute_ticks_[{time, node}];
        if (node_overhead + task_overhead.execution_ticks > base_time_slot_ticks_)
            return false;
        node_overhead += task_overhead.execution_ticks;

        task.SetScheduledTime(time);
        task.SetScheduledNode(node);
        return true;
    }

    bool Phase::CheckAllTasksScheduled() const
    {
        for (const auto &[name, task] : tasks_)
        {
            if (task.GetScheduledTime() == INVALID_TIME || task.GetScheduledNode() == INVALID_NODE)
                return false;
        }
        return true;
    }

    std::tuple<Tick, Energy, Energy> Phase::EstimateDataflowOverhead(DataflowScheduler::OverheadEstimateType type, Second time_limit)
    {
        dataflow_scheduler_ = std::make_shared<DataflowScheduler>(compute_ticks_, prefix_transmission_time_, suffix_transmission_time_, GetNetwork(), GetDataBlocks(), GetTasks());
        util::MakeDirectory(framework_ptr_->GetRootPath() + "/ilp_intermediate_result/");
        util::MakeDirectory(framework_ptr_->GetRootPath() + "/data_analysis/");
        dataflow_scheduler_->SetIntermediateResultPath(framework_ptr_->GetRootPath() + "/ilp_intermediate_result/" + phase_name_ + ".txt");
        dataflow_scheduler_->SetDataAnalysisPath(framework_ptr_->GetRootPath() + "/data_analysis/" + phase_name_ + ".txt");
        return dataflow_scheduler_->EstimateDataflowOverhead(type, time_limit);
    }

    void Phase::ScheduleTaskAllocation()
    {
        task_scheduler_ = std::make_shared<TaskScheduler>(compute_ticks_, base_time_slot_ticks_, suffix_transmission_time_, GetNetwork(), data_blocks_, GetTasks(), compute_omega_, buffer_omega_, spatial_omega, temporal_omega, dependency_omega);
        util::MakeDirectory(framework_ptr_->GetRootPath() + "/search_results/");
        task_scheduler_->SetSearchResultPath(framework_ptr_->GetRootPath() + "/search_results/" + phase_name_ + ".ini");
        task_scheduler_->SearchForTaskAllocation();
    }
}