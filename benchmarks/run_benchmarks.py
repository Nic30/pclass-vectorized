from os.path import join, isfile, basename
import numpy as np
import itertools
import os, subprocess
import sqlite3
import sys
from pprint import pprint

from graphs import *
from benchmark_exec_utils import *

DB_NAME = 'test_results.db'


def cartesian(*arrays):
    return itertools.product(*arrays)


def find_all_files(p):
    return [join(p, f) for f in os.listdir(p)
             if isfile(join(p, f))]


if __name__ == "__main__":
    conn = sqlite3.connect(DB_NAME)
    conn.execute('''CREATE TABLE IF NOT EXISTS test_result
                 (timestamp int, app_name text, ruleset_name text, flow_cnt int, packet_cnt int, real_rule_cnt int,
                 construction_time real, number_or_tries_or_tables int, lookup_speed real)''')
    conn.execute('''CREATE TABLE IF NOT EXISTS benchmark_execs
                 (timestamp int, revision text, machine_name text)''')
    conn.commit()

    #PARALLEL = True 
    PARALLEL = False

    CLASSBENCH_ROOT = "../classbench-ng/generated/"
    MESON_BUILD_PATH = "build/meson.debug.linux.x86_64.1/benchmarks/"
    # (filename, requires sudo, repeat cnt)
    REPEATS = 4
    BENCHMARKS = [
        (MESON_BUILD_PATH + "1_tree_lookup", False, REPEATS),
        (MESON_BUILD_PATH + "n_tree_lookup", False, REPEATS),
        (MESON_BUILD_PATH + "dpdk/1_tree_lookup_dpdk", True, REPEATS),
    ]
    
    FLOW_CNTS = [
        # 1, 
        16, 128,
        1024, 4096, 8192,
        65536
    ]
    PACKET_CNTS = [
        10000000,
        # 100000000,
    ]
    #rule_files = [
    #   '../classbench-ng/generated/acl3_5000',
    #]
    # pprint(find_all_files(CLASSBENCH_ROOT))
    # sys.exit(1)
    rule_files = [f for f in find_all_files(CLASSBENCH_ROOT) if not f.endswith(".py")]
    
    # (number of flows, number of packets)
    TEST_ARGS = list(
        cartesian(BENCHMARKS, rule_files, FLOW_CNTS, PACKET_CNTS)
    )
    
    run_benchmarks(DB_NAME, TEST_ARGS, PARALLEL)
          
    generate_graph_troughput_with_increasing_number_of_flows(DB_NAME)
    generate_graphs_depending_on_ruleset_size(DB_NAME, 4096)
