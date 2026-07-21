#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

#include "core/network.h"
#include "core/data_block.h"
#include "core/task.h"
#include "util/common.h"
#include "util/ini.h"
#include "util/random.h"

#include "phase.h"
#include "task_initializer.h"
#include "task_scheduler.h"

namespace planner
{
    TaskScheduler::TaskScheduler(const std::vector<std::pair<TimeSlotType, Tick>> &time_slot,
                                 TimeStamp schedule_begin_time, TimeStamp schedule_end_time,
                                 const core::Network &network,
                                 const std::map<DataBlockName, core::DataBlock> &data_blocks,
                                 const std::map<TaskName, core::Task> &tasks,
                                 double compute_omega, double buffer_omega,
                                 double spatial_omega, double temporal_omega,
                                 double dependency_omega)
        : num_time_steps_(time_slot.size()), time_slot_(time_slot),
          schedule_begin_time_(schedule_begin_time), schedule_end_time_(schedule_end_time),
          network_(network), data_blocks_(data_blocks), tasks_(tasks),
          compute_omega_(compute_omega), buffer_omega_(buffer_omega),
          spatial_omega_(spatial_omega), temporal_omega_(temporal_omega),
          dependency_omega_(dependency_omega)
    {
        ReCaculateUsedResource();
    }

    bool TaskScheduler::CheckAllTasksScheduled() const
    {
        for (const auto &[name, task] : tasks_)
        {
            if (task.GetScheduledTime() == INVALID_TIME || task.GetScheduledNode() == INVALID_NODE)
                return false;
        }
        return true;
    }

    std::tuple<std::vector<std::vector<TimeStamp>>, std::vector<std::vector<TimeStamp>>> TaskScheduler::GetDistanceMatrix() const
    {
        std::vector<std::vector<TimeStamp>> edge_distance(network_.GetNumNodes(), std::vector<TimeStamp>(network_.GetNumNodes(), network_.GetNumNodes()));
        std::vector<std::vector<TimeStamp>> chain_distance(network_.GetNumNodes(), std::vector<TimeStamp>(network_.GetNumNodes(), network_.GetNumNodes()));
        for (NodeId i = 0; i < network_.GetNumNodes(); i++)
            edge_distance[i][i] = chain_distance[i][i] = 0;
        for (const auto &[src, dst, _, __] : network_.GetEdges())
            edge_distance[src][dst] = 1;
        for (const auto &chain : network_.GetChains())
            for (NodeId to_node : chain.to_nodes)
                chain_distance[chain.from_node][to_node] = 1;
        for (NodeId k = 0; k < network_.GetNumNodes(); k++)
            for (NodeId i = 0; i < network_.GetNumNodes(); i++)
                for (NodeId j = 0; j < network_.GetNumNodes(); j++)
                {
                    if (edge_distance[i][k] + edge_distance[k][j] < edge_distance[i][j])
                        edge_distance[i][j] = edge_distance[i][k] + edge_distance[k][j];
                    if (chain_distance[i][k] + chain_distance[k][j] < chain_distance[i][j])
                        chain_distance[i][j] = chain_distance[i][k] + chain_distance[k][j];
                }
        return {edge_distance, chain_distance};
    }

    TimeStamp TaskScheduler::GetMSTWeight(const std::vector<NodeId> &nodes, const std::vector<std::vector<TimeStamp>> &distance)
    {
        if (nodes.empty())
            return 0;
        // Prim algorithm
        std::vector<bool> visited(nodes.size(), false);
        std::vector<TimeStamp> dis(nodes.size(), std::numeric_limits<TimeStamp>::max());
        NodeId last_node = 0;
        TimeStamp mst_weight = 0;
        for (size_t i = 1; i < nodes.size(); i++)
        {
            visited[last_node] = true;
            TimeStamp min_distance = std::numeric_limits<TimeStamp>::max();
            NodeId min_node = -1;
            for (size_t j = 0; j < nodes.size(); j++)
                if (!visited[j])
                {
                    if (distance[nodes[last_node]][nodes[j]] < dis[j])
                        dis[j] = distance[nodes[last_node]][nodes[j]];
                    if (dis[j] < min_distance)
                    {
                        min_distance = dis[j];
                        min_node = j;
                    }
                }
            last_node = min_node;
            mst_weight += min_distance;
        }
        return mst_weight;
    }

    void TaskScheduler::ReCaculateUsedResource()
    {
        used_compute_cycles_.clear();
        used_buffer_capacity_.clear();
        task_schedule_.clear();
        shared_memory_byte_.clear();
        produce_set_ = std::vector<std::multiset<std::pair<TimeStamp, NodeId>>>(data_blocks_.size());
        consume_set_ = std::vector<std::multiset<std::pair<TimeStamp, NodeId>>>(data_blocks_.size());

        for (auto &[_, data_block] : data_blocks_)
        {
            if (!data_block.GetSource().empty())
            {
                NodeId node = network_.GetNodeIdByArch(data_block.GetSource());
                produce_set_[data_block.GetId()].emplace(0, node);
            }
            if (!data_block.GetDestination().empty())
            {
                NodeId node = network_.GetNodeIdByArch(data_block.GetDestination());
                consume_set_[data_block.GetId()].emplace(num_time_steps_ - 1, node);
            }
        }

        for (auto &[name, task] : tasks_)
        {
            TimeStamp time = task.GetScheduledTime();
            NodeId node = task.GetScheduledNode();
            if (time == INVALID_TIME || node == INVALID_NODE)
                continue;
            task_schedule_[{time, node}].push_back(name);
            auto evaluation = network_.GetArch(node).Evaluate(task);
            used_compute_cycles_[{time, node}] += evaluation.execution_ticks;
            shared_memory_byte_[{time, node}] += task->GetSharedMemoryByte();
            for (const DataBlockName &input_block_name : task->GetInputBlockNames())
            {
                const core::DataBlock &data_block = data_blocks_.at(input_block_name);
                if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                    used_buffer_capacity_[{time, node}] += data_block.GetSize();
                consume_set_[data_block.GetId()].insert({time, node});
            }
            for (const DataBlockName &input_block_name : task->GetWeightBlockNames())
            {
                const core::DataBlock &data_block = data_blocks_.at(input_block_name);
                if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                    used_buffer_capacity_[{time, node}] += data_block.GetSize();
                consume_set_[data_block.GetId()].insert({time, node});
            }
            for (const DataBlockName &output_block_name : task->GetOutputBlockNames())
            {
                const core::DataBlock &data_block = data_blocks_.at(output_block_name);
                if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                    used_buffer_capacity_[{time, node}] += data_block.GetSize();
                produce_set_[data_block.GetId()].insert({time, node});
            }
        }
    }

    void TaskScheduler::AllocateTask(core::Task &task, TimeStamp time, NodeId node)
    {
        if (task.GetScheduledTime() != INVALID_TIME && task.GetScheduledNode() != INVALID_NODE)
            CancelAllocation(task);

        auto evaluation = network_.GetArch(node).Evaluate(task);
        task_schedule_[{time, node}].push_back(task->GetName());
        used_compute_cycles_[{time, node}] += evaluation.execution_ticks;
        shared_memory_byte_[{time, node}] += task->GetSharedMemoryByte();
        for (const DataBlockName &input_block_name : task->GetInputBlockNames())
        {
            const core::DataBlock &data_block = data_blocks_.at(input_block_name);
            if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                used_buffer_capacity_[{time, node}] += data_block.GetSize();
            consume_set_[data_block.GetId()].insert({time, node});
        }
        for (const DataBlockName &input_block_name : task->GetWeightBlockNames())
        {
            const core::DataBlock &data_block = data_blocks_.at(input_block_name);
            if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                used_buffer_capacity_[{time, node}] += data_block.GetSize();
            consume_set_[data_block.GetId()].insert({time, node});
        }
        for (const DataBlockName &output_block_name : task->GetOutputBlockNames())
        {
            const core::DataBlock &data_block = data_blocks_.at(output_block_name);
            if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                used_buffer_capacity_[{time, node}] += data_block.GetSize();
            produce_set_[data_block.GetId()].insert({time, node});
        }
        task.SetScheduledTime(time);
        task.SetScheduledNode(node);
    }

    void TaskScheduler::CancelAllocation(core::Task &task)
    {
        NodeId node = task.GetScheduledNode();
        TimeStamp time = task.GetScheduledTime();
        if (time == INVALID_TIME || node == INVALID_NODE)
            return;

        auto evaluation = network_.GetArch(node).Evaluate(task);
        task_schedule_[{time, node}].erase(std::find(task_schedule_[{time, node}].begin(), task_schedule_[{time, node}].end(), task->GetName()));
        used_compute_cycles_[{time, node}] -= evaluation.execution_ticks;
        shared_memory_byte_[{time, node}] -= task->GetSharedMemoryByte();
        for (const DataBlockName &input_block_name : task->GetInputBlockNames())
        {
            const core::DataBlock &data_block = data_blocks_.at(input_block_name);
            std::multiset<std::pair<TimeStamp, NodeId>> &produce_set = consume_set_[data_block.GetId()];
            produce_set.erase(produce_set.find({time, node}));
            if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                used_buffer_capacity_[{time, node}] -= data_block.GetSize();
        }
        for (const DataBlockName &input_block_name : task->GetWeightBlockNames())
        {
            const core::DataBlock &data_block = data_blocks_.at(input_block_name);
            std::multiset<std::pair<TimeStamp, NodeId>> &produce_set = consume_set_[data_block.GetId()];
            produce_set.erase(produce_set.find({time, node}));
            if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                used_buffer_capacity_[{time, node}] -= data_block.GetSize();
        }
        for (const DataBlockName &output_block_name : task->GetOutputBlockNames())
        {
            const core::DataBlock &data_block = data_blocks_.at(output_block_name);
            std::multiset<std::pair<TimeStamp, NodeId>> &consume_set = produce_set_[data_block.GetId()];
            consume_set.erase(consume_set.find({time, node}));
            if (produce_set_[data_block.GetId()].count({time, node}) == 0 && consume_set_[data_block.GetId()].count({time, node}) == 0)
                used_buffer_capacity_[{time, node}] -= data_block.GetSize();
        }
        task.SetScheduledTime(INVALID_TIME);
        task.SetScheduledNode(INVALID_NODE);
    }

    double TaskScheduler::ProxyEvaluation() const
    {
        if (!CheckAllTasksScheduled())
            return INFINITY;

        auto [edge_distance, chain_distance] = GetDistanceMatrix();

        double evaluation_temporal = 0.0;
        double evaluation_spatial = 0.0;
        double evaluation_compute = 0.0;
        double evaluation_buffer = 0.0;
        double evaluation_dependency = 0.0;
        for (auto &[_, data_block] : data_blocks_)
        {
            DataBlockId d = data_block.GetId();
            std::set<NodeId> nodes;
            double t_sum = 0.0;
            std::vector<double> dis_square_sum = std::vector<double>(network_.GetNumNodes(), 0);
            for (const auto &[time, node] : produce_set_[d])
            {
                t_sum += time;
                for (NodeId n = 0; n < network_.GetNumNodes(); ++n)
                {
                    double distance = edge_distance[node][n];
                    dis_square_sum[n] += distance * distance;
                }
            }
            for (const auto &[time, node] : consume_set_[d])
            {
                t_sum += time;
                for (NodeId n = 0; n < network_.GetNumNodes(); ++n)
                {
                    double distance = edge_distance[node][n];
                    dis_square_sum[n] += distance * distance;
                }
            }

            uint64_t count = produce_set_[d].size() + consume_set_[d].size();
            double min_dis_square = INFINITY;
            for (NodeId n = 0; n < network_.GetNumNodes(); ++n)
                min_dis_square = std::min(min_dis_square, dis_square_sum[n] / count);

            double t_avg = t_sum / count;
            double t_var = 0.0;
            for (const auto &[time, node] : produce_set_[d])
                t_var += (time - t_avg) * (time - t_avg);
            for (const auto &[time, node] : consume_set_[d])
                t_var += (time - t_avg) * (time - t_avg);
            t_var /= count;

            evaluation_temporal += temporal_omega_ * t_var * data_block.GetSize();
            evaluation_spatial += spatial_omega_ * min_dis_square * data_block.GetSize();
            // evaluation_temporal += temporal_omega_ * (t_max - t_min + 1) * data_block.GetSize();
            // evaluation_spatial += spatial_omega_ * GetMSTWeight(std::vector<NodeId>(nodes.begin(), nodes.end()), edge_distance) * data_block.GetSize();
        }

        for (const auto &[key, used_compute_cycles] : used_compute_cycles_)
        {
            auto [time, node] = key;
            double utilization = double(used_compute_cycles + 1) / (time_slot_[time].second + 1);
            evaluation_compute += compute_omega_ * util::pow(utilization, 10);
        }

        for (const auto &[key, used_buffer_capacity] : used_buffer_capacity_)
        {
            auto [time, node] = key;
            Size capacity = network_.GetArch(node)->GetBufferSizeBytes();
            double utilization = double(used_buffer_capacity - shared_memory_byte_.at(key)) / capacity;
            evaluation_buffer += buffer_omega_ * util::pow(utilization, 10);
        }

        for (auto &[_, data_block] : data_blocks_)
        {
            DataBlockId d = data_block.GetId();
            for (auto [prod_time, prod_node] : produce_set_[d])
                for (auto [cons_time, cons_node] : consume_set_[d])
                    if (prod_time + chain_distance[prod_node][cons_node] > cons_time)
                        evaluation_dependency += dependency_omega_ * (chain_distance[prod_node][cons_node] - (cons_time - prod_time)) * data_block.GetSize();
        }

        // std::cerr << "Temporal: " << evaluation_temporal << ", Spatial: " << evaluation_spatial << ", Compute: " << evaluation_compute << ", Buffer: " << evaluation_buffer << ", Dependency: " << evaluation_dependency << std::endl;
        return evaluation_temporal + evaluation_spatial + evaluation_compute + evaluation_buffer + evaluation_dependency;
    }

    std::vector<std::pair<TimeStamp, NodeId>> TaskScheduler::GetOverloadedTimeNode() const
    {
        std::vector<std::pair<TimeStamp, NodeId>> overloaded_time_node;
        for (TimeStamp time = schedule_begin_time_; time < schedule_end_time_; ++time)
            for (NodeId node = 0; node < network_.GetNumNodes(); ++node)
            {
                auto task_schedule_it = task_schedule_.find({time, node});
                if (task_schedule_it == task_schedule_.end() || task_schedule_it->second.empty())
                    continue;
                if (used_compute_cycles_.at({time, node}) > time_slot_[time].second || used_buffer_capacity_.at({time, node}) > network_.GetArch(node)->GetBufferSizeBytes())
                {
                    // std::cerr  << "Overloaded time node: " << time << ", " << node << ", used compute cycles: " << used_compute_cycles_.at({time, node}) << ", used buffer capacity: " << used_buffer_capacity_.at({time, node}) << ", task_num: " << task_schedule_it->second.size() << std::endl;
                    overloaded_time_node.push_back({time, node});
                }
            }
        return overloaded_time_node;
    }

    void TaskScheduler::AdjustAllocations()
    {
        std::vector<std::function<void()>> adjust_funcions =
            {
                [this]()
                {
                    std::vector<std::pair<TimeStamp, NodeId>> overloaded_time_node = GetOverloadedTimeNode();
                    std::vector<std::pair<TimeStamp, NodeId>> move_chain;

                    if (overloaded_time_node.empty() || util::uniform_d(0, 1) < 0.2)
                    {
                        TimeStamp time;
                        NodeId node;
                        do
                        {
                            time = util::uniform_u(schedule_begin_time_, schedule_end_time_ - 1);
                            node = util::uniform_u(0, network_.GetNumNodes() - 1);
                        } while (task_schedule_[{time, node}].empty());
                        move_chain.push_back({time, node});
                    }
                    else
                    {
                        auto [time, node] = overloaded_time_node[util::uniform_u(0, overloaded_time_node.size() - 1)];
                        move_chain.push_back({time, node});
                    }

                    do
                    {
                        TimeStamp time;
                        NodeId node;
                        do
                        {
                            time = util::uniform_u(schedule_begin_time_, schedule_end_time_ - 1);
                            node = util::uniform_u(0, network_.GetNumNodes() - 1);
                        } while (task_schedule_[{time, node}].empty());
                        move_chain.push_back({time, node});
                    } while (util::uniform_d(0.0, 1.0) < 0.5);

                    for (size_t i = 0; i < move_chain.size(); i++)
                    {
                        auto [time, node] = move_chain[(i + 1) % move_chain.size()];
                        core::Task &task = tasks_.at(task_schedule_[move_chain[i]][util::uniform_u(0, task_schedule_[move_chain[i]].size() - 1)]);
                        AllocateTask(task, time, node);
                    }
                },
                [this]()
                {
                    std::vector<std::pair<TimeStamp, NodeId>> overloaded_time_node = GetOverloadedTimeNode();
                    core::Task *task = nullptr;

                    if (overloaded_time_node.empty() || util::uniform_d(0, 1) < 0.1)
                    {
                        size_t id = util::uniform_u(0, tasks_.size() - 1);
                        for (auto &[name, t] : tasks_)
                            if (id-- == 0)
                            {
                                task = &t;
                                break;
                            }
                    }
                    else
                    {
                        auto [time, node] = overloaded_time_node[util::uniform_u(0, overloaded_time_node.size() - 1)];
                        task = &tasks_.at(task_schedule_[{time, node}][util::uniform_u(0, task_schedule_[{time, node}].size() - 1)]);
                        // std::cerr << "Moving task in " << time << " from node " << node << std::endl;
                    }
                    if (util::uniform_d(0.0, 1.0) < 0.1)
                    {
                        NodeId node;
                        TimeStamp time = util::uniform_u(schedule_begin_time_, schedule_end_time_ - 1);
                        do
                        {
                            node = util::uniform_u(0, network_.GetNumNodes() - 1);
                        } while (!network_.GetArch(node)->CanSchedule(*task));
                        AllocateTask(*task, time, node);
                    }
                    else
                    {
                        std::vector<std::pair<TimeStamp, NodeId>> idle_time_node;
                        for (NodeId node = 0; node < network_.GetNumNodes(); ++node)
                            if (network_.GetArch(node)->CanSchedule(*task))
                            {
                                auto evaluation = network_.GetArch(node).Evaluate(*task);
                                for (TimeStamp t = schedule_begin_time_; t < schedule_end_time_; ++t)
                                    if (evaluation.execution_ticks + used_compute_cycles_[{t, node}] <= time_slot_[t].second)
                                        idle_time_node.push_back({t, node});
                            }
                        TimeStamp time;
                        NodeId node;
                        if (!idle_time_node.empty())
                        {
                            auto [t, n] = idle_time_node[util::uniform_u(0, idle_time_node.size() - 1)];
                            time = t;
                            node = n;
                        }
                        else
                        {
                            time = util::uniform_u(schedule_begin_time_, schedule_end_time_ - 1);
                            do
                            {
                                node = util::uniform_u(0, network_.GetNumNodes() - 1);
                            } while (!network_.GetArch(node)->CanSchedule(*task));
                        }
                        AllocateTask(*task, time, node);
                    }
                    // std::cerr << "Moving task" << " to node " << node << std::endl;
                }};
        std::vector<double> probabilities = {0.5, 0.5};
        adjust_funcions[util::weighted_choice(probabilities)]();
    }

    void TaskScheduler::InitializeAllocations()
    {
        LoadTaskAllocation();
        if (CheckAllTasksScheduled())
            return;

        TaskInitializer task_initializer(this);
        task_initializer.Initialize();

        SaveTaskAllocation();
    }

    void TaskScheduler::SearchForTaskAllocation()
    {
        InitializeAllocations();

        std::map<TaskName, core::Task> best_allocations = tasks_;
        std::map<TaskName, core::Task> current_allocations = tasks_;
        double best_evaluation = ProxyEvaluation();
        double current_evaluation = best_evaluation;

        std::cerr << "Initial Evaluation: " << best_evaluation << std::endl;

        double T = 1e1, T_min = 1e-4, lambda = 0.99;
        double num_iteration = 1;

        do
        {
            while (T > T_min)
            {
                num_iteration = std::min(2000.0, num_iteration * 1.005);
                for (size_t i = 0; i < num_iteration; i++)
                {
                    AdjustAllocations();
                    double new_evaluation = ProxyEvaluation();
                    double delta = new_evaluation - current_evaluation;
                    if (delta < 0 || util::uniform_d(0.0, 1.0) < exp(-delta / T))
                    {
                        current_allocations = tasks_;
                        current_evaluation = new_evaluation;
                        if (current_evaluation < best_evaluation)
                        {
                            best_allocations = current_allocations;
                            best_evaluation = current_evaluation;
                            if (!search_result_path_.empty())
                                SaveTaskAllocation();
                        }
                    }
                    else
                    {
                        tasks_ = current_allocations;
                        ReCaculateUsedResource();
                    }
                }
                std::cerr << "Temperature: " << T << ", Best Evaluation: " << best_evaluation << ", Current Evaluation: " << current_evaluation << std::endl;
                T *= lambda;
            }
            T_min *= 0.1;
            tasks_ = best_allocations;
            ReCaculateUsedResource();
        } while (!CheckValidSolution());
        std::cerr << "Final Best Evaluation: " << best_evaluation << std::endl;

        dataflow_scheduler_ = std::make_shared<DataflowScheduler>(time_slot_, schedule_begin_time_, schedule_end_time_, network_, data_blocks_, best_allocations);
        dataflow_scheduler_->EstimateDataflowOverhead(DataflowScheduler::Relaxed);
    }

    void TaskScheduler::SaveTaskAllocation() const
    {
        if (search_result_path_.empty())
            return;
        std::map<std::string, std::map<std::string, std::string>> config;
        for (const auto &[name, task] : tasks_)
        {
            config[task->GetName()]["time"] = std::to_string(task.GetScheduledTime() - schedule_begin_time_);
            config[task->GetName()]["node"] = network_.GetNodeName(task.GetScheduledNode());
        }
        util::WriteIniConfig(search_result_path_, config);
    }

    void TaskScheduler::LoadTaskAllocation()
    {
        if (search_result_path_.empty())
            return;
        auto config = util::ReadIniConfig(search_result_path_);
        for (auto &[name, task] : tasks_)
        {
            auto it = config.find(name);
            if (it != config.end())
            {
                task.SetScheduledTime(schedule_begin_time_ + std::stoul(it->second.at("time")));
                task.SetScheduledNode(network_.GetNodeId(it->second.at("node")));
            }
        }
        ReCaculateUsedResource();
    }

    void TaskScheduler::RandomShuffle()
    {
        std::vector<core::Task *> tasks;
        for (auto &[_, task] : tasks_)
            tasks.push_back(&task);
        for (size_t i = 1; i < tasks.size(); i++)
        {
            size_t j = util::uniform_i(0, i - 1);
            TimeStamp time_i = tasks[i]->GetScheduledTime();
            NodeId node_i = tasks[i]->GetScheduledNode();
            TimeStamp time_j = tasks[j]->GetScheduledTime();
            NodeId node_j = tasks[j]->GetScheduledNode();
            AllocateTask(*tasks[i], time_j, node_j);
            AllocateTask(*tasks[j], time_i, node_i);
        }
    }

    bool TaskScheduler::CheckValidSolution() const
    {
        if (!CheckAllTasksScheduled())
            return false;

        for (const auto &[key, used_compute_cycles] : used_compute_cycles_)
        {
            auto [time, node] = key;
            double utilization = double(used_compute_cycles + 1) / (time_slot_[time].second + 1);
            if (utilization > 1)
                return false;
        }

        for (const auto &[key, used_buffer_capacity] : used_buffer_capacity_)
        {
            auto [time, node] = key;
            Size capacity = network_.GetArch(node)->GetBufferSizeBytes();
            double utilization = double(used_buffer_capacity - shared_memory_byte_.at(key)) / capacity;
            if (utilization > 1)
                return false;
        }
        return true;
    }
} // namespace planner