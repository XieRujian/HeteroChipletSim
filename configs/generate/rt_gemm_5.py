# Example: python configs/generate/gemm.py --output_file configs/phase/gemm.json --time_slot_types Forward Forward Normal Normal Normal Normal Backward --time_slot_cycles 0 0 760 760 760 760 0 --schedule_begin 2 --schedule_end 8 --m 256 --n 256 --k 384 --tile_size 64
import argparse
import json


def get_parser():
    parser = argparse.ArgumentParser(
        description="Generate data for GEMM task")
    parser.add_argument("--output_file", type=str, required=True,
                        help="File to save generated data")

    parser.add_argument("--time_slot_types", type=str, nargs='+',
                        required=True, help="Time slot types for task scheduling")
    parser.add_argument("--time_slot_cycles", type=int, nargs='+',
                        required=True, help="Time slot cycles for task scheduling")
    parser.add_argument("--schedule_begin", type=int,
                        required=True, help="Begin time stamp for task scheduling")
    parser.add_argument("--schedule_end", type=int,
                        required=True, help="End time stamp for task scheduling")

    parser.add_argument("--m", type=int, required=True,
                        help="GEMM matrix dimension M")
    parser.add_argument("--n", type=int, required=True,
                        help="GEMM matrix dimension N")
    parser.add_argument("--k", type=int, required=True,
                        help="GEMM matrix dimension K")
    parser.add_argument("--m_tile_size", type=int, required=True,
                        help="Tile size of M for GEMM")
    parser.add_argument("--n_tile_size", type=int, required=True,
                        help="Tile size of N for GEMM")
    parser.add_argument("--k_tile_size", type=int, required=True,
                        help="Tile size of K for GEMM")
    return parser


parser = get_parser()
args = parser.parse_args()

if len(args.time_slot_types) != len(args.time_slot_cycles):
    print(f"Length of time_slot_types: {len(args.time_slot_types)}, Length of time_slot_cycles: {len(args.time_slot_cycles)}")
    raise ValueError(
        "The length of time_slot_types and time_slot_cycles must be the same")

#if len(args.seq_parallel_len) != len(args.seq_parallel_node):
#    raise ValueError(
#        "The length of seq_parallel_len and seq_parallel_node must be the same")

time_slot = []
for i in range(len(args.time_slot_types)):
    time_slot.append({
        "type": args.time_slot_types[i],
        "cycles": args.time_slot_cycles[i]
    })

M = args.m
N = args.n
K = args.k
M_tile_size = args.m_tile_size
N_tile_size = args.n_tile_size
K_tile_size = args.k_tile_size
M_tile = (M + M_tile_size - 1) // M_tile_size
N_tile = (N + N_tile_size - 1) // N_tile_size
K_tile = (K + K_tile_size - 1) // K_tile_size

pim = ["node_0_2", "node_1_1", "node_2_2", "node_3_1"]

data_info = {}
for i in range(M_tile):
    for j in range(K_tile):
        data_info[f"input_{i}_{j}"] = {
            "shape": [min(M_tile_size, M - i * M_tile_size), min(K_tile_size, K - j * K_tile_size)],
            "dtype": "int8",
            "tag": "Input",
            "source": "pim_0" if j in range(0, K_tile // 4) else "pim_1" if j in range(K_tile // 4, K_tile // 2) else "pim_2" if j in range(K_tile // 2, 3 * K_tile // 4) else "pim_3"
        }

for j in range(K_tile):
    for k in range(N_tile):
        data_info[f"weight_{j}_{k}"] = {
            "shape": [min(K_tile_size, K - j * K_tile_size), min(N_tile_size, N - k * N_tile_size)],
            "dtype": "int8",
            "tag": "Input",
            "source": "Storage"
        }

for i in range(M_tile):
    for k in range(N_tile):
        data_info[f"output_{i}_{k}"] = {
            "shape": [min(M_tile_size, M - i * M_tile_size), min(N_tile_size, N - k * N_tile_size)],
            "dtype": "int8",
            "tag": "Output",
            "destination": "pim_0" if k in range(0, N_tile // 4) else "pim_1" if k in range(N_tile // 4, N_tile // 2) else "pim_2" if k in range(N_tile // 2, 3 * N_tile // 4) else "pim_3"
        }

#for i in range(len(args.seq_parallel_len)):
#    for k in range(args.col_parallel_1):
#        data_info[f"input2_{i}_{k}"] = {
#            "shape": [args.seq_parallel_len[i], N_1],
#            "dtype": "fp16",
#            "tag": "Input",
#        }

#for j in range(args.col_parallel_1):
#    for k in range(args.col_parallel_2):
#        data_info[f"weight2_{j}_{k}"] = {
#            "shape": [N_1, N_2],
#            "dtype": "fp16",
#            "tag": "Input",
#            "source": "Storage"
#        }

#for i in range(len(args.seq_parallel_len)):
#    for k in range(args.col_parallel_2):
#        data_info[f"output2_{i}_{k}"] = {
#            "shape": [args.seq_parallel_len[i], N_2],
#            "dtype": "fp16",
#            "tag": "Output",
#            "destination": args.seq_parallel_node[i]
#        }

task_info = {}
for i in range(M_tile):
    for j in range(K_tile):
        for k in range(N_tile):
            key = f"gemm_{i}_{j}_{k}"
            task_info[key] = {
                "type": "Gemm",
                "m": min(M_tile_size, M - i * M_tile_size),
                "n": min(N_tile_size, N - k * N_tile_size),
                "k": min(K_tile_size, K - j * K_tile_size),
                "inputs": [f"input_{i}_{j}"] + [f"weight_{j}_{k}"],
                "outputs": [f"output_{i}_{k}"],
                "shared_memory_byte": 0
            }

#for i in range(len(args.seq_parallel_len)):
#    for k in range(args.col_parallel_1):
#        key = f"collect1_{i}_{k}"
#        task_info[key] = {
#            "type": "Collect",
#            "inputs": [f"output1_{i}_{k}"],
#            "outputs": [f"input2_{i}_{k}"],
#            "shared_memory_byte": args.seq_parallel_len[i] * N_1 * 2
#        }

#for i in range(len(args.seq_parallel_len)):
#    for j in range(args.col_parallel_1):
#        for k in range(args.col_parallel_2):
#            key = f"gemm2_{i}_{j}_{k}"
#            task_info[key] = {
#                "type": "Gemm",
#                "m": args.seq_parallel_len[i],
#                "n": N_2,
#                "k": N_1,
#                "inputs": [f"input2_{i}_{j}"] + [f"weight2_{j}_{k}"],
#                "outputs": [f"output2_{i}_{k}"],
#                "shared_memory_byte": 0
#            }

json_data = {
    "time_slot": time_slot,
    "schedule_begin": args.schedule_begin,
    "schedule_end": args.schedule_end,
    "data_blocks": data_info,
    "tasks": task_info
}

with open(args.output_file, 'w') as file:
    json.dump(json_data, file, indent=4)
