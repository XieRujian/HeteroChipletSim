#ifndef UNIFORM_DISTRIBUTE_H
#define UNIFORM_DISTRIBUTE_H

#include "planner/data_distributor.h"

namespace planner
{
    class GreedByBandwidth : public DataDistributor
    {
        std::vector<Size> capacity_by_pim_num_;

    public:
        GreedByBandwidth(cJSON *config_json);
        void DistributeDataBlocks(const std::map<ArchName, core::Arch> &library_, const core::Network &network, std::map<DataBlockName, core::DataBlock> &data_blocks, Tick compute_tick) override;
    };
} // namespace planner

#endif // UNIFORM_DISTRIBUTE_H