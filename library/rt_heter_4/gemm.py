import argparse
from configparser import ConfigParser


def add_task(config, task, node, time):
    config.add_section(task)
    config.set(task, "node", node)
    config.set(task, "time", str(time))

sa_nodes=[
    ["node_0_0", "node_0_1", "node_0_3"],
    ["node_1_0", "node_1_2", "node_1_3"],
    ["node_2_0", "node_2_1", "node_2_3"],
    ["node_3_0", "node_3_2", "node_3_3"]
]

config = ConfigParser()
#for i in range(4):
#    for j in range(3):
#        for k in range(8):
#            add_task(
#                config, f"gemm_{0}_{k % 4}_{i*6+j*2+k//4}", sa_nodes[i][j], k // 2)

#for i in range(4):
#    for j in range(3):
#        for k in range(8):
#            add_task(
#                config, f"gemm_{0}_{k * 3 + j}_{i}", sa_nodes[i][j], k // 2)

#for i in range(4):
#    for j in range(3):
#        for k in range(8):
#            add_task(
#                config, f"gemm_{0}_{k % 4}_{i*8+j*2+k//4}", sa_nodes[i][j], k // 2)
#    add_task(
#        config, f"gemm_{0}_{0}_{i * 8 + 6}", sa_nodes[i][0], 4)
#    add_task(
#        config, f"gemm_{0}_{1}_{i * 8 + 6}", sa_nodes[i][1], 4)
#    add_task(
#        config, f"gemm_{0}_{2}_{i * 8 + 6}", sa_nodes[i][2], 4)
#    add_task(
#        config, f"gemm_{0}_{3}_{i * 8 + 6}", sa_nodes[i][0], 4)
#    add_task(
#        config, f"gemm_{0}_{0}_{i * 8 + 7}", sa_nodes[i][1], 4)
#    add_task(
#        config, f"gemm_{0}_{1}_{i * 8 + 7}", sa_nodes[i][2], 4)
#    add_task(
#        config, f"gemm_{0}_{2}_{i * 8 + 7}", sa_nodes[i][0], 5)
#    add_task(
#        config, f"gemm_{0}_{3}_{i * 8 + 7}", sa_nodes[i][1], 5)

#for i in range(4):
#    for j in range(32):
#        add_task(
#            config, f"gemm_{0}_{j % 16}_{i*2+j//16}", sa_nodes[i][j % 3], j // 6)

for i in range(4):
    for j in range(4):
        for k in range(8):
            add_task(
                config, f"gemm_{0}_{j * 4 + k % 4}_{i * 2 + k // 4}", f"node_{i}_{j}", k // 2)

# for i in range(4):
#    for j in range(4):
#        for k in range(8):
#            add_task(
#                config, f"gemm_{0}_{i * 8 + k}_{j}", f"node_{i}_{j}", k // 2)

# for i in range(4):
#    for j in range(4):
#        for k in range(8):
#            add_task(
#                config, f"gemm_{0}_{k * 2 + (i // 2)}_{j * 2 + (i % 2)}", f"node_{i}_{j}", k // 2)

#for i in range(4):
#    for j in range(4):
#        add_task(config, f"gemm_{0}_{i * 4 + (j // 2)}_{j}", f"node_{i}_{0}", j // 2)
#        add_task(config, f"gemm_{0}_{i * 4 + 2 + (j // 2)}_{j}", f"node_{i}_{3}", j // 2)

#for i in range(4):
#    for j in range(4):
#        for k in range(8):
#            add_task(config, f"gemm_{0}_{i * 4 + (k % 4)}_{j * 2 + (k // 4)}",
#                     f"node_{i}_{j}", k // 2)

with open("rt_de_ffn2.ini", "w") as f:
    config.write(f)
