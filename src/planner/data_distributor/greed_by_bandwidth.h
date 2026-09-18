#ifndef UNIFORM_DISTRIBUTE_H
#define UNIFORM_DISTRIBUTE_H

#include "planner/data_distributor.h"

namespace planner
{
    class GreedByBandwidth : public DataDistributor
    {
        struct DistributeSequence
        {
            std::vector<DataBlockName> data_blocks;
            std::vector<ArchName> arch_priority;
            enum
            {
                Src,
                Dst
            } distribute_item;
        };

        std::vector<DistributeSequence> sequences_;

    public:
        GreedByBandwidth(cJSON *config_json);
        void DistributeDataBlocks(const core::Network &network, std::map<DataBlockName, core::DataBlock> &data_blocks) override;
    };
} // namespace planner

#endif // UNIFORM_DISTRIBUTE_H