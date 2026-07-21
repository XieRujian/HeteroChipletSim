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
            config, f"QKV_{j * 2 + (i % 2)}_{0}_{(i // 2) * 2}", f"node_{i}_{j}", 0)
        add_task(
            config, f"QKV_{j * 2 + (i % 2)}_{0}_{(i // 2) * 2 + 1}", f"node_{i}_{j}", 0)

with open("rt_en_self.ini", "w") as f:
    config.write(f)
