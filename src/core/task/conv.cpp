#include "util/cJSON.h"

#include "../arch.h"
#include "conv.h"

namespace core
{
    Conv::Conv(cJSON *json) : TaskImpl(json)
    {
        for (cJSON *item = json->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "batch_size")
                batch_size = item->valueint;
            else if (key == "in_channels")
                in_channels = item->valueint;
            else if (key == "out_channels")
                out_channels = item->valueint;
            else if (key == "in_height")
                in_height = item->valueint;
            else if (key == "in_width")
                in_width = item->valueint;
            else if (key == "kernel_height")
                kernel_height = item->valueint;
            else if (key == "kernel_width")
                kernel_width = item->valueint;
            else if (key == "stride_height")
                stride_height = item->valueint;
            else if (key == "stride_width")
                stride_width = item->valueint;
            else if (key == "padding_height")
                padding_height = item->valueint;
            else if (key == "padding_width")
                padding_width = item->valueint;
        }
    }

    EvaluationResult Conv::accept(const Arch &arch) const { return arch->Evaluate(*this); }
}