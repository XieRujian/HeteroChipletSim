#include <stdexcept>

#include "data_block.h"

namespace core
{
    DataBlock::DataBlock(cJSON *json)
    {
        name_ = json->string ? json->string : "";
        for (cJSON *item = json->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "shape")
                for (cJSON *dim = item->child; dim; dim = dim->next)
                    shape_.push_back(dim->valueint);
            else if (key == "source")
                source_ = item->valuestring;
            else if (key == "tag")
            {
                if (std::string(item->valuestring) == "Input")
                    tag_ = DataTag::Input;
                else if (std::string(item->valuestring) == "Output")
                    tag_ = DataTag::Output;
            }
            else if (key == "dtype")
            {
                if (std::string(item->valuestring) == "int8")
                    dtype_ = DataType::INT8;
                else if (std::string(item->valuestring) == "fp16")
                    dtype_ = DataType::FP16;
                else if (std::string(item->valuestring) == "fp32")
                    dtype_ = DataType::FP32;
                else
                    throw std::runtime_error("Unsupported data type: " + std::string(item->valuestring));
            }
            else if (key == "destination")
                destination_ = item->valuestring;
        }
    }

    Size DataBlock::GetSize() const
    {
        Size size = 0;
        switch (dtype_)
        {
        case DataType::INT8:
            size = 1;
            break;
        case DataType::FP16:
            size = 2;
            break;
        case DataType::FP32:
            size = 4;
            break;
        default:
            throw std::runtime_error("Unsupported data type");
        }
        for (uint64_t dim : shape_)
            size *= dim;
        return size;
    }
}