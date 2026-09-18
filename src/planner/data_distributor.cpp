#include <cstring>
#include <stdexcept>

#include "data_distributor.h"
#include "data_distributor/data_distributors.h"

namespace planner
{
    DataDistributor *CreateDataDistributor(cJSON *config_json)
    {
        for (cJSON *arg_parse = config_json->child; arg_parse; arg_parse = arg_parse->next)
            if (strcmp(arg_parse->string, "strategy") == 0)
            {
                const std::string &strategy_str(arg_parse->valuestring);
                if (strategy_str == "GreedByBandwidth")
                    return new GreedByBandwidth(config_json);
                else
                    throw std::runtime_error("Invalid data distribution strategy: " + strategy_str);
            }
        throw std::runtime_error("Data distribution strategy not specified in config JSON");
    }
} // namespace planner