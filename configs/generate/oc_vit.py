# Example: python configs/generate/oc_vit.py --output_file configs/phase/vit.json --time_slot_types Forward Forward Normal Backward Backward Forward Normal Backward Backward Forward Normal Backward Backward --time_slot_cycles 0 0 7936 0 0 0 10368 0 0 0 10368 0 0 --schedule_begin 2 --schedule_end 10 --channel 3 32 96 192 384 --width 256 128 64 32 16 --height 256 128 64 32 16 --kernel_width 3 3 3 3 --kernel_height 3 3 3 3 --in_channel_parallel 1 1 2 4 --out_channel_parallel 1 1 2 4 --width_parallel 4 4 2 1 --height_parallel 4 4 2 1
from math import lcm, gcd
import argparse
import json


def get_parser():
    parser = argparse.ArgumentParser(
        description="Generate data for ViT task")
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

    parser.add_argument("--channel", type=int, nargs='+', required=True,
                        help="channel size for each layer of ViT")
    parser.add_argument("--width", type=int, nargs='+', required=True,
                        help="width for each layer of ViT")
    parser.add_argument("--height", type=int, nargs='+', required=True,
                        help="height for each layer of ViT")
    parser.add_argument("--kernel_width", type=int, nargs='+', required=True,
                        help="kernel width for each layer of ViT")
    parser.add_argument("--kernel_height", type=int, nargs='+', required=True,
                        help="kernel height for each layer of ViT")

    parser.add_argument('--in_channel_parallel', type=int, nargs='+', required=True,
                        help='in channel parallel size for each layer of ViT, provide a list of parallel sizes for each layer')
    parser.add_argument('--out_channel_parallel', type=int, nargs='+', required=True,
                        help='out channel parallel size for each layer of ViT, provide a list of parallel sizes for each layer')
    parser.add_argument("--width_parallel", type=int, nargs='+', required=True,
                        help="width parallel size for each layer of ViT, provide a list of parallel sizes for each layer")
    parser.add_argument("--height_parallel", type=int, nargs='+', required=True,
                        help="height parallel size for each layer of ViT, provide a list of parallel sizes for each layer")

    return parser


parser = get_parser()
args = parser.parse_args()

if len(args.time_slot_types) != len(args.time_slot_cycles):
    raise ValueError(
        "The length of time_slot_types and time_slot_cycles must be the same")

if len(args.channel) != len(args.width) or len(args.width) != len(args.height):
    raise ValueError(
        "The length of channel, width, and height must be the same")

if len(args.in_channel_parallel) != len(args.out_channel_parallel) or len(args.out_channel_parallel) != len(args.width_parallel) or len(args.width_parallel) != len(args.height_parallel):
    raise ValueError(
        "The length of in_channel_parallel, out_channel_parallel, width_parallel, and height_parallel must be the same")

if len(args.channel) != len(args.in_channel_parallel) + 1:
    raise ValueError(
        "The length of channel must be one more than the length of in_channel_parallel")

time_slot = []
for i in range(len(args.time_slot_types)):
    time_slot.append({
        "type": args.time_slot_types[i],
        "cycles": args.time_slot_cycles[i]
    })

data_info = {}

real_width_parallel = [args.width_parallel[0]]
real_height_parallel = [args.height_parallel[0]]
for i in range(len(args.in_channel_parallel)-1):
    real_width_parallel.append(
        lcm(args.width_parallel[i], args.width_parallel[i+1]))
    real_height_parallel.append(
        lcm(args.height_parallel[i], args.height_parallel[i+1]))
real_height_parallel.append(args.height_parallel[-1])
real_width_parallel.append(args.width_parallel[-1])

real_channel_parallel = [args.in_channel_parallel[0]]
for i in range(len(args.in_channel_parallel)-1):
    real_channel_parallel.append(
        lcm(args.out_channel_parallel[i], args.in_channel_parallel[i+1]))
real_channel_parallel.append(args.out_channel_parallel[-1])
nodes = [f"node_{i}_{j}" for i in range(4) for j in range(4)]

for i in range(real_channel_parallel[0]):
    for j in range(real_width_parallel[0]):
        for k in range(real_height_parallel[0]):
            data_info[f"input_0_{i}_{j}_{k}"] = {
                "shape": [args.channel[0] // real_channel_parallel[0], args.width[0] // real_width_parallel[0], args.height[0] // real_height_parallel[0]],
                "dtype": "int8",
                "tag": "Input",
                "source": "Storage"
            }

for l in range(1, len(args.in_channel_parallel)):
    if args.in_channel_parallel[l - 1] > 1:
        for i in range(real_channel_parallel[l]):
            for j in range(real_width_parallel[l]):
                for k in range(real_height_parallel[l]):
                    data_info[f"output_{l-1}_{i}_{j}_{k}"] = {
                        "shape": [args.channel[l] // real_channel_parallel[l], args.width[l] // real_width_parallel[l], args.height[l] // real_height_parallel[l]],
                        "dtype": "int8",
                        "tag": "Output",
                    }
    for i in range(real_channel_parallel[l]):
        for j in range(real_width_parallel[l]):
            for k in range(real_height_parallel[l]):
                data_info[f"input_{l}_{i}_{j}_{k}"] = {
                    "shape": [args.channel[l] // real_channel_parallel[l], args.width[l] // real_width_parallel[l], args.height[l] // real_height_parallel[l]],
                    "dtype": "int8",
                    "tag": "Input",
                }

l = len(args.in_channel_parallel)
id = 0
for i in range(real_channel_parallel[l]):
    for j in range(real_width_parallel[l]):
        for k in range(real_height_parallel[l]):
            data_info[f"output_{l-1}_{i}_{j}_{k}"] = {
                "shape": [args.channel[l] // real_channel_parallel[l], args.width[l] // real_width_parallel[l], args.height[l] // real_height_parallel[l]],
                "dtype": "int8",
                "tag": "Output",
                "destination": f"sa_{id % 16}"
            }
            id = id + 1

for l in range(len(args.in_channel_parallel)):
    for i in range(args.in_channel_parallel[l]):
        for j in range(args.out_channel_parallel[l]):
            data_info[f"weight_{l}_{i}_{j}"] = {
                "shape": [args.channel[l] // args.in_channel_parallel[l], args.channel[l+1] // args.out_channel_parallel[l], args.kernel_width[l], args.kernel_height[l]],
                "dtype": "int8",
                "tag": "Input",
                "source": "Storage"
            }

task_info = {}
for m in range(len(args.in_channel_parallel)):
    for i in range(args.in_channel_parallel[m]):
        for j in range(args.out_channel_parallel[m]):
            for k in range(args.width_parallel[m]):
                for l in range(args.height_parallel[m]):
                    key = f"conv_{m}_{i}_{j}_{k}_{l}"
                    task_info[key] = {
                        "type": "Conv",
                        "batch_size": 1,
                        "in_channels": args.channel[m] // args.in_channel_parallel[m],
                        "out_channels": args.channel[m+1] // args.out_channel_parallel[m],
                        "in_width": args.width[m] // args.width_parallel[m],
                        "in_height": args.height[m] // args.height_parallel[m],
                        "kernel_width": args.kernel_width[m],
                        "kernel_height": args.kernel_height[m],
                        "stride_width": 2,
                        "stride_height": 2,
                        "padding_width": 1,
                        "padding_height": 1,
                        "inputs": [f"input_{m}_{i_}_{k_}_{l_}"
                                   for l_ in range(l * (real_width_parallel[m] // args.width_parallel[m]), (l + 1) * (real_width_parallel[m] // args.width_parallel[m]))
                                   for k_ in range(k * (real_height_parallel[m] // args.height_parallel[m]), (k + 1) * (real_height_parallel[m] // args.height_parallel[m]))
                                   for i_ in range(i * (real_channel_parallel[m] // args.in_channel_parallel[m]), (i + 1) * (real_channel_parallel[m] // args.in_channel_parallel[m]))] + [f"weight_{m}_{i}_{j}"],
                        "outputs": [f"output_{m}_{j_}_{k_}_{l_}" if args.in_channel_parallel[m] > 1 else f"input_{m+1}_{j_}_{k_}_{l_}"
                                    for l_ in range(l * (real_width_parallel[m+1] // args.width_parallel[m]), (l + 1) * (real_width_parallel[m+1] // args.width_parallel[m]))
                                    for k_ in range(k * (real_height_parallel[m+1] // args.height_parallel[m]), (k + 1) * (real_height_parallel[m+1] // args.height_parallel[m]))
                                    for j_ in range(j * (real_channel_parallel[m+1] // args.out_channel_parallel[m]), (j + 1) * (real_channel_parallel[m+1] // args.out_channel_parallel[m]))],
                        "shared_memory_byte": 0
                    }
    if args.in_channel_parallel[m] > 1 and m != len(args.in_channel_parallel) - 1:
        for j in range(real_channel_parallel[m+1]):
            for k in range(real_width_parallel[m+1]):
                for l in range(real_height_parallel[m+1]):
                    key = f"collect_{m}_{j}_{k}_{l}"
                    task_info[key] = {
                        "type": "Collect",
                        "inputs": [f"output_{m}_{j}_{k}_{l}"],
                        "outputs": [f"input_{m+1}_{j}_{k}_{l}"],
                        "shared_memory_byte": (args.channel[m+1] // real_channel_parallel[m+1]) * (args.width[m+1] // real_width_parallel[m+1]) * (args.height[m+1] // real_height_parallel[m+1])
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
