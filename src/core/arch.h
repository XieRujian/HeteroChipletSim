#ifndef ARCH_H
#define ARCH_H

#include <cstdint>
#include <map>
#include <memory>
#include <set>

#include "util/common.h"

#include "arch/arch_types.h"
#include "task.h"
#include "task/tasks.h"

namespace core
{
    class ArchImpl
    {
    protected:
        ArchType type_;
        uint64_t buffer_capacity_kb_;
        std::set<DataBlockName> pre_store_data_blocks_;

    public:
        ArchImpl(ArchType type, const std::map<std::string, std::string> &config_ini);
        ArchImpl(ArchType type, uint64_t buffer_capacity_kb) : type_(type), buffer_capacity_kb_(buffer_capacity_kb) {}

        virtual EvaluationResult Evaluate(const Gemm &gemm) const = 0;
        virtual EvaluationResult Evaluate(const Conv &conv) const = 0;
        virtual EvaluationResult Evaluate(const Attention &attention) const = 0;
        virtual bool CanSchedule(const Task &task) const = 0;

        void AddPreStoreDataBlock(const DataBlockName &data_block_name);
        bool CheckPreStoreDataBlock(const DataBlockName &data_block_name) const { return pre_store_data_blocks_.count(data_block_name); }
        // uint64_t GetPreStoreSizeBytes() const noexcept { return pre_store_size_bytes_; }

        ArchType GetType() const noexcept { return type_; }
        uint64_t GetBufferSizeBytes() const { return buffer_capacity_kb_ * 1024; }
    };

    class Arch
    {
        ArchName name_;
        std::shared_ptr<ArchImpl> impl_;

    public:
        Arch() = default;
        Arch(const Arch &other) = default;
        Arch(Arch &&other) = default;
        Arch(const ArchName &name, const std::map<std::string, std::string> &config_ini);

        const ArchName &GetName() const noexcept { return name_; }
        Arch &operator=(const Arch &other) = default;
        Arch &operator=(Arch &&other) = default;
        ArchImpl *operator->() const noexcept { return impl_.get(); }

        EvaluationResult Evaluate(const Task &task) const;
    };
}

#endif // ARCH_H