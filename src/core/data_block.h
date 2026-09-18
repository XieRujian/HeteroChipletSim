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
            Input, // 输入数据
            Output // 输出数据
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
        ArchName source_;
        ArchName destination_;

    public:
        DataBlock(cJSON *json);

        const DataBlockName &GetName() const noexcept { return name_; }
        DataBlockId GetId() const noexcept { return id_; }
        DataTag GetDataTag() const noexcept { return tag_; }
        DataType GetDataType() const noexcept { return dtype_; }
        const ArchName &GetDestination() const noexcept { return destination_; }
        const ArchName &GetSource() const noexcept { return source_; }
        void SetSource(const ArchName &source) { source_ = source; }
        void SetDestination(const ArchName &destination) { destination_ = destination; }
        Size GetSize() const;

        void SetId(DataBlockId id) noexcept { id_ = id; }
    };
}

#endif // DATA_BLOCK_H