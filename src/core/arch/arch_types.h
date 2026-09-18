#ifndef CORE_ARCH_ARCH_TYPES_H
#define CORE_ARCH_ARCH_TYPES_H

#include <map>
#include <string>

namespace core
{
    enum class ArchType
    {
        DRAM,
        SA,
        PIM,
    };

    inline std::map<std::string, ArchType> ArchTypeMap = {
        {"DRAM", ArchType::DRAM},
        {"SA", ArchType::SA},
        {"PIM", ArchType::PIM},
    };
} // namespace core

#endif // CORE_ARCH_ARCH_TYPES_H