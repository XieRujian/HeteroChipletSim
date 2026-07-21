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
        add_task(
            config, f"gemm1_{i}_{j}_{0}", f"node_{i}_{j}", 0)
        add_task(
            config, f"gemm1_{i}_{j}_{1}", f"node_{i}_{j}", 0)
        add_task(
            config, f"gemm1_{i}_{j}_{2}", f"node_{i}_{j}", 1)
        add_task(
            config, f"gemm1_{i}_{j}_{3}", f"node_{i}_{j}", 1)
        add_task(
            config, f"gemm1_{i}_{j}_{4}", f"node_{i}_{j}", 2)
        add_task(
            config, f"gemm1_{i}_{j}_{5}", f"node_{i}_{j}", 2)
        add_task(
            config, f"gemm1_{i}_{j}_{6}", f"node_{i}_{j}", 3)
        add_task(
            config, f"gemm1_{i}_{j}_{7}", f"node_{i}_{j}", 3)

for i in range(4):
    add_task(
        config, f"collect1_{i}_{0}", f"node_{i}_{0}", 4)
    add_task(
        config, f"collect1_{i}_{1}", f"node_{i}_{0}", 4)
    add_task(
        config, f"collect1_{i}_{2}", f"node_{i}_{1}", 4)
    add_task(
        config, f"collect1_{i}_{3}", f"node_{i}_{1}", 4)
    add_task(
        config, f"collect1_{i}_{4}", f"node_{i}_{2}", 4)
    add_task(
        config, f"collect1_{i}_{5}", f"node_{i}_{2}", 4)
    add_task(
        config, f"collect1_{i}_{6}", f"node_{i}_{3}", 4)
    add_task(
        config, f"collect1_{i}_{7}", f"node_{i}_{3}", 4)

for i in range(4):
    for j in range(4):
        add_task(
            config, f"gemm2_{i}_{0}_{j}", f"node_{i}_{0}", 4+j)
        add_task(
            config, f"gemm2_{i}_{1}_{j}", f"node_{i}_{0}", 4+j)
        add_task(
            config, f"gemm2_{i}_{2}_{j}", f"node_{i}_{1}", 4+j)
        add_task(
            config, f"gemm2_{i}_{3}_{j}", f"node_{i}_{1}", 4+j)
        add_task(
            config, f"gemm2_{i}_{4}_{j}", f"node_{i}_{2}", 4+j)
        add_task(
            config, f"gemm2_{i}_{5}_{j}", f"node_{i}_{2}", 4+j)
        add_task(
            config, f"gemm2_{i}_{6}_{j}", f"node_{i}_{3}", 4+j)
        add_task(
            config, f"gemm2_{i}_{7}_{j}", f"node_{i}_{3}", 4+j)

with open("ffn.ini", "w") as f:
    config.write(f)
