# Example: python configs/generate/ffn_1.py --output_file configs/phase/ffn_1.json --time_slot_types Forward Forward Normal Normal Normal Normal Normal Normal Normal Normal Normal Normal Normal Normal Normal Normal Normal Normal Backward Backward --time_slot_cycles 0 0 6912 6912 6912 6912 6912 6912 6912 6912 6912 6912 6912 6912 6912 6912 6912 6912 0 0 --schedule_begin 2 --schedule_end 18 --hidden 768 --intermediate 3072 --seq_parallel_len 192 192 192 192 --row_parallel 4 --col_parallel_1 16
import argparse
import json


def get_parser():
    parser = argparse.ArgumentParser(
        description="Generate data for FFN task")
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
                        help="Parallel size for FFN")
    parser.add_argument("--intermediate", type=int,
                        required=True, help="Intermediate size for FFN")

    parser.add_argument('--seq_parallel_len', type=int, nargs='+', required=True,
                        help='Parallel size for sequence parallelism, provide a list of lengths for each sequence parallel group')
    #parser.add_argument('--seq_parallel_node', type=str, nargs='+', required=True,
    #                    help='Parallel size for sequence parallelism, provide a list of node indices for each sequence parallel group')
    parser.add_argument("--row_parallel", type=int, required=True,
                        help="parallel size for row parallelism")
    parser.add_argument("--col_parallel_1", type=int, required=True,
                        help="parallel size for column parallelism of the first GEMM in FFN")
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

K_1 = args.hidden // args.row_parallel
N_1 = args.intermediate // args.col_parallel_1

data_info = {}
for i in range(len(args.seq_parallel_len)):
    for j in range(args.row_parallel):
        data_info[f"input_{i}_{j}"] = {
            "shape": [args.seq_parallel_len[i], K_1],
            "dtype": "fp16",
            "tag": "Input",
            "source": f"node_{i}_{j}"
        }

for j in range(args.row_parallel):
    for k in range(args.col_parallel_1):
        data_info[f"weight_{j}_{k}"] = {
            "shape": [K_1, N_1],
            "dtype": "fp16",
            "tag": "Input",
            "source": "Storage"
        }

for i in range(len(args.seq_parallel_len)):
    for k in range(args.col_parallel_1):
        data_info[f"output_{i}_{k}"] = {
            "shape": [args.seq_parallel_len[i], N_1],
            "dtype": "fp16",
            "tag": "Output",
            "destination": f"node_{i}_{k % 4}"
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
for i in range(len(args.seq_parallel_len)):
    for j in range(args.row_parallel):
        for k in range(args.col_parallel_1):
            key = f"gemm_{i}_{j}_{k}"
            task_info[key] = {
                "type": "Gemm",
                "m": args.seq_parallel_len[i],
                "n": N_1,
                "k": K_1,
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
