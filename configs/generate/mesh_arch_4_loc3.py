import argparse
from configparser import ConfigParser


def get_parser():
    parser = argparse.ArgumentParser(description="Generate mesh topology")
    parser.add_argument("--output_file", type=str, required=True,
                        help="File to save the generated topology")

    parser.add_argument("--mesh_row", type=int, default=4,
                        help="row size of the mesh")
    parser.add_argument("--mesh_col", type=int, default=4,
                        help="column size of the mesh")
    parser.add_argument("--buffer_size", type=int, default=64,
                        help="buffer size of each node in KB")

    return parser


parser = get_parser()
args = parser.parse_args()

config = ConfigParser()

config.add_section(f"Storage")
config.set(f"Storage", "node", "dram")
config.set(f"Storage", "type", "Storage")
config.set(f"Storage", "movable", "false")

pim_count = 0
sa_count = 0

for i in range(args.mesh_row):
    for j in range(args.mesh_col):
        if i == 0 and j == 0 or i == 0 and j == 3 or i == 3 and j == 0 or i == 3 and j == 3:
            config.add_section(f"pim_{pim_count}")
            config.set(f"pim_{pim_count}", "node", f"node_{i}_{j}")
            config.set(f"pim_{pim_count}", "type", "PIM")
            config.set(f"pim_{pim_count}", "buffer_size", str(4096))
            config.set(f"pim_{pim_count}", "bank_num", str(32))
            config.set(f"pim_{pim_count}", "frequency_ghz", str(0.4))
            config.set(f"pim_{pim_count}", "rows_per_subarray", str(256))
            config.set(f"pim_{pim_count}", "bit_per_row", str(1024))
            config.set(f"pim_{pim_count}", "mac_per_bank", str(32))
            config.set(f"pim_{pim_count}", "global_bandwidth_bytes_per_cycle", str(64))
            config.set(f"pim_{pim_count}", "local_rf_depth", str(16))
            config.set(f"pim_{pim_count}", "movable", "true")
            pim_count += 1
        else:
            config.add_section(f"sa_{sa_count}")
            config.set(f"sa_{sa_count}", "node", f"node_{i}_{j}")
            config.set(f"sa_{sa_count}", "type", "SystolicArray")
            config.set(f"sa_{sa_count}", "buffer_size", str(args.buffer_size))
            config.set(f"sa_{sa_count}", "frequency_ghz", str(1.0))
            config.set(f"sa_{sa_count}", "row", str(16))
            config.set(f"sa_{sa_count}", "col", str(16))
            config.set(f"sa_{sa_count}", "num_array", str(4))
            config.set(f"sa_{sa_count}", "movable", "true")
            sa_count += 1

with open(args.output_file, "w") as f:
    config.write(f)
