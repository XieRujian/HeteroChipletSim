#ifndef TASK_H
#define TASK_H

#include <memory>
#include <string>
#include <vector>

#include "util/cJSON.h"
#include "util/common.h"

namespace core
{
    class Arch;
    class DataBlock;
    struct EvaluationResult
    {
        Tick execution_ticks;
        Energy energy_pj;
    };

    class TaskImpl
    {
        TaskName name_;
        Size shared_memory_byte_;
        std::vector<DataBlockName> input_block_names_;
        std::vector<DataBlockName> weight_block_names_;
        std::vector<DataBlockName> output_block_names_;

    public:
        TaskImpl(cJSON *json);

        virtual EvaluationResult accept(const Arch &arch) const = 0;

        const TaskName &GetName() const noexcept { return name_; }
        Size GetSharedMemoryByte() const noexcept { return shared_memory_byte_; }
        const std::vector<DataBlockName> &GetInputBlockNames() const noexcept { return input_block_names_; }
        const std::vector<DataBlockName> &GetWeightBlockNames() const noexcept { return weight_block_names_; }
        const std::vector<DataBlockName> &GetOutputBlockNames() const noexcept { return output_block_names_; }
    };

    class Task
    {
        std::shared_ptr<TaskImpl> impl_;

        TimeStamp scheduled_time_;
        NodeId scheduled_node_;

    public:
        Task() : scheduled_time_(INVALID_TIME), scheduled_node_(INVALID_NODE) {}
        Task(const Task &other) = default;
        Task(Task &&other) = default;
        Task(cJSON *json);

        Task &operator=(const Task &other) = default;
        Task &operator=(Task &&other) = default;
        TaskImpl *operator->() const noexcept { return impl_.get(); }

        bool operator==(const Task &other) const noexcept { return impl_ == other.impl_; }
        bool empty() const noexcept { return impl_ == nullptr; }

        TimeStamp GetScheduledTime() const noexcept { return scheduled_time_; }
        NodeId GetScheduledNode() const noexcept { return scheduled_node_; }
        void SetScheduledTime(TimeStamp time) noexcept { scheduled_time_ = time; }
        void SetScheduledNode(NodeId node) noexcept { scheduled_node_ = node; }
    };
}

#endif // TASK_H