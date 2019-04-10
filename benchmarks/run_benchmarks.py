from os.path import join, isfile, basename
import numpy as np
import itertools
import os, subprocess
import sqlite3
import json
import time
import socket
import matplotlib.pyplot as plt


DB_NAME = 'test_results.db'


def cartesian(*arrays):
    return itertools.product(*arrays)


def find_all_files(p):
    return [join(p, f) for f in os.listdir(p)
             if isfile(join(p, f))]


def get_last_exec_time(db_conn):
    res = db_conn.execute('SELECT MAX(timestamp) FROM benchmark_execs')
    print(res)
    print(db_conn.fetchone())


def generate_graph_troughput_with_increasing_number_of_flows():
    c = sqlite3.connect(DB_NAME)
    last_exec = 0
    #last_exec = get_last_exec_time(c)
    
    query = "SELECT * from test_result WHERE timestamp >= ? ORDER BY flow_cnt"
    flow_cnt_values = [] # x 
    app_namesXruleset_troughput = {} # name of series :  lists of troughtputs for different rulesetns (y)
    last_packet_cnt = None
    for _, app_name, ruleset_name, flow_cnt, packet_cnt, lookup_speed in c.execute(query, (last_exec, )):
        assert last_packet_cnt == None or packet_cnt == last_packet_cnt, (packet_cnt, last_packet_cnt)
        last_packet_cnt = packet_cnt

        app_namesXruleset = "%s_%s" % (basename(app_name), basename(ruleset_name))
        troughput = app_namesXruleset_troughput.get(app_namesXruleset, [])
        app_namesXruleset_troughput[app_namesXruleset] = troughput

        troughput.append(lookup_speed)
        
        if not flow_cnt_values or flow_cnt_values[-1] != flow_cnt:
            flow_cnt_values.append(flow_cnt)

    fig, ax = plt.subplots()
    
    for name, troughputs in app_namesXruleset_troughput.items():
        x = flow_cnt_values
        y = troughputs
        line1, = ax.plot(x, y, label=name)
    ax.set_ylabel('Throughput [MPkt/s]')
    ax.set_xlabel('Flow count')
    ax.legend()
    plt.show()

def generate_graph_troughput_with_increasing_number_of_rules():
    pass


def build_test_name(app_name, rule_file, flow_cnt, packet_cnt):
    return "%s_%s_%d_%d" % (basename(app_name), basename(rule_file), flow_cnt, packet_cnt)


def exec_benchmark(app_name, repetition_cnt, require_sudo, rule_file, flow_cnt, packet_cnt):
    cmd = [app_name, rule_file, str(flow_cnt), str(packet_cnt), "1"]
    lookup_speed = 0.0
    for _ in range(repetition_cnt):
        res = subprocess.check_output(cmd)
        res = json.loads(res)
        lookup_speed += float(res["lookup_speed"])
    lookup_speed /= repetition_cnt

    conn = sqlite3.connect(DB_NAME)
    conn.execute("INSERT INTO test_result VALUES (?,?,?,?,?,?)",
              (time.time(), app_name, rule_file, flow_cnt, packet_cnt, lookup_speed))
    conn.commit()


def get_repo_rev():
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip()


if __name__ == "__main__":
    conn = sqlite3.connect(DB_NAME)
    conn.execute('''CREATE TABLE IF NOT EXISTS test_result
                 (timestamp int, app_name text, ruleset_name text, flow_cnt int, packet_cnt int, lookup_speed real)''')
    conn.execute('''CREATE TABLE IF NOT EXISTS benchmark_execs
                 (timestamp int, revision text, machine_name text)''')
    conn.execute("INSERT INTO benchmark_execs VALUES (?,?,?)",
              (time.time(), get_repo_rev(), socket.gethostname()))

    conn.commit()

    PARALLEL = True 
    # PARALLEL = False

    CLASSBENCH_ROOT = "../classbench-ng/generated/"
    MESON_BUILD_PATH = "build/meson.debug.linux.x86_64.1/benchmarks/"
    # (filename, requires sudo)
    BENCHMARKS = [
        (MESON_BUILD_PATH + "1_tree_lookup", False, 4),
        # (MESON_BUILD_PATH + "dpdk/pcv_dpdk", True, 4),
    ]
    
    FLOW_CNTS = [
        1,16,128,
        1024, 4096,
        65536
    ]
    PACKET_CNTS = [
        #10000000,
        100000000,
    ]
    rule_files = find_all_files(CLASSBENCH_ROOT)
    # rule_files = ["acl1_100"]
    
    # (number of flows, number of packets)
    TEST_ARGS = list(
        cartesian(BENCHMARKS, rule_files, FLOW_CNTS, PACKET_CNTS)
    )
    requires_sudo = False
    for (_, req, _), _, _, _ in TEST_ARGS:
        if req:
            requires_sudo = True
            break
    
    if requires_sudo:
        user = os.getenv("SUDO_USER")
        if user is None:
            print("Some tests require 'sudo'")
            exit()
            
    #for (app_name, require_sudo, repetition_cnt), rule_file, flow_cnt, packet_cnt in TEST_ARGS:
    #    exec_benchmark(app_name, repetition_cnt, require_sudo, rule_file, flow_cnt, packet_cnt)
        
    generate_graph_troughput_with_increasing_number_of_flows()
