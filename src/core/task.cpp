#include <stdexcept>
#include <string>

#include "task/tasks.h"
#include "task.h"

namespace core
{
    TaskImpl::TaskImpl(cJSON *json) : shared_memory_byte_(0)
    {
        if (json == nullptr)
            return;
        name_ = json->string ? json->string : "";
        for (cJSON *item = json->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "inputs")
                for (cJSON *data_block = item->child; data_block; data_block = data_block->next)
                    input_block_names_.push_back(data_block->valuestring);
            else if (key == "weights")
                for (cJSON *data_block = item->child; data_block; data_block = data_block->next)
                    weight_block_names_.push_back(data_block->valuestring);
            else if (key == "outputs")
                for (cJSON *data_block = item->child; data_block; data_block = data_block->next)
                    output_block_names_.push_back(data_block->valuestring);
            else if (key == "shared_memory_byte")
                shared_memory_byte_ = item->valueint;
        }
    }

    Task::Task(cJSON *json) : impl_(nullptr), scheduled_time_(INVALID_TIME), scheduled_node_(INVALID_NODE)
    {
        std::string type = cJSON_GetObjectItem(json, "type")->valuestring;
        if (type == "Attention")
            impl_ = std::make_shared<Attention>(json);
        else if (type == "Gemm")
            impl_ = std::make_shared<Gemm>(json);
        else if (type == "Conv")
            impl_ = std::make_shared<Conv>(json);
        else if (type == "Collect")
            impl_ = std::make_shared<Collect>(json);
        else
            throw std::runtime_error("Invalid task type: " + type);
    }
}