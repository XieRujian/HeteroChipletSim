# Example: python configs/generate/attention.py --output_file configs/phase/attention.json --time_slot_types Forward Forward Normal Normal Normal Backward Backward Forward Normal Backward Backward --time_slot_cycles 0 0 27648 27648 27648 0 0 0 55296 0 0 --schedule_begin 2 --schedule_end 9 --hidden 768 --num_heads 12 --seq_parallel_len 192 192 192 192 --seq_parallel_node node_0_1 node_1_1 node_2_1 node_3_1 --row_parallel 4 --w_col_parallel_len 384 --seq_parallel_group_1 2 --seq_parallel_group_2 2 --head_parallel 4
import argparse
import json


def get_parser():
    parser = argparse.ArgumentParser(
        description="Generate data for Attention task")
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

    parser.add_argument("--hidden", type=int, required=True,
                        help="Parallel size for Attention")
    parser.add_argument("--num_heads", type=int, required=True,
                        help="Number of heads for Attention")

    parser.add_argument('--seq_parallel_len', type=int, nargs='+', required=True,
                        help='Parallel size for sequence parallelism, provide a list of lengths for each sequence parallel group')
    parser.add_argument('--seq_parallel_node', type=str, nargs='+', required=True,
                        help='Parallel size for sequence parallelism, provide a list of node indices for each sequence parallel group')
    parser.add_argument("--row_parallel", type=int, required=True,
                        help="parallel size for row parallelism")
    parser.add_argument("--w_col_parallel_len", type=int, required=True,
                        help="parallel size for column parallelism of the first GEMM in Attention")

    parser.add_argument("--seq_parallel_group_1", type=int, required=True,
                        help="group size for sequence parallelism of the second GEMM in Attention")
    parser.add_argument("--seq_parallel_group_2", type=int, required=True,
                        help="group size for sequence parallelism of the second GEMM in Attention")
    parser.add_argument("--head_parallel", type=int, required=True,
                        help="group size for head parallelism in Attention")
    return parser


parser = get_parser()
args = parser.parse_args()

if len(args.time_slot_types) != len(args.time_slot_cycles):
    raise ValueError(
        "The length of time_slot_types and time_slot_cycles must be the same")

if len(args.seq_parallel_len) != len(args.seq_parallel_node):
    raise ValueError(
        "The length of seq_parallel_len and seq_parallel_node must be the same")

time_slot = []
for i in range(len(args.time_slot_types)):
    time_slot.append({
        "type": args.time_slot_types[i],
        "cycles": args.time_slot_cycles[i]
    })

K_1 = args.hidden // args.row_parallel
q_col_parallel = args.hidden // args.w_col_parallel_len
kv_col_parallel = args.hidden // args.w_col_parallel_len * 2

data_info = {}
for i in range(len(args.seq_parallel_len)):
    for j in range(args.row_parallel):
        data_info[f"input_{i}_{j}"] = {
            "shape": [args.seq_parallel_len[i], K_1],
            "dtype": "fp16",
            "tag": "Input",
            "source": args.seq_parallel_node[i]
        }

for j in range(args.row_parallel):
    for k in range(q_col_parallel):
        data_info[f"WQ_{j}_{k}"] = {
            "shape": [K_1, args.w_col_parallel_len],
            "dtype": "fp16",
            "tag": "Input",
            "source": "Storage"
        }

for j in range(args.row_parallel):
    for k in range(kv_col_parallel):
        data_info[f"WKV_{j}_{k}"] = {
            "shape": [K_1, args.w_col_parallel_len],
            "dtype": "fp16",
            "tag": "Input",
            "source": "Storage"
        }

for i in range(len(args.seq_parallel_len)):
    for k in range(args.head_parallel):
        data_info[f"outputQ_{i}_{k}"] = {
            "shape": [args.seq_parallel_len[i], args.hidden // args.head_parallel],
            "dtype": "fp16",
            "tag": "Output",
        }

for i in range(len(args.seq_parallel_len)):
    for k in range(args.head_parallel):
        data_info[f"outputKV_{i}_{k}"] = {
            "shape": [args.seq_parallel_len[i], args.hidden // args.head_parallel * 2],
            "dtype": "fp16",
            "tag": "Output",
        }

for i in range(len(args.seq_parallel_len) // args.seq_parallel_group_1):
    for j in range(args.head_parallel):
        data_info[f"inputQ_{i}_{j}"] = {
            "shape": [sum(args.seq_parallel_len[i*args.seq_parallel_group_1:(i+1)*args.seq_parallel_group_1]), args.hidden // args.head_parallel],
            "dtype": "fp16",
            "tag": "Input",
        }

for i in range(len(args.seq_parallel_len) // args.seq_parallel_group_2):
    for j in range(args.head_parallel):
        data_info[f"inputKV_{i}_{j}"] = {
            "shape": [sum(args.seq_parallel_len[i*args.seq_parallel_group_2:(i+1)*args.seq_parallel_group_2]), args.hidden // args.head_parallel * 2],
            "dtype": "fp16",
            "tag": "Input",
        }

for i in range(len(args.seq_parallel_len)):
    for j in range(args.head_parallel):
        data_info[f"output_{i}_{j}"] = {
            "shape": [args.seq_parallel_len[i], args.hidden // args.head_parallel],
            "dtype": "fp16",
            "tag": "Output",
            "destination": args.seq_parallel_node[i]
        }

task_info = {}
q_group_size = args.head_parallel // q_col_parallel
for i in range(len(args.seq_parallel_len)):
    for j in range(args.row_parallel):
        for k in range(q_col_parallel):
            key = f"Q_{i}_{j}_{k}"
            task_info[key] = {
                "type": "Gemm",
                "m": args.seq_parallel_len[i],
                "n": args.w_col_parallel_len,
                "k": K_1,
                "inputs": [f"input_{i}_{j}"] + [f"WQ_{j}_{k}"],
                "outputs": [f"outputQ_{i}_{k * q_group_size + l}" for l in range(q_group_size)],
                "shared_memory_byte": 0
            }

kv_group_size = args.head_parallel // kv_col_parallel
for i in range(len(args.seq_parallel_len)):
    for j in range(args.row_parallel):
        for k in range(kv_col_parallel):
            key = f"KV_{i}_{j}_{k}"
            task_info[key] = {
                "type": "Gemm",
                "m": args.seq_parallel_len[i],
                "n": args.w_col_parallel_len,
                "k": K_1,
                "inputs": [f"input_{i}_{j}"] + [f"WKV_{j}_{k}"],
                "outputs": [f"outputKV_{i}_{k * kv_group_size + l}" for l in range(kv_group_size)],
                "shared_memory_byte": 0
            }

for i in range(len(args.seq_parallel_len) // args.seq_parallel_group_1):
    for k in range(args.head_parallel):
        key = f"collectQ_{i}_{k}"
        task_info[key] = {
            "type": "Collect",
            "inputs": [f"outputQ_{i * args.seq_parallel_group_1 + j}_{k}" for j in range(args.seq_parallel_group_1)],
            "outputs": [f"inputQ_{i}_{k}"],
            "shared_memory_byte": sum(args.seq_parallel_len[i * args.seq_parallel_group_1: (i + 1) * args.seq_parallel_group_1]) * (args.hidden // args.head_parallel) * 2
        }

for i in range(len(args.seq_parallel_len) // args.seq_parallel_group_2):
    for k in range(args.head_parallel):
        key = f"collectKV_{i}_{k}"
        task_info[key] = {
            "type": "Collect",
            "inputs": [f"outputKV_{i * args.seq_parallel_group_2 + j}_{k}" for j in range(args.seq_parallel_group_2)],
            "outputs": [f"inputKV_{i}_{k}"],
            "shared_memory_byte": sum(args.seq_parallel_len[i * args.seq_parallel_group_2: (i + 1) * args.seq_parallel_group_2]) * (2 * args.hidden // args.head_parallel) * 2
        }

for k in range(args.head_parallel):
    for i in range(len(args.seq_parallel_len) // args.seq_parallel_group_1):
        for j in range(len(args.seq_parallel_len) // args.seq_parallel_group_2):
            key = f"QKV_{k}_{i}_{j}"
            task_info[key] = {
                "type": "Attention",
                "seq_length": sum(args.seq_parallel_len[i * args.seq_parallel_group_1: (i + 1) * args.seq_parallel_group_1]),
                "head_num": args.num_heads // args.head_parallel,
                "head_dim": args.hidden // args.num_heads,
                "kv_size": sum(args.seq_parallel_len[j * args.seq_parallel_group_2: (j + 1) * args.seq_parallel_group_2]),
                "inputs": [f"inputQ_{i}_{k}"] + [f"inputKV_{j}_{k}"],
                "outputs": [f"output_{i * args.seq_parallel_group_1 + l}_{k}" for l in range(args.seq_parallel_group_1)],
                "shared_memory_byte": 0
            }

json_data = {
    "time_slot": time_slot,
    "schedule_begin": args.schedule_begin,
    "schedule_end": args.schedule_end,
    "data_blocks": data_info,
    "tasks": task_info
}

with open(args.output_file, 'w') as file:
    json.dump(json_data, file, indent=4)
