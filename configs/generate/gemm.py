# Example: python configs/generate/gemm.py --output_file configs/phase/gemm.json --m 256 --n 256 --k 384 --tile_size 64
import argparse
import json


def get_parser():
    parser = argparse.ArgumentParser(
        description="Generate data for GEMM task")
    parser.add_argument("--output_file", type=str, required=True,
                        help="File to save generated data")

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

    parser.add_argument("--data_type", type=str, default="int8", choices=["int8", "fp16", "fp32"],
                        help="Data type of GEMM")
    parser.add_argument("--gate", action="store_true",
                        help="Flag to indicate if gate is used in GEMM")
    return parser


parser = get_parser()
args = parser.parse_args()

data_type = args.data_type
gate = args.gate

M = args.m
N = args.n
K = args.k
M_tile_size = args.m_tile_size
N_tile_size = args.n_tile_size
K_tile_size = args.k_tile_size
M_tile = (M + M_tile_size - 1) // M_tile_size
N_tile = (N + N_tile_size - 1) // N_tile_size
K_tile = (K + K_tile_size - 1) // K_tile_size

data_info = {}
for i in range(M_tile):
    for j in range(K_tile):
        data_info[f"input_{i}_{j}"] = {
            "shape": [min(M_tile_size, M - i * M_tile_size), min(K_tile_size, K - j * K_tile_size)],
            "dtype": data_type,
            "tag": "Input"
        }

for j in range(K_tile):
    for k in range(N_tile):
        data_info[f"weight_{j}_{k}"] = {
            "shape": [min(K_tile_size, K - j * K_tile_size), min(N_tile_size, N - k * N_tile_size)],
            "dtype": data_type,
            "tag": "Weight"
        }

for i in range(M_tile):
    for k in range(N_tile):
        data_info[f"output_{i}_{k}"] = {
            "shape": [min(M_tile_size, M - i * M_tile_size), min(N_tile_size, N - k * N_tile_size) / 2 if gate else min(N_tile_size, N - k * N_tile_size)],
            "dtype": data_type,
            "tag": "Output"
        }

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

json_data = {
    "data_blocks": data_info,
    "tasks": task_info
}

with open(args.output_file, 'w') as file:
    json.dump(json_data, file, indent=4)
