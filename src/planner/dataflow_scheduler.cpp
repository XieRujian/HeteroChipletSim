#include <fstream>
#include <iostream>
#include <stdexcept>

#include "core/network.h"
#include "core/arch.h"
#include "core/task.h"
#include "core/data_block.h"

#include "dataflow_scheduler.h"
#include "phase.h"

namespace planner
{
    DataflowScheduler::DataflowScheduler(const std::vector<std::pair<TimeSlotType, Tick>> &time_slot, Tick schedule_begin_time, Tick schedule_end_time, const core::Network &network, const std::map<DataBlockName, core::DataBlock> &data_blocks, const std::map<TaskName, core::Task> &tasks)
        : num_time_steps_(time_slot.size()), time_slot_(time_slot), schedule_begin_time_(schedule_begin_time), schedule_end_time_(schedule_end_time), network_(network), data_blocks_(data_blocks)
    {
        for (const auto &[name, task] : tasks)
            tasks_.push_back(task);
    }

    bool DataflowScheduler::CheckAllTasksScheduled() const
    {
        for (const auto &task : tasks_)
        {
            if (task.GetScheduledTime() == INVALID_TIME || task.GetScheduledNode() == INVALID_NODE)
                return false;
        }
        return true;
    }

    DataflowScheduler::ReachabilitySets DataflowScheduler::BuildReachabilitySets() const
    {
        std::vector<std::set<std::pair<TimeStamp, NodeId>>> produce_set(data_blocks_.size());
        std::vector<std::set<std::pair<TimeStamp, NodeId>>> consume_set(data_blocks_.size());
        std::set<std::tuple<TimeStamp, NodeId, DataBlockId>> reachable_set;
        std::set<std::tuple<TimeStamp, NodeId, DataBlockId>> can_reach_set;
        std::map<std::pair<TimeStamp, NodeId>, Size> shared_memory_byte;

        DataBlockId data_block_num = data_blocks_.size();
        NodeId node_num = network_.GetNumNodes();

        // ===== 初始化：已知的初始可达性 =====
        for (auto &[_, data_block] : data_blocks_)
        {
            DataBlockId d = data_block.GetId();
            if (!data_block.GetSource().empty())
            {
                NodeId node = network_.GetNodeIdByArch(data_block.GetSource());
                reachable_set.insert({0, node, d});
                produce_set[d].insert({0, node});
            }
            if (!data_block.GetDestination().empty())
            {
                NodeId node = network_.GetNodeIdByArch(data_block.GetDestination());
                can_reach_set.insert({num_time_steps_ - 1, node, d});
                consume_set[d].insert({num_time_steps_ - 1, node});
            }
        }

        // 任务生成的数据在执行时刻出现在执行节点
        for (const auto &task : tasks_)
        {
            TimeStamp time = task.GetScheduledTime();
            NodeId node = task.GetScheduledNode();
            for (const DataBlockName &input_block_name : task->GetInputBlockNames())
            {
                const core::DataBlock &data_block = data_blocks_.at(input_block_name);
                can_reach_set.insert({time, node, data_block.GetId()});
                consume_set[data_block.GetId()].insert({time, node});
            }
            for (const DataBlockName &input_block_name : task->GetWeightBlockNames())
            {
                const core::DataBlock &data_block = data_blocks_.at(input_block_name);
                can_reach_set.insert({time, node, data_block.GetId()});
                consume_set[data_block.GetId()].insert({time, node});
            }
            for (const DataBlockName &output_block_name : task->GetOutputBlockNames())
            {
                const core::DataBlock &data_block = data_blocks_.at(output_block_name);
                reachable_set.insert({time, node, data_block.GetId()});
                produce_set[data_block.GetId()].insert({time, node});
            }
            shared_memory_byte[{time, node}] += task->GetSharedMemoryByte();
        }

        // ===== 前向传播：计算能到达的位置 =====
        for (TimeStamp t = 1; t < num_time_steps_; t++)
        {
            // 数据可以沿着边流动
            for (const core::Chain &chain : network_.GetChains())
                for (DataBlockId d = 0; d < data_block_num; d++)
                    if (reachable_set.count({t - 1, chain.from_node, d}))
                        for (NodeId to_node : chain.to_nodes)
                            reachable_set.insert({t, to_node, d});

            // 数据可以在节点上停留
            for (NodeId n = 0; n < node_num; n++)
                for (DataBlockId d = 0; d < data_block_num; d++)
                    if (reachable_set.count({t - 1, n, d}))
                        reachable_set.insert({t, n, d});
        }

        // ===== 反向传播：计算能被用上的数据 =====
        for (TimeStamp t = num_time_steps_; --t;)
        {
            // 反向沿着边传播
            for (const core::Chain &chain : network_.GetChains())
                for (DataBlockId d = 0; d < data_block_num; d++)
                    for (NodeId to_node : chain.to_nodes)
                        if (can_reach_set.count({t, to_node, d}))
                            can_reach_set.insert({t - 1, chain.from_node, d});

            // 反向沿着节点传播
            for (NodeId n = 0; n < node_num; n++)
                for (DataBlockId d = 0; d < data_block_num; d++)
                    if (can_reach_set.count({t, n, d}))
                        can_reach_set.insert({t - 1, n, d});
        }

        return {shared_memory_byte, produce_set, consume_set, reachable_set, can_reach_set};
    }

    std::tuple<Tick, Energy, Energy> DataflowScheduler::SolveDataflowILP(math::VariableType var_type, Second time_limit)
    {
        solver_.Clear();

        if (!CheckAllTasksScheduled())
            throw std::runtime_error("Not all tasks are scheduled.");

        NodeId node_num = network_.GetNumNodes();
        DataBlockId data_block_num = data_blocks_.size();

        auto [shared_memory_byte, produce_set, consume_set, reachable_set, can_reach_set] = BuildReachabilitySets();

        auto buffer = [this](TimeStamp t, NodeId n, DataBlockId d) -> math::Variable
        {
            return solver_.GetVariable("buffer_" + std::to_string(t) + "_" + std::to_string(n) + "_" + std::to_string(d));
        };

        auto flow = [this](TimeStamp t, ChainId c, DataBlockId d) -> math::Variable
        {
            return solver_.GetVariable("flow_" + std::to_string(t) + "_" + std::to_string(c) + "_" + std::to_string(d));
        };

        std::set<std::tuple<TimeStamp, NodeId, DataBlockId>> buffer_enable_set;
        std::set<std::tuple<TimeStamp, ChainId, DataBlockId>> flow_enable_set;
        for (TimeStamp t = 0; t < num_time_steps_; t++)
            for (NodeId n = 0; n < node_num; n++)
                for (DataBlockId d = 0; d < data_block_num; d++)
                    if (reachable_set.count({t, n, d}) && can_reach_set.count({t, n, d}))
                        buffer_enable_set.insert({t, n, d});

        for (TimeStamp t = 0; t < num_time_steps_ - 1; t++)
            for (ChainId c = 0; c < network_.GetNumChains(); c++)
            {
                NodeId from_node = network_.GetChains()[c].from_node;
                for (DataBlockId d = 0; d < data_block_num; d++)
                    if (reachable_set.count({t, from_node, d}))
                        for (NodeId to_node : network_.GetChains()[c].to_nodes)
                            if (can_reach_set.count({t + 1, to_node, d}))
                                flow_enable_set.insert({t, c, d});
            }

        for (const auto &[t, n, d] : buffer_enable_set)
            solver_.CreateVariable("buffer_" + std::to_string(t) + "_" + std::to_string(n) + "_" + std::to_string(d), var_type, 0, 1);
        for (const auto &[t, c, d] : flow_enable_set)
            solver_.CreateVariable("flow_" + std::to_string(t) + "_" + std::to_string(c) + "_" + std::to_string(d), var_type, 0, 1);

        for (const auto &[name, data_block] : data_blocks_)
        {
            if (!data_block.GetSource().empty())
            { // 添加初始数据存在的约束
                NodeId node = network_.GetNodeIdByArch(data_block.GetSource());
                solver_.AddConstraint(buffer(0, node, data_block.GetId()) == 1);
            }
            if (!data_block.GetDestination().empty())
            { // 添加最终数据存在的约束
                NodeId node = network_.GetNodeIdByArch(data_block.GetDestination());
                solver_.AddConstraint(buffer(num_time_steps_ - 1, node, data_block.GetId()) == 1);
            }
        }

        std::cerr << solver_.GetConstraintCount() << std::endl;

        // 添加流量与缓冲的关联约束
        // 流出约束
        for (const auto &[_, data_block] : data_blocks_)
        {
            DataBlockId d = data_block.GetId();
            switch (data_block.GetDataTag())
            {
            case core::DataBlock::Input:
                // 对于输入数据，流出时同一时刻必须存储于起点
                for (TimeStamp t = 0; t + 1 < num_time_steps_; t++)
                    for (ChainId c = 0; c < network_.GetNumChains(); c++)
                        if (flow_enable_set.count({t, c, d}))
                            solver_.AddConstraint(buffer(t, network_.GetChains()[c].from_node, d) >=
                                                  flow(t, c, d));
                break;
            case core::DataBlock::Output:
                // 对于输出数据，数据在节点要么流出，要么存储于下一时刻，不能释放数据
                for (TimeStamp t = 0; t < num_time_steps_; t++)
                    for (NodeId n = 0; n < network_.GetNumNodes(); n++)
                        if (consume_set[d].count({t, n}))
                            solver_.AddConstraint(buffer(t, n, d) == 1);
                        else if (buffer_enable_set.count({t, n, d}))
                        {
                            if (produce_set[d].count({t, n}))
                                solver_.AddConstraint(buffer(t, n, d) == 1);
                            math::LinearExpression expr;
                            for (ChainId c : network_.GetOutChainIds(n))
                                if (flow_enable_set.count({t, c, d}))
                                    expr += flow(t, c, d);
                            if (buffer_enable_set.count({t + 1, n, d}))
                                expr += buffer(t + 1, n, d);
                            solver_.AddConstraint(expr >= buffer(t, n, d));
                        }
                break;
            default:
                break;
            }
        }

        std::cerr << solver_.GetConstraintCount() << std::endl;
        // 流入约束：
        for (auto &[_, data_block] : data_blocks_)
        {
            DataBlockId d = data_block.GetId();
            switch (data_block.GetDataTag())
            {
            case core::DataBlock::Input:
                // 对于输入数据，流量存储于点时，要么上一时刻通过边流入，要么上一时刻已存储，不能无缘无故出现
                for (TimeStamp t = 0; t < num_time_steps_; t++)
                    for (NodeId n = 0; n < node_num; n++)
                        if (produce_set[d].count({t, n}))
                            solver_.AddConstraint(buffer(t, n, d) == 1);
                        else if (buffer_enable_set.count({t, n, d}))
                        {
                            if (consume_set[d].count({t, n}))
                                solver_.AddConstraint(buffer(t, n, d) == 1);
                            // if (solver_.GetConstraintCount() < 51393 && solver_.GetConstraintCount() > 51388)
                            //     std::cerr << "Node " << n << " at time " << t << " for data block " << d << std::endl;
                            math::LinearExpression inflow;
                            for (ChainId c : network_.GetInChainIds(n))
                                if (flow_enable_set.count({t - 1, c, d}))
                                    inflow += flow(t - 1, c, d);
                            if (buffer_enable_set.count({t - 1, n, d}))
                            {
                                solver_.AddConstraint(inflow + buffer(t - 1, n, d) >=
                                                      buffer(t, n, d));
                                // 剪枝：上一时刻已存储时，当前时刻不需要流入
                                solver_.AddConstraint(inflow + buffer(t - 1, n, d) <= 1);
                                // 剪枝：当前时刻未存储时，当前时刻不需要流入
                                solver_.AddConstraint(inflow <= buffer(t, n, d));
                            }
                            else
                                solver_.AddConstraint(inflow == buffer(t, n, d));
                        }
                break;
            case core::DataBlock::Output:
                // 对于输出数据，流入的数据必须存储于该节点，或与同类（同名字）输出数据合并，不能释放数据
                for (TimeStamp t = 1; t < num_time_steps_; t++)
                    for (NodeId n = 0; n < node_num; n++)
                        if (buffer_enable_set.count({t, n, d}))
                            for (ChainId c : network_.GetInChainIds(n))
                                if (flow_enable_set.count({t - 1, c, d}))
                                    solver_.AddConstraint(flow(t - 1, c, d) <= buffer(t, n, d));
                break;
            default:
                break;
            }
        }

        std::cerr << solver_.GetConstraintCount() << std::endl;
        std::vector<math::Variable> slacks;

        // 添加带宽约束
        for (TimeStamp time = 0; time < num_time_steps_ - 1; time++)
            if (time_slot_[time].first == TimeSlotType::Normal)
            {
                if (schedule_begin_time_ - 1 <= time && time < schedule_end_time_ && (schedule_begin_time_ != time || time_slot_[time - 1].first != TimeSlotType::Normal))
                {
                    size_t slack_id = slacks.size();
                    solver_.CreateVariable("slack_" + std::to_string(slack_id), math::VariableType::Continuous, 0, INFINITY);
                    math::Variable slack = solver_.GetVariable("slack_" + std::to_string(slack_id));
                    slacks.push_back(slack);
                }
                std::vector<TimeStamp> times = {time};
                Tick slot_cycle = time_slot_[time].second;
                for (TimeStamp t = time - 1; t >= 0 && time_slot_[t].first == TimeSlotType::Forward; t--)
                    times.push_back(t), slot_cycle += time_slot_[t].second;
                for (TimeStamp t = time + 1; t < num_time_steps_ && time_slot_[t].first == TimeSlotType::Backward; t++)
                    times.push_back(t), slot_cycle += time_slot_[t].second;

                for (EdgeId e = 0; e < network_.GetNumEdges(); e++)
                {
                    math::LinearExpression flow_size;
                    for (const auto &[_, data_block] : data_blocks_)
                    {
                        DataBlockId d = data_block.GetId();
                        for (ChainId c : network_.GetChainIdsContainEdge(e))
                            for (TimeStamp t : times)
                                if (flow_enable_set.count({t, c, d}))
                                    flow_size += flow(t, c, d) * data_block.GetSize();
                    }
                    if (schedule_begin_time_ - 1 <= time && time < schedule_end_time_)
                        solver_.AddConstraint(flow_size <= network_.GetEdges()[e].bandwidth_bytes_per_cycle * (slacks.back() + slot_cycle));
                    else
                        solver_.AddConstraint(flow_size <= network_.GetEdges()[e].bandwidth_bytes_per_cycle * slot_cycle);
                }
            }

        std::cerr << solver_.GetConstraintCount() << std::endl;

        // 添加节点缓冲约束
        for (TimeStamp t = 0; t < num_time_steps_; t++)
            for (NodeId n = 0; n < node_num; n++)
            {
                math::LinearExpression buffer_sum = -double(shared_memory_byte[{t, n}]);
                for (const auto &[_, data_block] : data_blocks_)
                {
                    DataBlockId d = data_block.GetId();
                    if (buffer_enable_set.count({t, n, d}))
                        buffer_sum += buffer(t, n, d) * data_block.GetSize();
                }
                solver_.AddConstraint(buffer_sum <= network_.GetArch(n)->GetBufferSizeBytes());
            }

        std::cerr << solver_.GetConstraintCount() << std::endl;

        // 目标函数：最小化
        math::LinearExpression obj;
        for (math::Variable &slack : slacks)
            obj += slack * 10000000;
        for (TimeStamp t = 0; t < num_time_steps_ - 1; t++)
            for (ChainId c = 0; c < network_.GetNumChains(); c++)
                for (const auto &[_, data_block] : data_blocks_)
                {
                    DataBlockId d = data_block.GetId();
                    if (flow_enable_set.count({t, c, d}))
                        obj += flow(t, c, d) * data_block.GetSize() * 8 * network_.GetChainEnergyPerBit(c);
                }

        solver_.SetObjective(obj, math::ILPSolver::ObjectiveSense::Minimize);

        solver_.SetIntermediateResultPath(intermediate_result_path_);
        solver_.Solve(time_limit);

        Tick total_time = 0;
        Energy total_noc_energy = 0;
        Energy total_dram_energy = 0;
        size_t slot_num = 0;

        for (TimeStamp time = 0; time < num_time_steps_ - 1; time++)
            if (schedule_begin_time_ <= time && time < schedule_end_time_ && time_slot_[time].first == TimeSlotType::Normal)
                total_time += time_slot_[time].second + solver_.GetVariable("slack_" + std::to_string(slot_num)).GetValue();

        uint64_t total_hops_bytes = 0;
        uint64_t total_bytes = 0;
        std::map<DataBlockId, uint64_t> data_block_hops;
        std::vector<Size> bytes_hops = {0};
        for (const auto &[_, data_block] : data_blocks_)
        {
            DataBlockId d = data_block.GetId();
            total_bytes += data_block.GetSize();
            bytes_hops[0] += data_block.GetSize();
            for (TimeStamp time = 0; time < num_time_steps_ - 1; time++)
                for (ChainId c = 0; c < network_.GetNumChains(); c++)
                    if (flow_enable_set.count({time, c, d}) && solver_.GetVariable("flow_" + std::to_string(time) + "_" + std::to_string(c) + "_" + std::to_string(d)).GetValue() > 0.5)
                    {
                        const std::vector<EdgeId> &chain_edges = network_.GetEdgeIdsInChain(c);
                        bytes_hops[data_block_hops[d]] -= data_block.GetSize();
                        data_block_hops[d] += chain_edges.size();
                        while(bytes_hops.size() <= data_block_hops[d])
                            bytes_hops.push_back(0);
                        bytes_hops[data_block_hops[d]] += data_block.GetSize();
                        total_hops_bytes += chain_edges.size() * data_block.GetSize();
                        for (EdgeId e : chain_edges)
                        {
                            const core::Edge &edge = network_.GetEdges()[e];
                            if (edge.energy_pj_per_bit > 10)
                                total_dram_energy += data_block.GetSize() * 8 * edge.energy_pj_per_bit;
                            else
                                total_noc_energy += data_block.GetSize() * 8 * edge.energy_pj_per_bit;
                        }
                    }
        }

        if (!data_analysis_path_.empty())
        {
            std::ofstream ofs(data_analysis_path_);
            if (ofs.is_open())
            {
                ofs << "Total Time: " << total_time << std::endl;
                ofs << "Total NOC Energy: " << total_noc_energy << std::endl;
                ofs << "Total DRAM Energy: " << total_dram_energy << std::endl;
                ofs << "Total Bytes: " << total_bytes << std::endl;
                ofs << "Total Hops Bytes: " << total_hops_bytes << std::endl;
                for (size_t i = 0; i < bytes_hops.size(); i++)
                    ofs << "Bytes with " << i << " hops: " << bytes_hops[i] << std::endl;
            }
        }

        return std::make_tuple(total_time, total_noc_energy, total_dram_energy);
    }

    std::tuple<Tick, Energy, Energy> DataflowScheduler::EstimateDataflowOverhead(OverheadEstimateType type, Second time_limit)
    {
        math::VariableType var_type = (type == Relaxed) ? math::VariableType::Continuous : math::VariableType::Binary;
        return SolveDataflowILP(var_type, time_limit);
    }
} // namespace planner
