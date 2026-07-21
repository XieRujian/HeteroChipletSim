#ifndef ATTENTION_H
#define ATTENTION_H

#include "../task.h"

namespace core
{
    class Attention : public TaskImpl
    {
    public:
        uint64_t seq_length;
        uint64_t kv_size;
        uint64_t head_num;
        uint64_t head_dim;

    public:
        Attention(cJSON *json);
        Attention(uint64_t seq_length, uint64_t kv_size, uint64_t head_num, uint64_t head_dim) : TaskImpl(nullptr), seq_length(seq_length), kv_size(kv_size), head_num(head_num), head_dim(head_dim) {}
        EvaluationResult accept(const Arch &) const override;
    };
}

#endif // ATTENTION_H