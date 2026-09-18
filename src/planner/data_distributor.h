#ifndef DATA_DISTRIBUTOR_H
#define DATA_DISTRIBUTOR_H

#include <map>

#include "core/data_block.h"
#include "core/network.h"
#include "core/task.h"
#include "util/cJSON.h"

namespace planner
{
    enum class DataDistributionStrategy
    {
        GreedByBandwidth,
    };

    class DataDistributor
    {
    public:
        DataDistributor() {}
        virtual void DistributeDataBlocks(const core::Network &network, std::map<DataBlockName, core::DataBlock> &data_blocks) = 0;
    };

    DataDistributor *CreateDataDistributor(cJSON *config_json);
} // namespace planner

#endif /* DATA_DISTRIBUTOR_H */