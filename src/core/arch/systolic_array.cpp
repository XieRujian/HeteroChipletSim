#include <cmath>

#include "util/common.h"

#include "../task/tasks.h"
#include "systolic_array.h"

namespace core
{
    SystolicArray::SystolicArray(const std::map<std::string, std::string> &config_ini)
        : ArchImpl(ArchType::SA, config_ini),
          row_(std::stoull(config_ini.at("row"))),
          col_(std::stoull(config_ini.at("col"))),
          num_array_(std::stoull(config_ini.at("num_array"))),
          frequency_ghz_(std::stod(config_ini.at("frequency_ghz"))) {}

    Tick SystolicArray::EvaluateGemmTick(uint64_t m, uint64_t n, uint64_t k) const
    {
        n = util::ceil(n, col_);
        k = util::ceil(k, row_);
        uint64_t cycles = util::ceil(n * k, num_array_) * (m + col_ + row_ - 1);
        return std::ceil(cycles / frequency_ghz_);
    }

    Energy SystolicArray::EvaluateGemmDynamicEnergy(uint64_t m, uint64_t n, uint64_t k, Size dt_bytes) const
    {
        Energy compute_energy = TechNode7nm::ENERGY_MAC_SA * m * n * k;

        uint64_t total_hops = 2 * m * k * n + k * n * (row_ / 2);
        Energy hop_energy = total_hops * TechNode7nm::ENERGY_PE_HOP_BYTE * dt_bytes;

        uint64_t total_gb_rw_bytes = (m * k + k * n + m * n) * dt_bytes;
        double gb_rw_energy = total_gb_rw_bytes * TechNode7nm::ENERGY_SRAM_RW_BYTE;

        uint64_t gb_row_activations = util::ceil(total_gb_rw_bytes, gb_row_width_bytes_);
        double gb_act_energy = gb_row_activations * TechNode7nm::ENERGY_SRAM_ROW_ACT;

        return compute_energy + hop_energy + gb_rw_energy + gb_act_energy;
    }

    EvaluationResult SystolicArray::Evaluate(const Gemm &gemm) const
    {
        Tick execution_ticks = EvaluateGemmTick(gemm.m, gemm.n, gemm.k);
        Energy dynamic_energy_pj = EvaluateGemmDynamicEnergy(gemm.m, gemm.n, gemm.k, 1);
        return {execution_ticks, dynamic_energy_pj};
    }

    EvaluationResult SystolicArray::Evaluate(const Conv &conv) const
    {
        uint64_t batch_size = conv.batch_size;
        uint64_t in_channels = conv.in_channels;
        uint64_t out_channels = conv.out_channels;
        uint64_t in_height = conv.in_height;
        uint64_t in_width = conv.in_width;
        uint64_t kernel_height = conv.kernel_height;
        uint64_t kernel_width = conv.kernel_width;
        uint64_t stride_height = conv.stride_height;
        uint64_t stride_width = conv.stride_width;
        uint64_t padding_height = conv.padding_height;
        uint64_t padding_width = conv.padding_width;

        uint64_t out_height = (in_height + 2 * padding_height - kernel_height) / stride_height + 1;
        uint64_t out_width = (in_width + 2 * padding_width - kernel_width) / stride_width + 1;
        uint64_t m = out_height * out_width;
        uint64_t n = out_channels;
        uint64_t k = kernel_width * kernel_height * in_channels;

        Tick execution_ticks = EvaluateGemmTick(m, n, k);
        Energy dynamic_energy_pj = EvaluateGemmDynamicEnergy(m, n, k, 1);
        return {execution_ticks, dynamic_energy_pj};
    }

    EvaluationResult SystolicArray::Evaluate(const Attention &attention) const
    {
        Tick execution_ticks = 0;
        Energy dynamic_energy_pj = 0;
        { // QK
            uint64_t m = attention.seq_length;
            uint64_t n = attention.kv_size;
            uint64_t k = attention.head_dim;

            Tick tick = attention.head_num * EvaluateGemmTick(m, n, k);
            Energy energy = attention.head_num * EvaluateGemmDynamicEnergy(m, n, k, 1);
            execution_ticks += tick;
            dynamic_energy_pj += energy;
        }
        { // AV
            uint64_t m = attention.seq_length;
            uint64_t n = attention.head_dim;
            uint64_t k = attention.kv_size;

            Tick tick = attention.head_num * EvaluateGemmTick(m, n, k);
            Energy energy = attention.head_num * EvaluateGemmDynamicEnergy(m, n, k, 1);
            execution_ticks += tick;
            dynamic_energy_pj += energy;
        }

        return {execution_ticks, dynamic_energy_pj};
    }
}
