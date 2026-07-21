# Example: python configs/generate/mesh.py --output_file configs/topology/mesh_4.json --mesh_row 4 --mesh_col 4 --bandwidth 32
import argparse
import json


def get_parser():
    parser = argparse.ArgumentParser(description="Generate mesh topology")
    parser.add_argument("--output_file", type=str, required=True,
                        help="File to save the generated topology")

    parser.add_argument("--mesh_row", type=int, default=4,
                        help="row size of the mesh")
    parser.add_argument("--mesh_col", type=int, default=4,
                        help="column size of the mesh")
    parser.add_argument("--noc_bandwidth", type=int, default=256,
                        help="bandwidth of each link in GB/s")
    parser.add_argument("--dram_bandwidth", type=int, default=32,
                        help="bandwidth of DRAM links in GB/s")

    return parser


parser = get_parser()
args = parser.parse_args()
topology = {
    "nodes": [],
    "edges": {},
    "chain": []
}

for i in range(args.mesh_row):
    for j in range(args.mesh_col):
        topology["nodes"].append(f"node_{i}_{j}")

for i in range(args.mesh_row):
    for j in range(args.mesh_col - 1):
        topology["edges"][f"right_{i}_{j}"] = {
            "from": f"node_{i}_{j}",
            "to": f"node_{i}_{j+1}",
            "bandwidth": args.noc_bandwidth,
            "energy_pj_per_bit": 1
        }

for i in range(args.mesh_row):
    for j in range(args.mesh_col - 1):
        topology["edges"][f"left_{i}_{j+1}"] = {
            "from": f"node_{i}_{j+1}",
            "to": f"node_{i}_{j}",
            "bandwidth": args.noc_bandwidth,
            "energy_pj_per_bit": 1
        }

for i in range(args.mesh_row - 1):
    for j in range(args.mesh_col):
        topology["edges"][f"down_{i}_{j}"] = {
            "from": f"node_{i}_{j}",
            "to": f"node_{i+1}_{j}",
            "bandwidth": args.noc_bandwidth,
            "energy_pj_per_bit": 1
        }

for i in range(args.mesh_row - 1):
    for j in range(args.mesh_col):
        topology["edges"][f"up_{i+1}_{j}"] = {
            "from": f"node_{i+1}_{j}",
            "to": f"node_{i}_{j}",
            "bandwidth": args.noc_bandwidth,
            "energy_pj_per_bit": 1
        }

for i in range(args.mesh_row):
    topology["edges"][f"in_{i}_0"] = {
        "from": "dram",
        "to": f"node_{i}_0",
        "bandwidth": args.dram_bandwidth,
        "energy_pj_per_bit": 20.0
    }

for i in range(args.mesh_row):
    topology["edges"][f"in_{i}_{args.mesh_col - 1}"] = {
        "from": "dram",
        "to": f"node_{i}_{args.mesh_col - 1}",
        "bandwidth": args.dram_bandwidth,
        "energy_pj_per_bit": 20.0
    }

for i in range(args.mesh_row):
    topology["edges"][f"out_{i}_0"] = {
        "from": f"node_{i}_0",
        "to": "dram",
        "bandwidth": args.dram_bandwidth,
        "energy_pj_per_bit": 20.0
    }

for i in range(args.mesh_row):
    topology["edges"][f"out_{i}_{args.mesh_col - 1}"] = {
        "from": f"node_{i}_{args.mesh_col - 1}",
        "to": "dram",
        "bandwidth": args.dram_bandwidth,
        "energy_pj_per_bit": 20.0
    }

for i in range(args.mesh_row):
    for j in range(args.mesh_col):
        for k in range(j+1, args.mesh_col):
            topology["chain"].append({
                "from": f"node_{i}_{j}",
                "to": [f"node_{i}_{l}" for l in range(j+1, k+1)],
                "edges": [f"right_{i}_{l}" for l in range(j, k)]
            })

for i in range(args.mesh_row):
    for j in range(args.mesh_col):
        for k in range(j+1, args.mesh_col):
            topology["chain"].append({
                "from": f"node_{i}_{k}",
                "to": [f"node_{i}_{l}" for l in range(j, k)],
                "edges": [f"left_{i}_{l+1}" for l in range(j, k)]
            })

for i in range(args.mesh_col):
    for j in range(args.mesh_row):
        for k in range(j+1, args.mesh_row):
            topology["chain"].append({
                "from": f"node_{j}_{i}",
                "to": [f"node_{l}_{i}" for l in range(j+1, k+1)],
                "edges": [f"down_{l}_{i}" for l in range(j, k)]
            })

for i in range(args.mesh_col):
    for j in range(args.mesh_row):
        for k in range(j+1, args.mesh_row):
            topology["chain"].append({
                "from": f"node_{k}_{i}",
                "to": [f"node_{l}_{i}" for l in range(j, k)],
                "edges": [f"up_{l+1}_{i}" for l in range(j, k)]
            })

for i in range(args.mesh_row):
    for j in range(args.mesh_col):
        topology["chain"].append({
            "from": "dram",
            "to": [f"node_{i}_{0}"] + [f"node_{i}_{l+1}" for l in range(j)],
            "edges": [f"in_{i}_{0}"] + [f"right_{i}_{l}" for l in range(j)]
        })

for i in range(args.mesh_row):
    for j in range(args.mesh_col):
        topology["chain"].append({
            "from": "dram",
            "to": [f"node_{i}_{args.mesh_col - 1}"] + [f"node_{i}_{l}" for l in range(j, args.mesh_col - 1)],
            "edges": [f"in_{i}_{args.mesh_col - 1}"] + [f"left_{i}_{l+1}" for l in range(j, args.mesh_col - 1)]
        })

for i in range(args.mesh_row):
    for j in range(args.mesh_col):
        topology["chain"].append({
            "from": f"node_{i}_{j}",
            "to": ["dram"] + [f"node_{i}_{l}" for l in range(j)],
            "edges": [f"out_{i}_{0}"] + [f"left_{i}_{l+1}" for l in range(j)]
        })

for i in range(args.mesh_row):
    for j in range(args.mesh_col):
        topology["chain"].append({
            "from": f"node_{i}_{j}",
            "to": ["dram"] + [f"node_{i}_{l+1}" for l in range(j, args.mesh_col - 1)],
            "edges": [f"out_{i}_{args.mesh_col - 1}"] + [f"right_{i}_{l}" for l in range(j, args.mesh_col - 1)]
        })

topology["nodes"].append("dram")

with open(args.output_file, "w") as f:
    json.dump(topology, f, indent=4)