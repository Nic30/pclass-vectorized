import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from typing import Tuple
import json
from matplotlib.ticker import MaxNLocator
from matplotlib import cm
from subprocess import check_output
import os
from cmath import isnan, nan
from builtins import staticmethod
from math import log, ceil, floor
from pprint import pprint


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

    def print_nodes(self, nodes, node_id, width, height):
        total_size = (height, width)
        overlaps = np.empty(total_size)
        overlaps[:] = 0
        img = np.empty(total_size)
        img[:] = np.nan
        self._print_nodes(nodes, node_id, img, overlaps)
        # [todo] do not know how to reliably handle NaN and 0 division in numpy
        res = [[
            i / o if o > 0 else nan for (i, o) in zip(cimg, coverlap)
            ] for cimg, coverlap in zip(img, overlaps)]
        return  np.array(res)

    def _print_nodes(self, nodes, node_id, img, overlaps):
        n = nodes[node_id]
        x = floor(n["x"] + n["width"] / 2)
        val = (n["key_cnt"] * 2
               +len(n["children"])
               +len(n["next_layer"])) / (2 * 8 + 9 + 8)
        y = n["y"]

        prev_val = img[y][x]
        if isnan(prev_val):
            img[y][x] = val
        else:
            img[y][x] += val
        overlaps[y][x] += 1

        for c in n["children"]:
            self._print_nodes(nodes, c, img, overlaps)
        for c in n["next_layer"]:
            self._print_nodes(nodes, c, img, overlaps)

    def to_log_width(self, nodes, node_id, log_base):
        """
        Resize the image to with logarithmical X axis
        Use average values a substitution
        """
        n = nodes[node_id]
        x = n["x"]
        if x < log_base:
            x = 0
        else:
            x = log(x, log_base)
        x = floor(x)
        n["x"] = x

        x_start = x
        x_end = x_start
        for c in n["children"]:
            w = self.to_log_width(nodes, c, log_base)
            x_end = nodes[c]["x"] + w

        for c in n["next_layer"]:
            w = self.to_log_width(nodes, c, log_base)
            x_end = nodes[c]["x"] + w

        width = n["width"] = max(1, x_end - x_start)
        return width

    def generate_heatmap(self):
        imgs = []
        for t in self.trees:
            root_id = t["root"]
            nodes = t["nodes"]
            depth = self.resolve_y(nodes, root_id, 0)
            self.resolve_x(nodes, root_id, 0)
            # width = root["width"]
            width = self.to_log_width(nodes, root_id, log_base=2)
            # pprint(nodes)

            #print(width)
            # start of the tree on X axis (left corner)
            img = self.print_nodes(nodes, root_id, width + 1, depth + 1)
            imgs.append(img)

        return imgs


def add_colorbar(fig, ax, last_img, cmap):
    # https://stackoverflow.com/questions/13784201/matplotlib-2-subplots-1-colorbar
    # # normalize colors to minimum and maximum values of dataset
    norm = matplotlib.colors.Normalize(vmin=0, vmax=1)
    ncontours = 10
    cmap = cm.get_cmap(cmap, ncontours)  # number of colors on colorbar
    m = cm.ScalarMappable(cmap=cmap, norm=norm)
    m.set_array([])
    #im_ratio = last_img.shape[0] / last_img.shape[1]
    cbar_top = fig.colorbar(m, #ax=ax,
                            orientation='vertical',
                            #fraction=5 * im_ratio,
                            shrink=0.75, pad=0.2)  # , cax=cax_top)
    cbar_top.set_ticks(np.linspace(0, 1, ncontours))


def main(data):
    gen = HeatMapGen(data)
    imgs = gen.generate_heatmap()
    # pprint(gen.trees)
    # print(img)
    #plots = len(gen.trees)
    plots = 3
    # x_max = max(len(img[0]) for img in imgs)
    y_max = max(len(img) for img in imgs)

    #  * -> y
    #  |
    #  v
    #  X

    fig, axs = plt.subplots(1, plots, sharey="row")
    if plots == 1:
        axs = (axs,)

    # We want to show all ticks...
    # ax.set_xticks(np.arange(len(img)))
    # ax.set_yticks(np.arange(len(img[0])))
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
    # cbar = fig.colorbar(im, ax=ax, **cbar_kw)
    # cbar.ax.set_ylabel(cbarlabel, rotation=-90, va="bottom")
    cmap = "plasma"
    for ax, img in zip(axs, imgs):
        # ax.yaxis.set_major_locator(MaxNLocator(integer=True))
        ax.set_ylim((0, y_max))
        # ax.xaxis.set_major_locator(MaxNLocator(integer=True))
        # ax.xaxis.set_ticks([])
        # ax.set_xscale("log")
        im = ax.imshow(img,
                        cmap=cmap,
                       # cmap="YlGn",
                        origin='lower', aspect="auto", vmin=0, vmax=1,
                        )

    add_colorbar(fig, axs[-1], imgs[-1], cmap)
    # fig.set_title("Memory usage efficiency")
    fig.tight_layout()
    plt.legend()
    # plt.show()
    plt.savefig('../fig/mem_efficiency_heatmap.png')



ROOT = os.path.join(os.path.dirname(__file__), "..")
APP = os.path.join(ROOT, "build/meson.debug.linux.x86_64/benchmarks/pcv_mem_efficiency_report")


def run_analysis(file):
    j = check_output([APP, file])
    return json.loads(j)


if __name__ == "__main__":
    #trees_example = [
    #    {
    #        "root": 0,
    #        "nodes": {
    #            0: {
    #                "children": [1, 2],  # List[int]
    #                "next_layer": [],  # List[int]
    #                "key_cnt": 2,
    #            },
    #            1: {
    #                "children": [],  # List[int]
    #                "next_layer": [],  # List[int]
    #                "key_cnt": 8,
    #            },
    #            2: {
    #                "children": [],  # List[int]
    #                "next_layer": [],  # List[int]
    #                "key_cnt": 16,
    #            }
    #        }
    #    }
    #]
    # fname = "../fw2_500.json"
    fname = os.path.join(ROOT, "../classbench-ng/generated",
                         #"acl1_100"
                         #"acl2_100"
                         "acl1_5000"
                         # "fw2_5000"
                         #"exact0_1024"
                         #"exact0_8192"
                         )
    trees = run_analysis(fname)
    # trees = trees_example
    main(trees)
