#ifndef STORAGE_H
#define STORAGE_H

#include "../arch.h"

namespace core
{
    class Storage : public ArchImpl
    {
    public:
        Storage() : ArchImpl(ArchType::DRAM, 0xffffffffffff) {}

        EvaluationResult Evaluate(const Gemm &) const override { return {0xffffffffffff, 0}; }
        EvaluationResult Evaluate(const Conv &) const override { return {0xffffffffffff, 0}; }
        EvaluationResult Evaluate(const Attention &) const override { return {0xffffffffffff, 0}; }
        bool CanSchedule(const Task &) const override { return false; }
    };

} // namespace core

#endif // STORAGE_H