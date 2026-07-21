#ifndef CONV_H
#define CONV_H

#include "../task.h"

namespace core
{
    class Conv : public TaskImpl
    {
    public:
        uint64_t batch_size;
        uint64_t in_channels;
        uint64_t out_channels;
        uint64_t in_height;
        uint64_t in_width;
        uint64_t kernel_height;
        uint64_t kernel_width;
        uint64_t stride_height;
        uint64_t stride_width;
        uint64_t padding_height;
        uint64_t padding_width;

    public:
        Conv(cJSON *json);
        EvaluationResult accept(const Arch &arch) const override;
    };
}

#endif