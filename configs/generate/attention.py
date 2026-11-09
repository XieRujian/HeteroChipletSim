# Example: python configs/generate/attention.py --output_file configs/phase/attention.json --time_slot_types Forward Forward Normal Normal Normal Backward Backward Forward Normal Backward Backward --time_slot_cycles 0 0 27648 27648 27648 0 0 0 55296 0 0 --schedule_begin 2 --schedule_end 9 --hidden 768 --num_heads 12 --seq_parallel_len 192 192 192 192 --seq_parallel_node node_0_1 node_1_1 node_2_1 node_3_1 --row_parallel 4 --w_col_parallel_len 384 --seq_parallel_group_1 2 --seq_parallel_group_2 2 --head_parallel 4
import argparse
import json


def get_parser():
    parser = argparse.ArgumentParser(
        description="Generate data for Attention task")
    parser.add_argument("--output_file", type=str, required=True,
                        help="File to save generated data")

    parser.add_argument("--batch", type=int, default=1,
                        help="batch for Attention")
    parser.add_argument("--seq_len", type=int, required=True,
                        help="seq_len for Attention")
    parser.add_argument("--kv_size", type=int, required=True,
                        help="kv_size for Attention")
    parser.add_argument("--head_dim", type=int, required=True,
                        help="Parallel size for Attention")
    parser.add_argument("--head_num", type=int, required=True,
                        help="Number of heads for Attention")

    parser.add_argument('--seq_tile_size', type=int, required=True,
                        help='Tile size for sequence parallelism in Attention')
    parser.add_argument("--head_tile_size", type=int, required=True,
                        help="Tile size for head parallelism in Attention")
    parser.add_argument("--kv_tile_size", type=int, required=True,
                        help="Tile size for row parallelism in Attention")

    parser.add_argument("--data_type", type=str, default="int8", choices=["int8", "fp16", "fp32"],
                        help="Data type of Attention")
    return parser


parser = get_parser()
args = parser.parse_args()

data_type = args.data_type
seq_len = args.seq_len
kv_size = args.kv_size
batch = args.batch
head_num = args.head_num
head_dim = args.head_dim

seq_tile_size = args.seq_tile_size
head_tile_size = args.head_tile_size
kv_tile_size = args.kv_tile_size
seq_tile = (seq_len + seq_tile_size - 1) // seq_tile_size
head_tile = head_num // head_tile_size
kv_tile = (kv_size + kv_tile_size - 1) // kv_tile_size

data_info = {}
for b in range(batch):
    for i in range(seq_tile):
        for k in range(head_tile):
            data_info[f"Q_{b}_{i}_{k}"] = {
                "shape": [min(seq_tile_size, seq_len - i * seq_tile_size), head_tile_size * head_dim],
                "dtype": data_type,
                "tag": "Input"
            }

for b in range(batch):
    for j in range(kv_tile):
        for k in range(head_tile):
            data_info[f"KV_{b}_{j}_{k}"] = {
                "shape": [min(kv_tile_size, kv_size - j * kv_tile_size), head_tile_size * head_dim * 2],
                "dtype": data_type,
                "tag": "Input"
            }

for b in range(batch):
    for i in range(seq_tile):
        for k in range(head_tile):
            data_info[f"output_{b}_{i}_{k}"] = {
                "shape": [min(seq_tile_size, seq_len - i * seq_tile_size), head_tile_size * head_dim],
                "dtype": data_type,
                "tag": "Output"
            }

task_info = {}
for b in range(batch):
    for k in range(head_tile):
        for i in range(seq_tile):
            for j in range(kv_tile):
                key = f"attention_{b}_{k}_{i}_{j}"
                task_info[key] = {
                    "type": "Attention",
                    "seq_length": seq_tile_size,
                    "head_num": head_tile_size,
                    "head_dim": head_dim,
                    "kv_size": kv_tile_size,
                    "inputs": [f"Q_{b}_{i}_{k}"] + [f"KV_{b}_{j}_{k}"],
                    "outputs": [f"output_{b}_{i}_{k}"],
                    "shared_memory_byte": 0
                }

json_data = {
    "data_blocks": data_info,
    "tasks": task_info
}

with open(args.output_file, 'w') as file:
    json.dump(json_data, file, indent=4)
