import argparse
from configparser import ConfigParser


def add_task(config, task, node, time):
    config.add_section(task)
    config.set(task, "node", node)
    config.set(task, "time", str(time))


config = ConfigParser()
block = [[0, 1, 4, 5], [2, 3, 6, 7], [10, 11, 14, 15], [8, 9, 12, 13]]
for j in range(4):
    for i in range(4):
        for k in range(6):
            x = k // 2
            y = k % 2
            add_task(
                config, f"gemm_{i}_{2 * x + j // 2}_{2 * y + (j % 2)}", f"node_{j}_{i}", k // 2)

with open("gemm__.ini", "w") as f:
    config.write(f)
