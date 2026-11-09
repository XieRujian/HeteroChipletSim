#ifndef DATA_BLOCK_H
#define DATA_BLOCK_H

#include <string>
#include <vector>

#include "util/cJSON.h"
#include "util/common.h"

namespace core
{
    class DataBlock
    {
    public:
        enum class DataTag
        {
            InAct,  // 输入激活
            Weight, // 权重数据
            OutAct  // 输出激活
        };
        enum class DataType
        {
            INT8,
            FP16,
            FP32
        };

    private:
        DataBlockName name_;
        DataBlockId id_;
        DataTag tag_;
        DataType dtype_;
        std::vector<uint64_t> shape_;
        NodeId source_;
        NodeId destination_;

    public:
        DataBlock(cJSON *json);

        const DataBlockName &GetName() const noexcept { return name_; }
        DataBlockId GetId() const noexcept { return id_; }
        DataTag GetDataTag() const noexcept { return tag_; }
        DataType GetDataType() const noexcept { return dtype_; }
        const NodeId &GetDestination() const noexcept { return destination_; }
        const NodeId &GetSource() const noexcept { return source_; }
        void SetSource(const NodeId &source) { source_ = source; }
        void SetDestination(const NodeId &destination) { destination_ = destination; }
        Size GetSize() const;

        void SetId(DataBlockId id) noexcept { id_ = id; }
    };
}

#endif // DATA_BLOCK_H