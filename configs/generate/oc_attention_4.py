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

    parser.add_argument("--seq_len", type=int, required=True,
                        help="seq_len for Attention")
    parser.add_argument("--kv_size", type=int, required=True,
                        help="kv_size for Attention")
    parser.add_argument("--hidden", type=int, required=True,
                        help="Parallel size for Attention")
    parser.add_argument("--num_heads", type=int, required=True,
                        help="Number of heads for Attention")
    
    parser.add_argument('--seq_tile_size', type=int, required=True,
                        help='Tile size for sequence parallelism in Attention')
    parser.add_argument("--head_tile_size", type=int, required=True,
                        help="Tile size for head parallelism in Attention")
    parser.add_argument("--kv_tile_size", type=int, required=True,
                        help="Tile size for row parallelism in Attention")
    return parser


parser = get_parser()
args = parser.parse_args()

M = args.seq_len
N = args.kv_size
K = args.hidden
num_heads = args.num_heads
head_dim = K // num_heads

seq_tile_size = args.seq_tile_size
head_tile_size = args.head_tile_size
kv_tile_size = args.kv_tile_size
seq_tile = (M + seq_tile_size - 1) // seq_tile_size
head_tile = num_heads // head_tile_size
kv_tile = (N + kv_tile_size - 1) // kv_tile_size

if len(args.time_slot_types) != len(args.time_slot_cycles):
    raise ValueError(
        "The length of time_slot_types and time_slot_cycles must be the same")

time_slot = []
for i in range(len(args.time_slot_types)):
    time_slot.append({
        "type": args.time_slot_types[i],
        "cycles": args.time_slot_cycles[i]
    })

data_info = {}
for i in range(seq_tile):
    for k in range(head_tile):
        data_info[f"Q_{i}_{k}"] = {
            "shape": [min(seq_tile_size, M - i * seq_tile_size), head_tile_size * head_dim],
            "dtype": "int8",
            "tag": "Input",
            "source": "pim_0" if k % 4 == 0 else "pim_1" if k % 4 == 1 else "pim_2" if k % 4 == 2 else "pim_3"
        }

for j in range(kv_tile):
    for k in range(head_tile):
        data_info[f"KV_{j}_{k}"] = {
            "shape": [min(kv_tile_size, N - j * kv_tile_size), head_tile_size * head_dim * 2],
            "dtype": "int8",
            "tag": "Input",
            "source": "pim_0" if k % 4 == 0 else "pim_1" if k % 4 == 1 else "pim_2" if k % 4 == 2 else "pim_3"
        }

for i in range(seq_tile):
    for k in range(head_tile):
        data_info[f"output_{i}_{k}"] = {
            "shape": [min(seq_tile_size, M - i * seq_tile_size), head_tile_size * head_dim],
            "dtype": "int8",
            "tag": "Output",
            "destination": "pim_0" if k % 4 == 0 else "pim_1" if k % 4 == 1 else "pim_2" if k % 4 == 2 else "pim_3"
        }

task_info = {}
for k in range(head_tile):
    for i in range(seq_tile):
        for j in range(kv_tile):
            key = f"QKV_{k}_{i}_{j}"
            task_info[key] = {
                "type": "Attention",
                "seq_length": seq_tile_size,
                "head_num": head_tile_size,
                "head_dim": head_dim,
                "kv_size": kv_tile_size,
                "inputs": [f"Q_{i}_{k}"] + [f"KV_{j}_{k}"],
                "outputs": [f"output_{i}_{k}"],
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
