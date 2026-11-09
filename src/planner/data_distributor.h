#ifndef DATA_DISTRIBUTOR_H
#define DATA_DISTRIBUTOR_H

#include <map>
#include <memory>
#include <vector>

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
        virtual ~DataDistributor() = default;
        virtual void DistributeDataBlocks(const std::map<ArchName, core::Arch> &library_, const core::Network &network, std::map<DataBlockName, core::DataBlock> &data_blocks, Tick compute_tick) = 0;
    };

    std::shared_ptr<DataDistributor> CreateDataDistributor(cJSON *config_json);
} // namespace planner

#endif /* DATA_DISTRIBUTOR_H */