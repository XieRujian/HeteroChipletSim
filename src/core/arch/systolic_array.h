#ifndef SYSTOLIC_ARRAY_H
#define SYSTOLIC_ARRAY_H

#include "../arch.h"

namespace core
{
    class SystolicArray : public ArchImpl
    {
        uint64_t row_;
        uint64_t col_;
        uint64_t num_array_;
        double frequency_ghz_;

        uint64_t gb_row_width_bytes_ = 64;

        Tick EvaluateGemmTick(uint64_t m, uint64_t n, uint64_t k) const;
        Energy EvaluateGemmDynamicEnergy(uint64_t m, uint64_t n, uint64_t k, Size dt_bytes) const;

    public:
        SystolicArray(const std::map<std::string, std::string> &config_ini);

        EvaluationResult Evaluate(const Gemm &gemm) const override;
        EvaluationResult Evaluate(const Conv &conv) const override;
        EvaluationResult Evaluate(const Attention &attention) const override;
        bool CanSchedule(const Task &) const override { return true; }
    };

} // namespace core

#endif // SYSTOLIC_ARRAY_H