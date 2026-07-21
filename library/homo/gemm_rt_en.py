import argparse
from configparser import ConfigParser


def add_task(config, task, node, time):
    config.add_section(task)
    config.set(task, "node", node)
    config.set(task, "time", str(time))


config = ConfigParser()
block = [[0, 1, 4, 5], [2, 3, 6, 7], [10, 11, 14, 15], [8, 9, 12, 13]]
for i in range(4):
    for j in range(4):
        for k in range(4):
            add_task(
                config, f"gemm_{0}_{k * 4 + i}_{j}", f"node_{j}_{i}", k)

with open("gemm_rt_en_2.ini", "w") as f:
    config.write(f)
