from os.path import join, isfile, basename
import itertools
import os
import sqlite3

from benchmarks.benchmark_exec_utils import run_benchmarks
from benchmarks.graphs import generate_graph_throughput_with_increasing_number_of_flows, \
    generate_graphs_depending_on_ruleset_size

DB_NAME = 'test_results.db'


def cartesian(*arrays):
    return itertools.product(*arrays)


def find_all_files(p):
    return [join(p, f) for f in os.listdir(p)
             if isfile(join(p, f))]


if __name__ == "__main__":
    conn = sqlite3.connect(DB_NAME)
    conn.execute('''CREATE TABLE IF NOT EXISTS test_result
                 (id INTEGER PRIMARY KEY AUTOINCREMENT,
                  timestamp INT,                   app_name TEXT,                  ruleset_name TEXT,
                  flow_cnt INT,                    packet_cnt INT,                 real_rule_cnt INT,
                  construction_time REAL,          number_of_tries_or_tables INT,  lookup_speed REAL,
                  minimal_benchmark_overhead REAL, packet_lookup_times BLOB
                 )''')
    conn.execute('''CREATE TABLE IF NOT EXISTS benchmark_execs
                 (id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp INT, revision TEXT, machine_name TEXT)''')
    conn.commit()

    # PARALLEL = True
    PARALLEL = False

    CLASSBENCH_ROOT = "../classbench-ng/generated/"
    MESON_BUILD_PATH = "build/meson.debug.linux.x86_64/benchmarks/"
    # (filename, requires sudo, repeat cnt)
    REPEATS = 1
    BENCHMARKS = [
        # (MESON_BUILD_PATH + "1_tree_lookup", False, REPEATS),
        (MESON_BUILD_PATH + "n_tree_lookup", False, REPEATS),
        # (MESON_BUILD_PATH + "dpdk/1_tree_lookup_dpdk", True, REPEATS),
        (MESON_BUILD_PATH + "ovs/ovs_pcv_lookup", False, REPEATS),
        (MESON_BUILD_PATH + "ovs/ovs_tss_lookup", False, REPEATS),
    ]

    FLOW_CNTS = [
        # 1,
        #16, 128,
        1024,
        #4096,
        #8192,
        #65536
    ]
    PACKET_CNTS = [
        # 10000,
        int(10e6),
        # 100000000,
    ]

    # rule_files = [
    #   '../classbench-ng/generated/acl3_5000',
    # ]
    # pprint(find_all_files(CLASSBENCH_ROOT))
    # sys.exit(1)
    def benchmark_nominal_size(file_name):
        tn = basename(file_name)
        tn = tn.split("_")[1]
        mul = tn[-1].lower()
        if mul in ["k", "m"]:
            rules = int(tn[:-1])
            if mul == "k":
                rules *= int(1e3)
            else:
                rules *= int(1e6)
        else:
            rules = int(tn)
        return rules

    rule_files = [
        f for f in find_all_files(CLASSBENCH_ROOT)
        if not f.endswith(".py")
           # this rule sets are consuming more than available memory in default mempool
           and not f.endswith("exact0_32k")
           and not f.endswith("ipc1_5000")
           and not f.endswith("wildcard")
           and benchmark_nominal_size(f) >= 5000
    ]

    # (number of flows, number of packets)
    TEST_ARGS = list(
        cartesian(BENCHMARKS, rule_files, FLOW_CNTS, PACKET_CNTS)
    )

    run_benchmarks(DB_NAME, TEST_ARGS, PARALLEL)

    generate_graph_throughput_with_increasing_number_of_flows(DB_NAME)
    generate_graphs_depending_on_ruleset_size(DB_NAME, FLOW_CNTS[-1], PACKET_CNTS[-1])
