#include <cmath>
#include <limits>
#include <stdexcept>

#include "core/arch.h"
#include "core/data_block.h"

#include "greed_by_bandwidth.h"

namespace planner
{
    GreedByBandwidth::GreedByBandwidth(cJSON *config_json)
    {
        for (cJSON *item = config_json->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "capacity_by_pim_num")
            {
                for (cJSON *pim_num_parse = item->child; pim_num_parse; pim_num_parse = pim_num_parse->next)
                {
                    Size capacity_bytes = pim_num_parse->valueint;
                    capacity_by_pim_num_.push_back(capacity_bytes);
                }
            }
        }
    }

    void GreedByBandwidth::DistributeDataBlocks(const std::map<ArchName, core::Arch> &library_, const core::Network &network, std::map<DataBlockName, core::DataBlock> &data_blocks, Tick compute_tick)
    {
        std::vector<std::vector<double>> bandwidths(network.GetNumNodes());
        uint32_t pim_num = 0;
        for (NodeId n = 0; n < network.GetNumNodes(); n++)
            if (library_.at(network.GetArch(n))->GetType() != core::ArchType::PIM)
                pim_num++;
        for (const auto &edge : network.GetEdges())
            if (library_.at(network.GetArch(edge.to_node))->GetType() != core::ArchType::DRAM)
                bandwidths[edge.from_node].push_back(edge.bandwidth_bytes_per_tick);
        Size pim_buffer = capacity_by_pim_num_[pim_num];

        auto check_tick_limit = [&](Tick limit) -> bool
        {
            std::vector<std::vector<Tick>> link_ticks_inact_;
            std::vector<std::vector<Tick>> link_ticks_outact_;
            std::vector<Size> capacity(network.GetNumNodes(), 0);
            // std::vector<>
            for (NodeId n = 0; n < network.GetNumNodes(); n++)
            {
                link_ticks_inact_.emplace_back(bandwidths[n].size(), 0);
                link_ticks_outact_.emplace_back(bandwidths[n].size(), 0);
            }

            NodeId last_node = 0;
            for (const auto &[data_block_name, data_block] : data_blocks)
                if (data_block.GetDataTag() == core::DataBlock::DataTag::InAct)
                {
                    Size size_bytes = data_block.GetSize();
                    bool success_distribute = false;

                    for (NodeId n_offset = 1; n_offset <= network.GetNumNodes(); n_offset++)
                    {
                        NodeId n = (last_node + n_offset) % network.GetNumNodes();
                        if (library_.at(network.GetArch(n))->GetType() == core::ArchType::PIM && size_bytes + capacity[n] <= pim_buffer)
                        {
                            for (size_t link_index = 0; link_index < bandwidths[n].size(); link_index++)
                            {
                                double bandwidth_bytes_per_tick = bandwidths[n][link_index];
                                Tick ticks_needed = std::ceil(size_bytes / bandwidth_bytes_per_tick);
                                if (link_ticks_inact_[n][link_index] + ticks_needed <= limit)
                                {
                                    capacity[n] += size_bytes;
                                    link_ticks_inact_[n][link_index] += ticks_needed;
                                    data_blocks.at(data_block_name).SetSource(n);
                                    success_distribute = true;
                                    last_node = n;
                                    break;
                                }
                            }
                            break;
                        }
                    }

                    if (success_distribute)
                        continue;

                    for (NodeId n_offset = 1; n_offset <= network.GetNumNodes(); n_offset++)
                    {
                        NodeId n = (last_node + n_offset) % network.GetNumNodes();
                        if (library_.at(network.GetArch(n))->GetType() == core::ArchType::DRAM)
                        {
                            for (size_t link_index = 0; link_index < bandwidths[n].size(); link_index++)
                            {
                                double bandwidth_bytes_per_tick = bandwidths[n][link_index];
                                Tick ticks_needed = std::ceil(size_bytes / bandwidth_bytes_per_tick);
                                if (link_ticks_inact_[n][link_index] + ticks_needed <= limit)
                                {
                                    link_ticks_inact_[n][link_index] += ticks_needed;
                                    data_blocks.at(data_block_name).SetSource(n);
                                    success_distribute = true;
                                    last_node = n;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if (!success_distribute)
                        return false; // No available architecture for this data block within the tick limit
                }
                else if (data_block.GetDataTag() == core::DataBlock::DataTag::OutAct)
                {
                    Size size_bytes = data_block.GetSize();
                    bool success_distribute = false;

                    for (NodeId n_offset = 0; n_offset < network.GetNumNodes(); n_offset++)
                    {
                        NodeId n = (last_node + n_offset) % network.GetNumNodes();
                        if (library_.at(network.GetArch(n))->GetType() == core::ArchType::PIM && size_bytes + capacity[n] <= pim_buffer)
                        {
                            for (size_t link_index = 0; link_index < bandwidths[n].size(); link_index++)
                            {
                                double bandwidth_bytes_per_tick = bandwidths[n][link_index];
                                Tick ticks_needed = std::ceil(size_bytes / bandwidth_bytes_per_tick);
                                if (link_ticks_outact_[n][link_index] + ticks_needed <= limit)
                                {
                                    capacity[n] += size_bytes;
                                    link_ticks_outact_[n][link_index] += ticks_needed;
                                    data_blocks.at(data_block_name).SetSource(n);
                                    success_distribute = true;
                                    last_node = n;
                                    break;
                                }
                            }
                            break;
                        }
                    }

                    if (success_distribute)
                        continue;

                    for (NodeId n_offset = 0; n_offset < network.GetNumNodes(); n_offset++)
                    {
                        NodeId n = (last_node + n_offset) % network.GetNumNodes();
                        if (library_.at(network.GetArch(n))->GetType() == core::ArchType::DRAM)
                        {
                            for (size_t link_index = 0; link_index < bandwidths[n].size(); link_index++)
                            {
                                double bandwidth_bytes_per_tick = bandwidths[n][link_index];
                                Tick ticks_needed = std::ceil(size_bytes / bandwidth_bytes_per_tick);
                                if (link_ticks_outact_[n][link_index] + ticks_needed <= limit)
                                {
                                    link_ticks_outact_[n][link_index] += ticks_needed;
                                    data_blocks.at(data_block_name).SetSource(n);
                                    success_distribute = true;
                                    last_node = n;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if (!success_distribute)
                        return false; // No available architecture for this data block within the tick limit
                }

            for (const auto &[data_block_name, data_block] : data_blocks)
                if (data_block.GetDataTag() == core::DataBlock::DataTag::Weight)
                {
                    Size size_bytes = data_block.GetSize();
                    bool success_distribute = false;

                    for (NodeId n = 0; n < network.GetNumNodes(); n++)
                        if (library_.at(network.GetArch(n))->GetType() == core::ArchType::PIM && size_bytes + capacity[n] <= pim_buffer)
                        {
                            for (size_t link_index = 0; link_index < bandwidths[n].size(); link_index++)
                            {
                                double bandwidth_bytes_per_tick = bandwidths[n][link_index];
                                Tick ticks_needed = std::ceil(size_bytes / bandwidth_bytes_per_tick);
                                if (link_ticks_inact_[n][link_index] + ticks_needed <= limit)
                                {
                                    capacity[n] += size_bytes;
                                    link_ticks_inact_[n][link_index] += ticks_needed;
                                    data_blocks.at(data_block_name).SetSource(n);
                                    success_distribute = true;
                                    break;
                                }
                            }
                            break;
                        }

                    for (NodeId n = 0; n < network.GetNumNodes(); n++)
                        if (library_.at(network.GetArch(n))->GetType() == core::ArchType::DRAM)
                        {
                            for (size_t link_index = 0; link_index < bandwidths[n].size(); link_index++)
                            {
                                double bandwidth_bytes_per_tick = bandwidths[n][link_index];
                                Tick ticks_needed = std::ceil(size_bytes / bandwidth_bytes_per_tick);
                                if (link_ticks_inact_[n][link_index] + ticks_needed <= limit)
                                {
                                    link_ticks_inact_[n][link_index] += ticks_needed;
                                    data_blocks.at(data_block_name).SetSource(n);
                                    success_distribute = true;
                                    break;
                                }
                            }
                            break;
                        }
                    if (!success_distribute)
                        return false; // No available architecture for this data block within the tick limit
                }
            return true; // All data blocks distributed within the tick limit
        };

        Tick l = compute_tick, r = std::numeric_limits<Tick>::max();
        while (l < r)
        {
            Tick mid = l + (r - l) / 2;
            if (check_tick_limit(mid))
                r = mid;
            else
                l = mid + 1;
        }
    }
}