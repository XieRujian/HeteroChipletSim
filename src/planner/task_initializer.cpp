#include <iostream>

#include "task.h"
#include "task_initializer.h"
#include "task_scheduler.h"

namespace planner
{
    void TaskInitializer::Initialize()
    {
        bool gemm = true;
        bool attention = true;
        bool conv = true;
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            if (task_name.find("gemm") != 0)
                gemm = false;
            if (task_name.find("QKV") != 0)
                attention = false;
            if (task_name.find("conv") != 0)
                conv = false;
        }
        if (gemm)
            InitializeGemm();
        else if (attention)
            InitializeAttention();
        else if (conv)
            InitializeConv();
        else
            std::cerr << "Unsupported task pattern in TaskInitializer::Initialize()" << std::endl;
    }

    void TaskInitializer::InitializeGemm()
    {
        uint64_t tile_n = 0, tile_m = 0, tile_k = 0;
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t m_pos = task_name.find('_', 0) + 1;
            uint64_t k_pos = task_name.find('_', m_pos) + 1;
            uint64_t n_pos = task_name.find('_', k_pos) + 1;
            uint64_t m = std::stoul(task_name.substr(m_pos, k_pos - m_pos - 1));
            uint64_t k = std::stoul(task_name.substr(k_pos, n_pos - k_pos - 1));
            uint64_t n = std::stoul(task_name.substr(n_pos));

            tile_m = std::max(tile_m, m + 1);
            tile_k = std::max(tile_k, k + 1);
            tile_n = std::max(tile_n, n + 1);
        }

        if (tile_m == 4 && tile_k == 8 && tile_n == 4)
            Mesh4M4N4K8();
        else if (tile_m == 4 && tile_k == 4 && tile_n == 8)
            Mesh4M4N8K4();
        else if (tile_m == 4 && tile_k == 4 && tile_n == 24)
            Mesh4M1N24K4();
        else if (tile_m == 4 && tile_k == 4 && tile_n == 32)
            Mesh4M1N32K4();
        else
            std::cerr << "Unsupported task pattern: M=" << tile_m << ", K=" << tile_k << ", N=" << tile_n << std::endl;
    }

    void TaskInitializer::InitializeAttention()
    {
        uint64_t tile_h = 0, tile_m = 0, tile_n = 0;
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t h_pos = task_name.find('_', 0) + 1;
            uint64_t m_pos = task_name.find('_', h_pos) + 1;
            uint64_t n_pos = task_name.find('_', m_pos) + 1;
            uint64_t h = std::stoul(task_name.substr(h_pos, m_pos - h_pos - 1));
            uint64_t m = std::stoul(task_name.substr(m_pos, n_pos - m_pos - 1));
            uint64_t n = std::stoul(task_name.substr(n_pos));

            tile_h = std::max(tile_h, h + 1);
            tile_m = std::max(tile_m, m + 1);
            tile_n = std::max(tile_n, n + 1);
        }

        if (tile_h == 6 && tile_m == 4 && tile_n == 4)
            Mesh4H6N4M4();
        else
            std::cerr << "Unsupported task pattern: H=" << tile_h << ", M=" << tile_m << ", N=" << tile_n << std::endl;
    }

    void TaskInitializer::InitializeConv()
    {
        uint64_t tile_w = 0, tile_h = 0, tile_i = 0, tile_o = 0;
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t w_pos = task_name.find('_', 0) + 1;
            uint64_t h_pos = task_name.find('_', w_pos) + 1;
            uint64_t i_pos = task_name.find('_', h_pos) + 1;
            uint64_t o_pos = task_name.find('_', i_pos) + 1;
            uint64_t w = std::stoul(task_name.substr(w_pos, h_pos - w_pos - 1));
            uint64_t h = std::stoul(task_name.substr(h_pos, i_pos - h_pos - 1));
            uint64_t i = std::stoul(task_name.substr(i_pos, o_pos - i_pos - 1));
            uint64_t o = std::stoul(task_name.substr(o_pos));

            tile_w = std::max(tile_w, w + 1);
            tile_h = std::max(tile_h, h + 1);
            tile_i = std::max(tile_i, i + 1);
            tile_o = std::max(tile_o, o + 1);
        }

        if (tile_w == 8 && tile_h == 8 && tile_i == 1 && tile_o == 1)
            Mesh4W8H8I1O1();
        else if (tile_w == 4 && tile_h == 8 && tile_i == 2 && tile_o == 1)
            Mesh4W4H8I2O1();
        else if (tile_w == 2 && tile_h == 4 && tile_i == 6 && tile_o == 1)
            Mesh4W2H4I6O1();
        else if (tile_w == 1 && tile_h == 2 && tile_i == 12 && tile_o == 2)
            Mesh4W1H2I12O2();
        else
            std::cerr << "Unsupported task pattern: W=" << tile_w << ", H=" << tile_h << ", I=" << tile_i << ", O=" << tile_o << std::endl;
    }

    std::vector<TimeStamp> TaskInitializer::GetNonZeroTimeSlot() const
    {
        std::vector<TimeStamp> non_zero_time_slot;
        for (TimeStamp t = task_scheduler_ptr_->schedule_begin_time_; t < task_scheduler_ptr_->schedule_end_time_; ++t)
            if (task_scheduler_ptr_->time_slot_[t].second > 0)
                non_zero_time_slot.push_back(t);
        return non_zero_time_slot;
    }

    void TaskInitializer::Mesh4H6N4M4()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t h_pos = task_name.find('_', 0) + 1;
            uint64_t m_pos = task_name.find('_', h_pos) + 1;
            uint64_t n_pos = task_name.find('_', m_pos) + 1;
            uint64_t h = std::stoul(task_name.substr(h_pos, m_pos - h_pos - 1));
            uint64_t m = std::stoul(task_name.substr(m_pos, n_pos - m_pos - 1));
            uint64_t n = std::stoul(task_name.substr(n_pos));

            uint64_t row = (m % 2) * 2 + (n / 2 % 2);
            uint64_t col = (h % 2) * 2 + (n % 2);
            uint64_t time = non_zero_time_slot.at(((h / 2) * 2 + (m / 2)) / 2);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }

    void TaskInitializer::Mesh4W8H8I1O1()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t w_pos = task_name.find('_', 0) + 1;
            uint64_t h_pos = task_name.find('_', w_pos) + 1;
            uint64_t i_pos = task_name.find('_', h_pos) + 1;
            // uint64_t o_pos = task_name.find('_', i_pos) + 1;
            uint64_t w = std::stoul(task_name.substr(w_pos, h_pos - w_pos - 1));
            uint64_t h = std::stoul(task_name.substr(h_pos, i_pos - h_pos - 1));
            // uint64_t i = std::stoul(task_name.substr(i_pos, o_pos - i_pos - 1));
            // uint64_t o = std::stoul(task_name.substr(o_pos));

            uint64_t row = w % 4;
            uint64_t col = h % 4;
            uint64_t time = non_zero_time_slot.at(0);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }

    void TaskInitializer::Mesh4W4H8I2O1()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t w_pos = task_name.find('_', 0) + 1;
            uint64_t h_pos = task_name.find('_', w_pos) + 1;
            uint64_t i_pos = task_name.find('_', h_pos) + 1;
            // uint64_t o_pos = task_name.find('_', i_pos) + 1;
            uint64_t w = std::stoul(task_name.substr(w_pos, h_pos - w_pos - 1));
            uint64_t h = std::stoul(task_name.substr(h_pos, i_pos - h_pos - 1));
            // uint64_t i = std::stoul(task_name.substr(i_pos, o_pos - i_pos - 1));
            // uint64_t o = std::stoul(task_name.substr(o_pos));

            uint64_t row = w;
            uint64_t col = h % 4;
            uint64_t time = non_zero_time_slot.at(0);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }

    void TaskInitializer::Mesh4W2H4I6O1()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t w_pos = task_name.find('_', 0) + 1;
            uint64_t h_pos = task_name.find('_', w_pos) + 1;
            uint64_t i_pos = task_name.find('_', h_pos) + 1;
            uint64_t o_pos = task_name.find('_', i_pos) + 1;
            uint64_t w = std::stoul(task_name.substr(w_pos, h_pos - w_pos - 1));
            uint64_t h = std::stoul(task_name.substr(h_pos, i_pos - h_pos - 1));
            uint64_t i = std::stoul(task_name.substr(i_pos, o_pos - i_pos - 1));
            // uint64_t o = std::stoul(task_name.substr(o_pos));

            uint64_t row = w * 2 + (i % 2);
            uint64_t col = h;
            uint64_t time = non_zero_time_slot.at((i / 2) / 2);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }

    void TaskInitializer::Mesh4W1H2I12O2()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t w_pos = task_name.find('_', 0) + 1;
            uint64_t h_pos = task_name.find('_', w_pos) + 1;
            uint64_t i_pos = task_name.find('_', h_pos) + 1;
            uint64_t o_pos = task_name.find('_', i_pos) + 1;
            uint64_t w = std::stoul(task_name.substr(w_pos, h_pos - w_pos - 1));
            //uint64_t h = std::stoul(task_name.substr(h_pos, i_pos - h_pos - 1));
            uint64_t i = std::stoul(task_name.substr(i_pos, o_pos - i_pos - 1));
            uint64_t o = std::stoul(task_name.substr(o_pos));

            uint64_t row = (i / 2 % 2) * 2 + o;
            uint64_t col = w * 2 + (i % 2);
            uint64_t time = non_zero_time_slot.at((i / 4) / 2);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }

    void TaskInitializer::Mesh4M4N4K8()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t m_pos = task_name.find('_', 0) + 1;
            uint64_t k_pos = task_name.find('_', m_pos) + 1;
            uint64_t n_pos = task_name.find('_', k_pos) + 1;
            uint64_t m = std::stoul(task_name.substr(m_pos, k_pos - m_pos - 1));
            uint64_t k = std::stoul(task_name.substr(k_pos, n_pos - k_pos - 1));
            uint64_t n = std::stoul(task_name.substr(n_pos));

            uint64_t row = (m % 2) * 2 + (k / 2 % 2);
            uint64_t col = (n % 2) * 2 + (k % 2);
            uint64_t time = non_zero_time_slot.at(((m / 2) * 4 + (n / 2) * 2 + (k / 4)) / 2);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }

    void TaskInitializer::Mesh4M4N8K4()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t m_pos = task_name.find('_', 0) + 1;
            uint64_t k_pos = task_name.find('_', m_pos) + 1;
            uint64_t n_pos = task_name.find('_', k_pos) + 1;
            uint64_t m = std::stoul(task_name.substr(m_pos, k_pos - m_pos - 1));
            uint64_t k = std::stoul(task_name.substr(k_pos, n_pos - k_pos - 1));
            uint64_t n = std::stoul(task_name.substr(n_pos));

            uint64_t row = (m % 2) * 2 + (k / 2 % 2);
            uint64_t col = (n % 2) * 2 + (k % 2);
            uint64_t time = non_zero_time_slot.at(((m / 2) * 4 + (n / 2)) / 2);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }

    void TaskInitializer::Mesh4M1N24K4()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t m_pos = task_name.find('_', 0) + 1;
            uint64_t k_pos = task_name.find('_', m_pos) + 1;
            uint64_t n_pos = task_name.find('_', k_pos) + 1;
            // uint64_t m = std::stoul(task_name.substr(m_pos, k_pos - m_pos - 1));
            uint64_t k = std::stoul(task_name.substr(k_pos, n_pos - k_pos - 1));
            uint64_t n = std::stoul(task_name.substr(n_pos));

            uint64_t row = n % 4;
            uint64_t col = k;
            uint64_t time = non_zero_time_slot.at((n / 4) / 2);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }

    void TaskInitializer::Mesh4M1N32K4()
    {
        auto non_zero_time_slot = GetNonZeroTimeSlot();
        for (auto &[task_name, task] : task_scheduler_ptr_->tasks_)
        {
            uint64_t m_pos = task_name.find('_', 0) + 1;
            uint64_t k_pos = task_name.find('_', m_pos) + 1;
            uint64_t n_pos = task_name.find('_', k_pos) + 1;
            // uint64_t m = std::stoul(task_name.substr(m_pos, k_pos - m_pos - 1));
            uint64_t k = std::stoul(task_name.substr(k_pos, n_pos - k_pos - 1));
            uint64_t n = std::stoul(task_name.substr(n_pos));

            uint64_t row = n % 4;
            uint64_t col = k;
            uint64_t time = non_zero_time_slot.at((n / 4) / 2);

            NodeId node = task_scheduler_ptr_->network_.GetNodeId("node_" + std::to_string(row) + "_" + std::to_string(col));
            task_scheduler_ptr_->AllocateTask(task, time, node);
        }
    }
}