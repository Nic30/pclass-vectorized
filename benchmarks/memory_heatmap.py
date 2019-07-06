import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from typing import Tuple
from pprint import pprint
import json


class HeatMapGen():

    def __init__(self, trees):
        self.trees = trees

    def resolve_y(self, nodes, node_id, y):
        # the longer path from the root the larger y
        n = nodes[node_id]
        n["y"] = y
        y_max = y
        for c in n["children"]:
            _y = self.resolve_y(nodes, c, y + 1)
            y_max = max(y_max, _y)
        for c in n["next_layer"]:
            _y = self.resolve_y(nodes, c, y + 1)
            y_max = max(y_max, _y)
        return y_max

    def translate_subtree_x(self, t, root_id, x):
        n = t[root_id]
        n["x"] += x
        for c in n["children"]:
            self.translate_subtree_x(t, c, x)
        for c in n["next_layer"]:
            self.translate_subtree_x(t, c, x)

    def resolve_x(self, nodes, node_id, x_offset) -> Tuple[int, int]:
        """
        x position in tree
        """
        x = x_offset
        width = 0
        n = nodes[node_id]
        for c in n["children"]:
            _width = self.resolve_x(nodes, c, x_offset + width)
            width += _width
        for c in n["next_layer"]:
            _width = self.resolve_x(nodes, c, x_offset + width)
            width += _width

        n["x"] = x
        n["width"] = width = max(width, 1)
        return width

    def print_nodes(self, nodes, node_id, img):
        n = nodes[node_id]
        x = n["x"] + n["width"] // 2
        val = (n["key_cnt"] + len(n["children"]) + len(n["next_layer"])) / 24
        img[n["y"]][x] = val
        for c in n["children"]:
            self.print_nodes(nodes, c, img)
        for c in n["next_layer"]:
            self.print_nodes(nodes, c, img)

    def generate_heatmap(self):
        x_offset = 0
        y_max = 0
        x_padding = 2
        for t in self.trees:
            root_id = t["root"]
            nodes = t["nodes"]
            depth = self.resolve_y(nodes, root_id, 0)
            y_max = max(y_max, depth)
            self.resolve_x(nodes, root_id, x_offset)
            root = nodes[root_id]
            x_offset += root["width"] + x_padding

        total_size = (y_max + 1, x_offset + 1)
        img = np.empty(total_size)
        img[:] = np.nan
        # start of the tree on X axis (left corner)
        for t in self.trees:
            self.print_nodes(t["nodes"],
                             t["root"], img)

        return img


if __name__ == "__main__":
    trees_example = [
        {
            "root": 0,
            "nodes": {
                0: {
                    "children": [1, 2],  # List[int]
                    "next_layer": [],  # List[int]
                    "key_cnt": 2,
                },
                1: {
                    "children": [],  # List[int]
                    "next_layer": [],  # List[int]
                    "key_cnt": 8,
                },
                2: {
                    "children": [],  # List[int]
                    "next_layer": [],  # List[int]
                    "key_cnt": 16,
                }
            }
        }
    ]
    with open("../acl1_100.json") as f:
        trees = json.load(f)
    gen = HeatMapGen(trees)
    img = gen.generate_heatmap()
    pprint(gen.trees)
    print(img)
    fig, ax = plt.subplots()

    # We want to show all ticks...
    ax.set_xticks(np.arange(len(img)))
    ax.set_yticks(np.arange(len(img[0])))
    # ... and label them with the respective list entries
    # ax.set_xticklabels(farmers)
    # ax.set_yticklabels(vegetables)

    # Rotate the tick labels and set their alignment.
    # plt.setp(ax.get_xticklabels(), rotation=45, ha="right",
    #         rotation_mode="anchor")
    #
    # Loop over data dimensions and create text annotations.
    # for i in range(len(vegetables)):
    #    for j in range(len(farmers)):
    #        text = ax.text(j, i, harvest[i, j],
    #                       ha="center", va="center", color="w")
    #
    im = ax.imshow(img, origin='lower')

    ax.set_title("Memory usage efficiency")
    fig.tight_layout()
    plt.show()
