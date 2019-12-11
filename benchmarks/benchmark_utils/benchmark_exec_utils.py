import sqlite3
import subprocess
import multiprocessing
from multiprocessing import Pool
import time
import socket
import json
import os
from os.path import basename


def build_test_name(app_name, rule_file, flow_cnt, packet_cnt):
    return "%s_%s_%d_%d" % (basename(app_name), basename(rule_file), flow_cnt, packet_cnt)


def exec_benchmark(db_name, app_name, repetition_cnt, require_sudo, rule_file, flow_cnt, packet_cnt):
    # print((app_name, rule_file, flow_cnt, packet_cnt))

    is_dpkd = "dpdk" in app_name
    if is_dpkd:
        cmd = [app_name, "--", rule_file, str(flow_cnt), str(packet_cnt)]
    else:
        cmd = [app_name, rule_file, str(flow_cnt), str(packet_cnt)]

    lookup_speed = 0.0
    construction_time = 0.0
    real_rule_cnt = None
    number_of_tries_or_tables = None
    minimal_benchmark_overhead = 0
    packet_lookup_times = None

    for _ in range(repetition_cnt):
        res = subprocess.check_output(cmd)
        if is_dpkd:
            res = res[res.find(b"{") - 1:]
        res = json.loads(res)
        lookup_speed += float(res["lookup_speed"])
        construction_time += float(res["construction_time"])
        if real_rule_cnt is None:
            real_rule_cnt = int(res["real_rule_cnt"])
        if number_of_tries_or_tables is None:
            number_of_tries_or_tables = int(res['number_of_tries_or_tables'])
        minimal_benchmark_overhead += float(res['minimal_benchmark_overhead'])

        plt = res.get("packet_lookup_times", [])
        if packet_lookup_times is None:
            packet_lookup_times = plt
        else:
            assert len(plt) == len(packet_lookup_times)
            packet_lookup_times = [a + b for a, b in zip(packet_lookup_times, plt)]

    lookup_speed /= repetition_cnt
    construction_time /= repetition_cnt
    minimal_benchmark_overhead /= repetition_cnt
    packet_lookup_times = [a / repetition_cnt for a in packet_lookup_times]
    _packet_lookup_times = json.dumps(packet_lookup_times)
    conn = sqlite3.connect(db_name)
    conn.execute("INSERT INTO test_result VALUES (?,?,?,?,?,?,?,?,?,?,?,?)",
              (None, time.time(), app_name, rule_file, flow_cnt, packet_cnt, real_rule_cnt,
               construction_time, number_of_tries_or_tables, lookup_speed,
               minimal_benchmark_overhead, sqlite3.Binary(_packet_lookup_times.encode("utf-8"))))
    conn.commit()

    return (app_name, rule_file, flow_cnt, packet_cnt)


def _exec_benchmark(args):
    db_name, (app_name, require_sudo, repetition_cnt), rule_file, flow_cnt, packet_cnt = args
    return exec_benchmark(db_name, app_name, repetition_cnt, require_sudo, rule_file, flow_cnt, packet_cnt)


def run_benchmarks(db_file, tasks, parallel):
    requires_sudo = False
    for (_, req, _), _, _, _ in tasks:
        if req:
            requires_sudo = True
            break

    if requires_sudo:
        user = os.getenv("SUDO_USER")
        if user is None:
            print("Some tests require 'sudo'")
            exit()

    conn = sqlite3.connect(db_file)
    conn.execute("INSERT INTO benchmark_execs VALUES (?,?,?,?)",
               (None, time.time(), get_repo_rev(), socket.gethostname()))
    conn.commit()
    num_tasks = len(tasks)
    if parallel:
        with Pool(multiprocessing.cpu_count() // 2) as pool:
            # pool.map(_exec_benchmark, [(db_file, *t) for t in tasks])
            for i, cmd in enumerate(pool.imap_unordered(_exec_benchmark, [(db_file, *t) for t in tasks]), 1):
                print('{0:%} {1}'.format(i / num_tasks, cmd))

    else:
        for i, ((app_name, require_sudo, repetition_cnt), rule_file, flow_cnt, packet_cnt) in enumerate(tasks):
            cmd = exec_benchmark(db_file, app_name, repetition_cnt, require_sudo, rule_file, flow_cnt, packet_cnt)
            print('{0:%} {1}'.format(i / num_tasks, cmd))


def get_repo_rev():
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip()
