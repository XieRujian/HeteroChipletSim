#ifndef PIM_H
#define PIM_H

#include "../arch.h"

namespace core
{
    class PIM : public ArchImpl
    {
        uint64_t bank_num_;
        uint64_t rows_per_subarray_;
        uint64_t bit_per_row_;
        uint64_t mac_per_bank_;
        uint64_t global_bandwidth_bytes_per_cycle_;
        uint64_t local_rf_depth_;
        double frequency_ghz_;

        Tick EvaluateGemmTick(uint64_t m, uint64_t n, uint64_t k, Size dt_bytes) const;
        Energy EvaluateGemmDynamicEnergy(uint64_t m, uint64_t n, uint64_t k, Size dt_bytes) const;

    public:
        PIM(const std::map<std::string, std::string> &config_ini);

        EvaluationResult Evaluate(const Gemm &) const override;
        EvaluationResult Evaluate(const Conv &) const override;
        EvaluationResult Evaluate(const Attention &) const override;
        bool CanSchedule(const Task &task) const override;
    };

} // namespace core

#endif // PIM_H