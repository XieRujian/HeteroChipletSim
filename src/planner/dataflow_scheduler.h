#ifndef DATAFLOW_SCHEDULER_H
#define DATAFLOW_SCHEDULER_H

#include <set>
#include <tuple>

#include "core/network.h"
#include "core/task.h"
#include "core/data_block.h"
#include "math/ilp_solver.h"

namespace planner
{
    class Phase;
    class DataflowScheduler
    {
        class ReachabilitySets;

        const Phase *phase_ptr_;
        std::vector<Tick> compute_ticks_;
        TimeStamp prefix_transmission_time_;
        TimeStamp suffix_transmission_time_;
        core::Network network_;
        std::map<DataBlockName, core::DataBlock> data_blocks_;
        std::vector<core::Task> tasks_;

        math::ILPSolver solver_;

        std::string data_analysis_path_;
        std::string intermediate_result_path_;

        ReachabilitySets BuildReachabilitySets() const;
        std::tuple<Tick, Energy, Energy> SolveDataflowILP(math::VariableType var_type, Second time_limit = 21600);
        bool CheckAllTasksScheduled() const;

    public:
        enum class OverheadEstimateType
        {
            Relaxed,
            Strict
        };

    public:
        DataflowScheduler(const Phase *phase_ptr, const std::vector<Tick> &compute_ticks, TimeStamp prefix_transmission_time, TimeStamp suffix_transmission_time, const core::Network &network, const std::map<DataBlockName, core::DataBlock> &data_blocks, const std::map<TaskName, core::Task> &tasks);
        DataflowScheduler(const Phase *phase_ptr, const std::vector<Tick> &compute_ticks, TimeStamp prefix_transmission_time, TimeStamp suffix_transmission_time, const core::Network &network, const std::map<DataBlockName, core::DataBlock> &data_blocks, const std::vector<core::Task> &tasks)
            : phase_ptr_(phase_ptr), compute_ticks_(compute_ticks), prefix_transmission_time_(prefix_transmission_time), suffix_transmission_time_(suffix_transmission_time), network_(network), data_blocks_(data_blocks), tasks_(tasks) {}

        void SetIntermediateResultPath(const std::string &path) { intermediate_result_path_ = path; }
        void SetDataAnalysisPath(const std::string &path) { data_analysis_path_ = path; }
        // void SetNumTimeSteps(TimeStamp num_time_steps) noexcept { num_time_steps_ = num_time_steps; }
        // void SetTimeSlotTicks(Tick time_slot_cycles) noexcept { time_slot_cycles_ = time_slot_cycles; }
        // void SetNetwork(const arch::Network &network) { network_ = network; }
        // void SetDataBlocks(const std::map<DataBlockName, core::DataBlock> &data_blocks) { data_blocks_ = data_blocks; }
        // void SetTasks(const std::map<TaskName, core::Task> &tasks) { tasks_ = tasks; }
        std::tuple<Tick, Energy, Energy> EstimateDataflowOverhead(OverheadEstimateType type, Second time_limit = 21600);

    private:
        struct ReachabilitySets
        {
            std::map<std::pair<TimeStamp, NodeId>, Size> shared_memory_byte;
            std::vector<std::set<std::pair<TimeStamp, NodeId>>> produce_set;
            std::vector<std::set<std::pair<TimeStamp, NodeId>>> consume_set;
            // ==================== 可达性分析 ====================
            /// reachable_set[t,n,d] = 1 表示数据d能在时刻t到达节点n
            std::set<std::tuple<TimeStamp, NodeId, DataBlockId>> reachable_set;
            /// can_reach_set[t,n,d] = 1 表示数据d从时刻t的节点n出发能被用上
            std::set<std::tuple<TimeStamp, NodeId, DataBlockId>> can_reach_set;
        };
    };
} // namespace planner

#endif // DATAFLOW_SCHEDULER_H