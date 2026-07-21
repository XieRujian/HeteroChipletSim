import argparse
from configparser import ConfigParser


def add_task(config, task, node, time):
    config.add_section(task)
    config.set(task, "node", node)
    config.set(task, "time", str(time))


config = ConfigParser()

for i in range(4):
    for j in range(4):
        for k in range(16):
            x = k // 4
            y = k % 4
            add_task(
                config, f"gemm_{i}_{x}_{y * 4 + j}", f"node_{i}_{j}", k)

with open("ffn_1.ini", "w") as f:
    config.write(f)
