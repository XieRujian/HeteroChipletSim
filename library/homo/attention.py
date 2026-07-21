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
            config, f"Q_{i}_{j}_{0}", f"node_{i}_{j}", 0)
        add_task(
            config, f"Q_{i}_{j}_{1}", f"node_{i}_{j}", 0)
        add_task(
            config, f"KV_{i}_{j}_{0}", f"node_{i}_{j}", 1)
        add_task(
            config, f"KV_{i}_{j}_{1}", f"node_{i}_{j}", 1)
        add_task(
            config, f"KV_{i}_{j}_{2}", f"node_{i}_{j}", 2)
        add_task(
            config, f"KV_{i}_{j}_{3}", f"node_{i}_{j}", 2)

for i in range(2):
    add_task(
        config, f"collectKV_{i}_{0}", f"node_{i * 2}_{0}", 4)
    add_task(
        config, f"collectKV_{i}_{1}", f"node_{i * 2}_{1}", 4)
    add_task(
        config, f"collectKV_{i}_{2}", f"node_{i * 2}_{2}", 4)
    add_task(
        config, f"collectKV_{i}_{3}", f"node_{i * 2}_{3}", 4)
    add_task(
        config, f"collectQ_{i}_{0}", f"node_{i * 2 + 1}_{0}", 4)
    add_task(
        config, f"collectQ_{i}_{1}", f"node_{i * 2 + 1}_{1}", 4)
    add_task(
        config, f"collectQ_{i}_{2}", f"node_{i * 2 + 1}_{2}", 4)
    add_task(
        config, f"collectQ_{i}_{3}", f"node_{i * 2 + 1}_{3}", 4)

for k in range(4):
    for i in range(2):
        for j in range(2):
            add_task(
                config, f"QKV_{k}_{i}_{j}", f"node_{i*2+j}_{k}", 6)

with open("attention.ini", "w") as f:
    config.write(f)
