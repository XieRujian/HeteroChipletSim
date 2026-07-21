# HeteroChipletSim

详细的项目内容与 [goal.md](goal.md) 对照说明见 [docs/goal_alignment.md](docs/goal_alignment.md)。

HeteroChipletSim 是一个面向 **chiplet/片上互连拓扑** 的“**任务映射（time+node）** + **数据流/通信可行性与能耗评估**”的 C++ 仿真与优化工具。

它把一个 phase 中的任务图（Task + DataBlock）映射到网络拓扑的节点与时间槽上，并在给定映射下通过 ILP（Gurobi）求解数据在网络中的传播/驻留（buffer/flow），输出通信侧的 slack 与能耗开销。

---

## 目录结构（按职责分层）

- `src/app/`
  - 可执行程序入口（命令行）。目前包含：
    - `task_schedule.cpp`：为指定用例/phase 搜索任务分配并写出 ini
    - `dataflow_schedule.cpp`：读取 ini 任务映射并评估数据流开销
    - `compute_energy.cpp`：对单个架构/任务做能耗评估的实验入口
- `src/planner/`
  - “规划器/调度器”层：加载 framework/phase，做任务分配与数据流 ILP
- `src/core/`
  - 核心建模：网络拓扑（节点/边/通信链）、架构模型（SystolicArray/PIM/Storage）、任务模型（Gemm/Conv/Attention/Collect）、数据块模型
- `src/math/`
  - ILP 建模与求解器封装（当前实现对接 Gurobi）
- `src/util/`
  - 基础工具：JSON/INI 读写、随机数、文件/目录工具、通用类型定义等

此外：
- `configs/`：用于生成/存放拓扑、架构 ini、phase JSON 的配置与脚本
- `library/`：用例库（benchmark 实例）。每个子目录一般是一个“实验根目录”，包含 `framework.json`、每个 phase 的任务映射 ini、以及 ILP/搜索输出目录等

---

## 核心概念与数据结构

### 1) Framework / Phase

- **Framework**：一个实验根目录（例如 `library/rt_heter_2`），通过 `framework.json` 指定拓扑、架构、多个 phase。
  - 代码入口：`planner::Framework`（见 `src/planner/framework.h/.cpp`）
- **Phase**：一次独立的调度与评估单元，包含：
  - `time_slot[]`：离散时间槽，每个槽有类型与可用 cycles（见下）
  - `schedule_begin/schedule_end`：允许放置计算任务的时间窗
  - `tasks`：该 phase 的任务集合
  - `data_blocks`：该 phase 中会被生产/消费的数据块集合
  - 代码入口：`planner::Phase`（见 `src/planner/phase.h/.cpp`）

### 2) Time slot（时间槽）

`configs/phase/*.json` 的 `time_slot` 是一个数组，每一项包含：

- `type`：`Normal | Forward | Backward`
- `cycles`：该槽位内可用的 tick/cycle 数

直观理解：
- `Normal`：主要承载计算与常规通信约束的“中心槽”。
- `Forward/Backward`：用于把相邻若干槽并入同一个带宽预算窗口（数据流 ILP 中会把 Normal 槽附近的 Forward/Backward 槽的 cycles 合并到同一条带宽约束里）。

### 3) Network（拓扑）与 Chain（通信链）

- 拓扑由 `configs/topology/*.json` 给出（见 `core::Network::LoadTopology`：`src/core/network.h/.cpp`）。
- 该拓扑不仅有 **edges**（链路），还要求有 **chain**（通信链）：
  - `edges` 是一个“带名字的字典对象”，每条边包含 `from/to/bandwidth/energy_pj_per_bit`。
  - `chain` 是一个数组/对象集合：每条 chain 给出 `from`、`to`（可多目的节点集合）、以及 `edges`（该 chain 经过哪些 edge 名字）。

项目里数据流 ILP 的 flow 变量是 **按 chain 建模** 的：`flow(t, chain, data_block)`，
而带宽约束会落到每条 edge 上（通过 chain->edge 的包含关系聚合）。

### 4) Arch（节点架构）

- 每个节点的架构由 `configs/arch/*.ini` 给出。
- `core::Arch` 是一个 facade，内部用 `type` 字段实例化不同 `ArchImpl`：
  - `SystolicArray`
  - `PIM`
  - `Storage`
- 每个架构能：
  - `Evaluate(task)`：返回 `{execution_ticks, energy_pj}`
  - `CanSchedule(task)`：是否允许把该任务放到该节点（例如 Storage 通常不能跑计算）
  - `GetBufferSizeBytes()`：buffer 容量（字节）

见 `src/core/arch.h/.cpp` 与 `src/core/arch/*`。

### 5) Task / DataBlock

- **Task**：采用 visitor/多态方式（`TaskImpl::accept(Arch)`）支持多任务类型：
  - Gemm / Conv / Attention / Collect（见 `src/core/task/` 与 `src/core/task.cpp`）
- **DataBlock**：只区分 `Input` / `Output` 两类，并携带 dtype/shape/source/destination（见 `src/core/data_block.h/.cpp`）。

此外，Task 还可以声明 `shared_memory_byte`：它会在 buffer 约束里从占用中扣除/单独处理，用于表达节点侧“不可挪用的共享内存占用”。

---

## 输入/输出文件约定

### 1) `library/<case>/framework.json`

典型结构（字段名以当前实现为准）：

- `topology`: `configs/topology/*.json`
- `arch`: `configs/arch/*.ini`
- `phases`: `{ "phase_name": "configs/phase/*.json", ... }`

Framework 会按 `phases` 的遍历顺序把 phase 依次载入到 `Framework::phases_`。

### 2) `configs/phase/*.json`

关键字段：

- `time_slot`: `[{type, cycles}, ...]`
- `schedule_begin`, `schedule_end`
- `data_blocks`: `{ name: {shape, dtype, tag, source/destination}, ... }`
- `tasks`: `{ name: {type, inputs, weights?, outputs, shared_memory_byte, ...任务参数...}, ... }`

例如 `Gemm` 任务会包含 `m/n/k`。

### 3) `library/<case>/<phase_name>.ini`（任务映射输出/输入）

该 ini 是“任务分配”的结果，也可作为 dataflow 评估的输入。格式为：

```ini
[task_name]
node = node_0_0
time = 0
```

注意：这里的 `time` 是 **相对于 `schedule_begin` 的偏移**。
也就是说，当 `schedule_begin=4`，ini 里的 `time=0` 实际会被调度到全局时间步 `t=4`。

---

## 逻辑流程（从入口程序看全链路）

### A. 任务分配：`task_schedule`

入口：`src/app/task_schedule.cpp`

用法：

```bash
./build/bin/task_schedule <lib_name> <phase_id>
```

- `<lib_name>`：`library/` 下的子目录名，例如 `rt_heter_2`
- `<phase_id>`：该用例 `framework.json` 里 phases 的索引（从 0 开始）

内部流程：

1. `Framework(root_path)` 加载拓扑与节点 arch，并加载全部 phases。
2. 取 `Framework::GetPhases()[phase_id]`。
3. `Phase::ScheduleTaskAllocation()` 创建 `planner::TaskScheduler` 并执行搜索。
4. 输出：`library/<lib_name>/search_results/<phase_name>.ini`

如果你想立刻用 `dataflow_schedule` 对该结果做 ILP 评估，需要把这个 ini 拷贝/重命名为：

- `library/<lib_name>/<phase_name>.ini`

#### TaskScheduler 的核心思想（简述）

- 使用模拟退火式搜索（`AdjustAllocations` + Metropolis 接受准则）。
- **硬约束**（不可行会被惩罚/在 move 中尽量规避）：
  - 节点是否可执行：`Arch::CanSchedule(task)`
  - 计算 ticks：同一 `(time,node)` 的 `Σ execution_ticks` 不超过该 time_slot 的 `cycles`
  - buffer 容量：同一 `(time,node)` 的数据块驻留需求不超过 `GetBufferSizeBytes()`（并考虑 `shared_memory_byte`）
- **代价函数（ProxyEvaluation）**由多项组成，指导搜索趋向更好的映射：
  - compute/buffer 利用率的高次惩罚（强烈避免接近或超过上限）
  - 数据块的时空“集中性”惩罚（基于 produce/consume 的时间方差与空间距离平方的最小值）
  - 依赖可达性惩罚：如果某数据块从 producer 到 consumer 的最短 chain 距离超过时间差，会被惩罚（鼓励满足依赖可达）

### B. 数据流评估：`dataflow_schedule`

入口：`src/app/dataflow_schedule.cpp`

用法：

```bash
./build/bin/dataflow_schedule <lib_name> <phase_id>
```

内部流程：

1. 加载 Framework 与指定 Phase。
2. 读取 `library/<lib_name>/<phase_name>.ini`。
3. 对每个任务调用 `phase.ScheduleTask(time, node_name, task_name)`：
   - 会自动把 ini 的 `time` 加上 `schedule_begin` 形成全局时间步。
4. 调用 `phase.EstimateDataflowOverhead()`：
   - 构建 `DataflowScheduler` 并用 ILP 求最小通信开销。
5. 输出：
   - 标准输出打印 `Dataflow overhead: <objective>`
   - 若设置了中间结果路径：会写 `library/<lib_name>/ilp_intermediate_result/<phase_name>.txt`

#### DataflowScheduler 的核心思想（简述）

- 变量：
  - `buffer(t,node,data)`：数据是否在该时刻驻留于节点
  - `flow(t,chain,data)`：数据是否沿某条 chain 在该时刻传播一步
  - `slack`：带宽不足时的松弛量（等价于需要更多可用 cycles）
- 为了缩小 ILP 规模，会先做可达性分析（前向 reachable / 反向 can_reach），只对“既能到达又未来会用上”的状态创建变量。
- 约束包括：
  - 输入/输出数据的存在性（t=0 的 source、t=T-1 的 destination）
  - 输入/输出数据的流入/流出一致性（不能凭空出现/不能消失）
  - 节点 buffer 容量（扣除 shared_memory_byte）
  - edge 带宽约束：会把一个 `Normal` 槽附近的 `Forward/Backward` 槽合并成同一时间窗口，统一对每条 edge 做流量总和约束
- 目标函数：
  - `min  1e7 * slack  +  Σ(flow * size * 8 * chain_energy_per_bit)`
  - 即优先压低 slack（保证带宽可行），同时最小化通信能耗。

### C. 单点能耗评估：`compute_energy`

入口：`src/app/compute_energy.cpp`

用法：

```bash
./build/bin/compute_energy <lib_name>
```

该程序主要用于对某个节点 arch 的 `Evaluate(Task)` 做实验性输出（当前实现里示例性地评估了一个 Gemm）。

---

## 构建

仓库里提供了 `compile.sh`：

```bash
./compile.sh
```

它等价于在已配置的 `build/` 目录下执行 `make -j`。
如果你需要从零生成 `build/`，可以用 CMake 常规方式生成后再编译（本项目为标准 CMake 工程）。

---

## 常见问题与坑点

- 拓扑文件必须包含 `chain` 字段，否则 `core::Network::LoadTopology` 会触发断言失败。
- `library/<case>/<phase>.ini` 里的 `time` 是相对 `schedule_begin` 的偏移；如果你手写 ini，时间基准不要搞错。
- 数据流 ILP 依赖 Gurobi（见 `src/math/gurobi.cpp`）。没有正确安装/链接时会导致编译或运行失败。
- phase 的 `time_slot` 中允许出现 `cycles=0` 的槽；这会影响可用资源与带宽窗口合并逻辑。

---

## 想扩展一个新用例（建议路径）

1. 在 `configs/topology/` 添加/生成带 `edges+chain` 的拓扑 JSON。
2. 在 `configs/arch/` 添加/生成节点架构 ini（每个 node 一个 section）。
3. 在 `configs/phase/` 添加 phase JSON（time_slot、data_blocks、tasks）。
4. 在 `library/<new_case>/` 新建 `framework.json` 指向上述文件。
5. 运行 `task_schedule` 生成 `search_results/<phase>.ini`，或手写 `<phase>.ini` 后运行 `dataflow_schedule` 做评估。
