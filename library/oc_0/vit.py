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
            config, f"conv_0_0_0_{i}_{j}", f"node_{i}_{j}", 0)

for i in range(4):
    for j in range(4):
        add_task(
            config, f"conv_1_0_0_{i}_{j}", f"node_{i}_{j}", 0)

for i in range(4):
    for j in range(4):
        add_task(
            config, f"conv_2_{i % 2}_{j % 2}_{i // 2}_{j // 2}", f"node_{i}_{j}", 4)

for i in range(4):
    add_task(
        config, f"collect_2_{(i % 2) * 2}_{i // 2}_{0}", f"node_{i}_{1}", 6)
    add_task(
        config, f"collect_2_{(i % 2) * 2 + 1}_{i // 2}_{0}", f"node_{i}_{1}", 6)
    add_task(
        config, f"collect_2_{(i % 2) * 2}_{i // 2}_{1}", f"node_{i}_{2}", 6)
    add_task(
        config, f"collect_2_{(i % 2) * 2 + 1}_{i // 2}_{1}", f"node_{i}_{2}", 6)

for i in range(4):
    for j in range(4):
        add_task(
            config, f"conv_3_{i}_{j}_0_0", f"node_{i}_{j}", 8)

with open("vit.ini", "w") as f:
    config.write(f)
