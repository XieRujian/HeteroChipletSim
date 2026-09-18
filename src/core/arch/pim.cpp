#include <cmath>

#include "../task/tasks.h"
#include "pim.h"

namespace core
{
    PIM::PIM(const std::map<std::string, std::string> &config_ini) : ArchImpl(ArchType::PIM, config_ini),
                                                                     bank_num_(std::stoull(config_ini.at("bank_num"))),
                                                                     rows_per_subarray_(std::stoull(config_ini.at("rows_per_subarray"))),
                                                                     bit_per_row_(std::stoull(config_ini.at("bit_per_row"))),
                                                                     mac_per_bank_(std::stoull(config_ini.at("mac_per_bank"))),
                                                                     global_bandwidth_bytes_per_tick_(std::stoull(config_ini.at("global_bandwidth_bytes_per_tick"))),
                                                                     local_rf_depth_(std::stoull(config_ini.at("local_rf_depth"))),
                                                                     frequency_ghz_(std::stod(config_ini.at("frequency_ghz"))) {}

    Tick PIM::EvaluateGemmTick(uint64_t m, uint64_t n, uint64_t k, Size dt_bytes) const
    {
        Cycle cycle = 0;
        Cycle pipeline_startup_penalty = 2 * util::log2(bank_num_);
        cycle += pipeline_startup_penalty;

        uint64_t element_per_row = bit_per_row_ / (8 * dt_bytes);
        uint64_t n_per_bank = util::ceil(n, bank_num_);
        auto tile_compute_cycle = [&](uint64_t m, uint64_t n, uint64_t k)
        {
            Cycle access_cycle = util::ceil((m * k + m * n) * dt_bytes, global_bandwidth_bytes_per_tick_);
            Cycle compute_cycle = m * k * n / mac_per_bank_;
            return std::max(access_cycle, compute_cycle);
        };
        if (n_per_bank > element_per_row)
            cycle += tile_compute_cycle(m, element_per_row, k) * (n_per_bank / element_per_row);
        if (uint64_t n_remain = n_per_bank % element_per_row; n_remain > 0)
            cycle += tile_compute_cycle(m, n_remain, k);
        return std::ceil(cycle / frequency_ghz_);
    }

    Energy PIM::EvaluateGemmDynamicEnergy(uint64_t m, uint64_t n, uint64_t k, Size dt_bytes) const
    {
        Energy compute_energy = TechNode7nm::ENERGY_MAC_DPIM * m * n * k;

        uint64_t tile_m = util::ceil(m, local_rf_depth_);
        Energy tsv_energy = (k * n * dt_bytes) * tile_m * TechNode7nm::ENERGY_3D_TSV_BYTE;

        uint64_t n_per_bank = util::ceil(n, bank_num_);
        uint64_t element_per_row = bit_per_row_ / (8 * dt_bytes);
        uint64_t tile_n = util::ceil(n_per_bank, element_per_row);
        uint64_t bytes_broadcast = m * k * dt_bytes * tile_n;
        Energy broadcast_energy = bytes_broadcast * bank_num_ * TechNode7nm::ENERGY_BROADCAST_TREE;

        Energy sram_rw_energy = (bytes_broadcast + m * n * dt_bytes) * TechNode7nm::ENERGY_SRAM_RW_BYTE;

        uint64_t row_activations = util::ceil(k * n_per_bank, element_per_row) * tile_m * bank_num_;
        Energy sram_act_energy = row_activations * TechNode7nm::ENERGY_SRAM_ROW_ACT;

        return compute_energy + tsv_energy + broadcast_energy + sram_rw_energy + sram_act_energy;
    }

    EvaluationResult PIM::Evaluate(const Gemm &gemm) const
    {
        for (DataBlockName data_block_name : gemm.GetWeightBlockNames())
            if (!CheckPreStoreDataBlock(data_block_name))
                return {0xffffffffffff, 0};

        Tick execution_ticks = EvaluateGemmTick(gemm.m, gemm.n, gemm.k, 1);
        Energy dynamic_energy_pj = EvaluateGemmDynamicEnergy(gemm.m, gemm.n, gemm.k, 1);
        return {execution_ticks, dynamic_energy_pj};
    }
    EvaluationResult PIM::Evaluate(const Conv &conv) const
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

        Tick execution_ticks = EvaluateGemmTick(m, n, k, 1);
        Energy dynamic_energy_pj = EvaluateGemmDynamicEnergy(m, n, k, 1);
        return {execution_ticks, dynamic_energy_pj};
    }
    EvaluationResult PIM::Evaluate(const Attention &attention) const
    {
        Tick execution_ticks = 0;
        Energy dynamic_energy_pj = 0;
        Energy static_energy_pj = 0;
        { // QK
            uint64_t m = attention.seq_length;
            uint64_t n = attention.kv_size;
            uint64_t k = attention.head_dim;

            Tick tick = attention.head_num * EvaluateGemmTick(m, n, k, 1);
            Energy energy = attention.head_num * EvaluateGemmDynamicEnergy(m, n, k, 1);
            execution_ticks += tick;
            dynamic_energy_pj += energy;
        }
        { // AV
            uint64_t m = attention.seq_length;
            uint64_t n = attention.head_dim;
            uint64_t k = attention.kv_size;

            Tick tick = attention.head_num * EvaluateGemmTick(m, n, k, 1);
            Energy energy = attention.head_num * EvaluateGemmDynamicEnergy(m, n, k, 1);
            execution_ticks += tick;
            dynamic_energy_pj += energy;
        }

        return {execution_ticks, dynamic_energy_pj};
    }

    bool PIM::CanSchedule(const Task &task) const
    {
        for (DataBlockName data_block_name : task->GetWeightBlockNames())
            if (!CheckPreStoreDataBlock(data_block_name))
                return false;
        return true;
    }
} // namespace core