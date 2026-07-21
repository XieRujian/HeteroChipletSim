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
            else if (key == "time_slot")
                for (cJSON *time_slot_parse = item->child; time_slot_parse; time_slot_parse = time_slot_parse->next)
                {
                    TimeSlotType time_slot_type = TimeSlotType::Normal;
                    Tick tick = 0;
                    for (cJSON *item_parse = time_slot_parse->child; item_parse; item_parse = item_parse->next)
                    {
                        std::string item_key(item_parse->string);
                        if (item_key == "type")
                        {
                            if (std::string(item_parse->valuestring) == "Normal")
                                time_slot_type = TimeSlotType::Normal;
                            else if (std::string(item_parse->valuestring) == "Forward")
                                time_slot_type = TimeSlotType::Forward;
                            else if (std::string(item_parse->valuestring) == "Backward")
                                time_slot_type = TimeSlotType::Backward;
                            else
                                throw std::runtime_error("Invalid time slot type: " + std::string(item_parse->valuestring));
                        }
                        else if (item_key == "cycles")
                            tick = item_parse->valueint;
                    }
                    time_slot_.emplace_back(time_slot_type, tick);
                }
            else if (key == "schedule_begin")
                schedule_begin_time_ = item->valueint;
            else if (key == "schedule_end")
                schedule_end_time_ = item->valueint;
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
        }
        num_time_steps_ = time_slot_.size();

        DataBlockId data_block_id = 0;
        for (auto &[name, data_block] : data_blocks_)
        {
            data_block.SetId(data_block_id++);
            if (!data_block.GetSource().empty())
                GetNetwork().GetArch(GetNetwork().GetNodeIdByArch(data_block.GetSource()))->AddPreStoreDataBlock(name);
        }
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
        if (time < schedule_begin_time_ || time > schedule_end_time_)
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
        if (node_overhead + task_overhead.execution_ticks > time_slot_[time].second)
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
        dataflow_scheduler_ = std::make_shared<DataflowScheduler>(time_slot_, schedule_begin_time_, schedule_end_time_, GetNetwork(), GetDataBlocks(), GetTasks());
        util::MakeDirectory(framework_ptr_->GetRootPath() + "/ilp_intermediate_result/");
        util::MakeDirectory(framework_ptr_->GetRootPath() + "/data_analysis/");
        dataflow_scheduler_->SetIntermediateResultPath(framework_ptr_->GetRootPath() + "/ilp_intermediate_result/" + phase_name_ + ".txt");
        dataflow_scheduler_->SetDataAnalysisPath(framework_ptr_->GetRootPath() + "/data_analysis/" + phase_name_ + ".txt");
        return dataflow_scheduler_->EstimateDataflowOverhead(type, time_limit);
    }

    void Phase::ScheduleTaskAllocation()
    {
        task_scheduler_ = std::make_shared<TaskScheduler>(time_slot_, schedule_begin_time_, schedule_end_time_, GetNetwork(), data_blocks_, GetTasks(), compute_omega_, buffer_omega_, spatial_omega, temporal_omega, dependency_omega);
        util::MakeDirectory(framework_ptr_->GetRootPath() + "/search_results/");
        task_scheduler_->SetSearchResultPath(framework_ptr_->GetRootPath() + "/search_results/" + phase_name_ + ".ini");
        task_scheduler_->SearchForTaskAllocation();
    }
}