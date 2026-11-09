#include <cstring>
#include <stdexcept>

#include "data_distributor.h"
#include "data_distributor/data_distributors.h"

namespace planner
{
    std::shared_ptr<DataDistributor> CreateDataDistributor(cJSON *config_json)
    {
        std::string data_distributor_type;
        cJSON *args_json = nullptr;
        for (cJSON *arg_parse = config_json->child; arg_parse; arg_parse = arg_parse->next)
            if (strcmp(arg_parse->string, "type") == 0)
                data_distributor_type = arg_parse->valuestring;
            else if (strcmp(arg_parse->string, "args") == 0)
                args_json = arg_parse;
        if (data_distributor_type == "GreedByBandwidth")
            return std::make_shared<GreedByBandwidth>(args_json);
        else
            throw std::runtime_error("Data distribution strategy not specified in config JSON");
    }
} // namespace planner