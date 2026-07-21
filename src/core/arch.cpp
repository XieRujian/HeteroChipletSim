#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "arch.h"
#include "arch/archs.h"
#include "data_block.h"
#include "task.h"

namespace core
{
    ArchImpl::ArchImpl(const std::map<std::string, std::string> &config_ini)
        : buffer_capacity_kb_(std::stoull(config_ini.at("buffer_size"))) {}

    void ArchImpl::AddPreStoreDataBlock(const DataBlockName &data_block_name)
    {
        pre_store_data_blocks_.insert(data_block_name);
    }

    EvaluationResult Arch::Evaluate(const Task &task) const { return task->accept(*this); }

    Arch::Arch(const ArchName &name, const std::map<std::string, std::string> &config_ini)
        : name_(name)
    {
        std::string type = config_ini.at("type");
        if (type == "SystolicArray")
            impl_ = std::make_shared<core::SystolicArray>(config_ini);
        else if (type == "PIM")
            impl_ = std::make_shared<core::PIM>(config_ini);
        else if (type == "Storage")
            impl_ = std::make_shared<core::Storage>();
        else
            throw std::runtime_error("Invalid architecture type: " + type);
    }
}