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

    parser.add_argument("--in_height", type=int, required=True,
                        help="Input height for ViT task")
    parser.add_argument("--in_width", type=int, required=True,
                        help="Input width for ViT task")
    parser.add_argument("--in_channels", type=int, required=True,
                        help="Input channels for ViT task")
    parser.add_argument("--out_channels", type=int, required=True,
                        help="Output channels for ViT task")
    parser.add_argument("--width_tile_size", type=int, required=True,
                        help="Tile size of width for ViT task")
    parser.add_argument("--height_tile_size", type=int, required=True,
                        help="Tile size of height for ViT task")
    parser.add_argument("--in_channel_tile_size", type=int, required=True,
                        help="Tile size of input channels for ViT task")
    parser.add_argument("--out_channel_tile_size", type=int, required=True,
                        help="Tile size of output channels for ViT task")
    return parser


parser = get_parser()
args = parser.parse_args()

if len(args.time_slot_types) != len(args.time_slot_cycles):
    print(
        f"Length of time_slot_types: {len(args.time_slot_types)}, Length of time_slot_cycles: {len(args.time_slot_cycles)}")
    raise ValueError(
        "The length of time_slot_types and time_slot_cycles must be the same")

# if len(args.seq_parallel_len) != len(args.seq_parallel_node):
#    raise ValueError(
#        "The length of seq_parallel_len and seq_parallel_node must be the same")

time_slot = []
for i in range(len(args.time_slot_types)):
    time_slot.append({
        "type": args.time_slot_types[i],
        "cycles": args.time_slot_cycles[i]
    })

in_height = args.in_height
in_width = args.in_width
in_channels = args.in_channels
out_height = args.in_height // 2
out_width = args.in_width // 2
out_channels = args.out_channels
width_tile_size = args.width_tile_size
height_tile_size = args.height_tile_size
in_channel_tile_size = args.in_channel_tile_size
out_channel_tile_size = args.out_channel_tile_size
in_channel_tile = (in_channels + in_channel_tile_size -
                   1) // in_channel_tile_size
width_tile = (out_width + width_tile_size - 1) // width_tile_size
height_tile = (out_height + height_tile_size - 1) // height_tile_size
out_channel_tile = (out_channels + out_channel_tile_size -
                    1) // out_channel_tile_size

data_info = {}
for i in range(width_tile):
    for j in range(height_tile):
        for x in range(in_channel_tile):
            data_info[f"input_{i}_{j}_{x}"] = {
                "shape": [min(width_tile_size * 2, in_width - i * width_tile_size * 2), min(height_tile_size * 2, in_height - j * height_tile_size * 2), min(in_channel_tile_size, in_channels - x * in_channel_tile_size)],
                "dtype": "int8",
                "tag": "Input",
                "source": f"Storage"
            }

for x in range(in_channel_tile):
    for y in range(out_channel_tile):
        data_info[f"weight_{x}_{y}"] = {
            "shape": [3, 3, min(in_channel_tile_size, in_channels - x * in_channel_tile_size), min(out_channel_tile_size, out_channels - y * out_channel_tile_size)],
            "dtype": "int8",
            "tag": "Input",
            "source": "Storage"
        }

for i in range(width_tile):
    for j in range(height_tile):
        for y in range(out_channel_tile):
            data_info[f"output_{i}_{j}_{y}"] = {
                "shape": [min(width_tile_size, out_width - i * width_tile_size), min(height_tile_size, out_height - j * height_tile_size), min(out_channel_tile_size, out_channels - y * out_channel_tile_size)],
                "dtype": "int8",
                "tag": "Output",
                "destination": "pim_0"
            }

task_info = {}
for i in range(width_tile):
    for j in range(height_tile):
        for x in range(in_channel_tile):
            for y in range(out_channel_tile):
                key = f"conv_{i}_{j}_{x}_{y}"
                task_info[key] = {
                    "type": "Conv",
                    "batch_size": 1,
                    "in_channels": min(in_channel_tile_size, in_channels - x * in_channel_tile_size),
                    "out_channels": min(out_channel_tile_size, out_channels - y * out_channel_tile_size),
                    "in_width": min(width_tile_size * 2, in_width - i * width_tile_size * 2),
                    "in_height": min(height_tile_size * 2, in_height - j * height_tile_size * 2),
                    "kernel_width": 3,
                    "kernel_height": 3,
                    "stride_width": 2,
                    "stride_height": 2,
                    "padding_width": 1,
                    "padding_height": 1,
                    "inputs": [f"input_{i}_{j}_{x}"] + [f"weight_{x}_{y}"],
                    "outputs": [f"output_{i}_{j}_{y}"],
                    "shared_memory_byte": 0
                }

json_data = {
    "time_slot": time_slot,
    "schedule_begin": args.schedule_begin,
    "schedule_end": args.schedule_end,
    "data_blocks": data_info,
    "tasks": task_info,
    "compute_omega": 100
}

with open(args.output_file, 'w') as file:
    json.dump(json_data, file, indent=4)
