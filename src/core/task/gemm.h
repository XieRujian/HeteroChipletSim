#ifndef TASK_GEMM_H
#define TASK_GEMM_H

#include "../task.h"

namespace core
{
    class Gemm : public TaskImpl
    {
    public:
        uint64_t m;
        uint64_t n;
        uint64_t k;

    public:
        Gemm(cJSON *json);
        Gemm(uint64_t m, uint64_t n, uint64_t k): TaskImpl(nullptr), m(m), n(n), k(k) {}
        EvaluationResult accept(const Arch &arch) const override;
    };
}

#endif // TASK_GEMM_H