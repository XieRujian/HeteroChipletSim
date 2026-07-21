#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <string>

using Cycle = uint64_t;
using Tick = uint64_t;
using DataBlockId = int64_t;
using NodeId = int64_t;
using EdgeId = int64_t;
using ChainId = int64_t;
using TimeStamp = int64_t;
using Size = uint64_t;
using Energy = double;

using TaskName = std::string;
using DataBlockName = std::string;
using NodeName = std::string;
using ArchName = std::string;

using Second = uint64_t;

constexpr NodeId INVALID_NODE = -1;
constexpr TimeStamp INVALID_TIME = -1;

namespace util
{
    std::string GetFileContent(const std::string &path);
    void MakeDirectory(const std::string &path);

    double pow(double x, int n);
    inline int log2(uint64_t x) { return 63 - __builtin_clzll(x); }
    constexpr uint64_t ceil(uint64_t x, uint64_t y) { return (x + y - 1) / y; }
    constexpr uint64_t padding(uint64_t size, uint64_t alignment) { return (size + alignment - 1) / alignment * alignment; }
}

namespace TechNode7nm
{
    static constexpr double ENERGY_MAC_DPIM = 1.0;       // DPIM 极简 MAC
    static constexpr double ENERGY_MAC_SA = 1.2;         // SA 复杂 PE 内 MAC (含更多寄存器)
    
    // SRAM (Global Buffer / Subarray)
    static constexpr double ENERGY_SRAM_ROW_ACT = 1.8;   // 每次拉高字线开启一行的能耗
    static constexpr double ENERGY_SRAM_RW_BYTE = 0.15;  // 每字节动态读写能耗
    
    // 数据传输 (Interconnect)
    static constexpr double ENERGY_3D_TSV_BYTE = 0.05;   // 3D 堆叠微凸点/混合键合垂直传输
    static constexpr double ENERGY_2D_BROADCAST_BYTE = 0.55; // 逻辑层长线广播
    static constexpr double ENERGY_PE_HOP_BYTE = 0.15;   // 脉动阵列相邻 PE 间一跳
    static constexpr double ENERGY_BROADCAST_TREE = 0.4;
    
    // 静态漏电 (单位: mW -> 用于转化 pJ)
    static constexpr double STATIC_POWER_MW = 100.0;
}

#endif /* COMMON_H */
