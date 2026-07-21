import argparse
from configparser import ConfigParser


def add_task(config, task, node, time):
    config.add_section(task)
    config.set(task, "node", node)
    config.set(task, "time", str(time))


config = ConfigParser()
#for i in range(4):
#    for j in range(4):
#        for k in range(6):
#            add_task(
#                config, f"gemm_{i}_{k}_{j}", f"node_{j}_{i}", k // 2)

for i in range(4):
    for j in range(4):
        for k in range(6):
            add_task(
                config, f"gemm_{0}_{j}_{k * 4 + i}", f"node_{i}_{j}", k // 2)

with open("rt_de_qkv.ini", "w") as f:
    config.write(f)
