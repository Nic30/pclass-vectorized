import sqlite3
import subprocess
import multiprocessing
from multiprocessing import Pool
import time
import socket
import json

def build_test_name(app_name, rule_file, flow_cnt, packet_cnt):
    return "%s_%s_%d_%d" % (basename(app_name), basename(rule_file), flow_cnt, packet_cnt)


def exec_benchmark(db_name, app_name, repetition_cnt, require_sudo, rule_file, flow_cnt, packet_cnt):
    is_dpkd = "dpdk" in app_name
    if is_dpkd:
        cmd = [app_name, "--", rule_file, str(flow_cnt), str(packet_cnt), "1"]
    else:
        cmd = [app_name, rule_file, str(flow_cnt), str(packet_cnt), "1"]
        
    lookup_speed = 0.0
    construction_time = 0.0
    real_rule_cnt = None
    number_or_tries_or_tables = None

    for _ in range(repetition_cnt):
        res = subprocess.check_output(cmd)
        if is_dpkd:
            res = res[res.find(b"{") - 1:]
        res = json.loads(res)
        lookup_speed += float(res["lookup_speed"])
        construction_time += float(res["construction_time"])
        if real_rule_cnt is None:
            real_rule_cnt = int(res["real_rule_cnt"])
        if number_or_tries_or_tables is None:
            number_or_tries_or_tables = int(res['number_or_tries_or_tables'])

    lookup_speed /= repetition_cnt
    construction_time /= repetition_cnt
    conn = sqlite3.connect(db_name)
    conn.execute("INSERT INTO test_result VALUES (?,?,?,?,?,?,?,?,?)",
              (time.time(), app_name, rule_file, flow_cnt, packet_cnt, real_rule_cnt,
               construction_time, number_or_tries_or_tables, lookup_speed))
    conn.commit()



def _exec_benchmark(args):
    db_name, (app_name, require_sudo, repetition_cnt), rule_file, flow_cnt, packet_cnt = args
    exec_benchmark(db_name, app_name, repetition_cnt, require_sudo, rule_file, flow_cnt, packet_cnt)

def run_benchmarks(db_file, tasks, parallel):
    conn = sqlite3.connect(db_file)
    conn.execute("INSERT INTO benchmark_execs VALUES (?,?,?)",
               (time.time(), get_repo_rev(), socket.gethostname()))
    conn.commit()
    if parallel:
        with Pool(multiprocessing.cpu_count() // 2) as pool:
            pool.map(_exec_benchmark, [(db_file, *t) for t in tasks])
    else:      
        for (app_name, require_sudo, repetition_cnt), rule_file, flow_cnt, packet_cnt in tasks:
            exec_benchmark(db_name, app_name, repetition_cnt, require_sudo, rule_file, flow_cnt, packet_cnt)
        
def get_repo_rev():
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip()