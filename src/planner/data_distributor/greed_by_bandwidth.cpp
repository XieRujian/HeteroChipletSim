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
        for (cJSON *seq_json = config_json->child; seq_json; seq_json = seq_json->next)
        {
            DistributeSequence seq;
            for (cJSON *item = seq_json->child; item; item = item->next)
            {
                const std::string &key(item->string);
                if (key == "data_blocks")
                {
                    for (cJSON *data_block_item = item->child; data_block_item; data_block_item = data_block_item->next)
                        seq.data_blocks.push_back(data_block_item->valuestring);
                }
                else if (key == "arch_priority")
                {
                    for (cJSON *arch_item = item->child; arch_item; arch_item = arch_item->next)
                        seq.arch_priority.push_back(arch_item->valuestring);
                }
                else if (key == "distribute_item")
                {
                    std::string distribute_item_str(item->valuestring);
                    if (distribute_item_str == "Src")
                        seq.distribute_item = DistributeSequence::Src;
                    else if (distribute_item_str == "Dst")
                        seq.distribute_item = DistributeSequence::Dst;
                    else
                        throw std::runtime_error("Invalid distribute item: " + distribute_item_str);
                }
            }
            sequences_.push_back(seq);
        }
    }

    void GreedByBandwidth::DistributeDataBlocks(const core::Network &network, std::map<DataBlockName, core::DataBlock> &data_blocks)
    {
        std::vector<std::vector<double>> bandwidths(network.GetNumNodes());
        for (const auto &edge : network.GetEdges())
            if (network.GetArch(edge.to_node)->GetType() != core::ArchType::DRAM)
                bandwidths[edge.from_node].push_back(edge.bandwidth_bytes_per_tick);

        auto check_tick_limit = [&](Tick limit) -> bool
        {
            std::vector<std::vector<Tick>> link_ticks;
            for (NodeId n = 0; n < network.GetNumNodes(); n++)
                link_ticks.emplace_back(bandwidths[n].size(), 0);
            for (const auto &seq : sequences_)
            {
                uint32_t arch_index = 0;
                for (const auto &data_block_name : seq.data_blocks)
                {
                    const auto &data_block = data_blocks.at(data_block_name);
                    Size size_bytes = data_block.GetSize();
                    bool success_distribute = false;

                    while (arch_index < seq.arch_priority.size())
                    {
                        const auto &arch_name = seq.arch_priority[arch_index];
                        NodeId node_id = network.GetNodeIdByArch(arch_name);
                        for (size_t link_index = 0; link_index < bandwidths[node_id].size(); link_index++)
                        {
                            double bandwidth_bytes_per_tick = bandwidths[node_id][link_index];
                            Tick ticks_needed = std::ceil(size_bytes / bandwidth_bytes_per_tick);
                            if (link_ticks[node_id][link_index] + ticks_needed <= limit)
                            {
                                link_ticks[node_id][link_index] += ticks_needed;
                                if (seq.distribute_item == DistributeSequence::Src)
                                    data_blocks.at(data_block_name).SetSource(arch_name);
                                else
                                    data_blocks.at(data_block_name).SetDestination(arch_name);
                                success_distribute = true;
                                break;
                            }
                        }
                        if (success_distribute)
                            break;
                        arch_index++;
                    }
                    if (!success_distribute)
                        return false; // No available architecture for this data block within the tick limit
                }
            }
            return true; // All data blocks distributed within the tick limit
        };

        Tick l = 0, r = std::numeric_limits<Tick>::max();
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