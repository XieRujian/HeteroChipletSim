import argparse
from configparser import ConfigParser


def add_task(config, task, node, time):
    config.add_section(task)
    config.set(task, "node", node)
    config.set(task, "time", str(time))

pim_nodes = ["node_0_2", "node_1_1", "node_2_2", "node_3_1", "node_1_3"]

config = ConfigParser()


with open("rt_de_ffn2.ini", "w") as f:
    config.write(f)
