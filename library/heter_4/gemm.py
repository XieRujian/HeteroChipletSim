import argparse
from configparser import ConfigParser


def add_task(config, task, node, time):
    config.add_section(task)
    config.set(task, "node", node)
    config.set(task, "time", str(time))


def add1(config, node: str, ms: list[int], ks: list[int], ns: list[int], exception: list[tuple[int, int, int]] = []):
    t = 0
    for m in ms:
        for k in ks:
            for n in ns:
                if (m, k, n) not in exception:
                    add_task(config, f"gemm_{m}_{k}_{n}", node, t // 2)
                    t += 1


config = ConfigParser()
add1(config, "node_0_0", [0, 1], [0, 1], [0, 1], [
     (0, 0, 1), (1, 0, 0), (1, 0, 1)])
add1(config, "node_0_3", [2, 3], [0, 1], [0, 1], [
     (2, 0, 0), (3, 0, 0), (3, 0, 1)])
add1(config, "node_3_0", [0, 1], [0, 1], [
     2, 3], [(0, 0, 3), (1, 0, 2), (1, 0, 3)])
add1(config, "node_3_3", [2, 3], [0, 1], [
     2, 3], [(2, 0, 2), (3, 0, 2), (3, 0, 3)])

add1(config, "node_0_1", [0, 1], [4, 5], [0, 1], [(0, 4, 0)])
add1(config, "node_1_0", [0, 1], [2, 3], [0, 1], [(0, 2, 0)])
add_task(config, "gemm_0_4_0", "node_0_0", 2)
add_task(config, "gemm_0_2_0", "node_0_0", 3)

add1(config, "node_0_2", [2, 3], [4, 5], [0, 1], [(2, 4, 0)])
add1(config, "node_1_3", [2, 3], [2, 3], [0, 1], [(2, 2, 0)])
add_task(config, "gemm_2_4_0", "node_0_3", 2)
add_task(config, "gemm_2_2_0", "node_0_3", 3)

add1(config, "node_3_2", [2, 3], [4, 5], [2, 3], [(2, 4, 2)])
add1(config, "node_2_3", [2, 3], [2, 3], [2, 3], [(2, 2, 2)])
add_task(config, "gemm_2_4_2", "node_3_3", 2)
add_task(config, "gemm_2_2_2", "node_3_3", 3)

add1(config, "node_3_1", [0, 1], [4, 5], [2, 3], [(0, 4, 2)])
add1(config, "node_2_0", [0, 1], [2, 3], [2, 3], [(0, 2, 2)])
add_task(config, "gemm_0_4_2", "node_3_0", 2)
add_task(config, "gemm_0_2_2", "node_3_0", 3)

add_task(config, "gemm_1_0_0", "node_1_1", 0)
add_task(config, "gemm_2_0_0", "node_1_1", 1)
add_task(config, "gemm_3_0_0", "node_1_1", 2)

add_task(config, "gemm_0_0_1", "node_1_2", 0)
add_task(config, "gemm_1_0_1", "node_1_2", 1)
add_task(config, "gemm_3_0_1", "node_1_2", 2)

add_task(config, "gemm_1_0_2", "node_2_1", 0)
add_task(config, "gemm_2_0_2", "node_2_1", 1)
add_task(config, "gemm_3_0_2", "node_2_1", 2)

add_task(config, "gemm_0_0_3", "node_2_2", 0)
add_task(config, "gemm_1_0_3", "node_2_2", 1)
add_task(config, "gemm_3_0_3", "node_2_2", 2)

with open("gemm_4_0.ini", "w") as f:
    config.write(f)
