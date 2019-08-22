import os
from benchmark_utils.run_benchmarks import cartesian
import subprocess
import json
import matplotlib.pyplot as plt

ROOT = "build/meson.debug.linux.x86_64/benchmarks/"
APP_BASIC = "hash_vs_rb_tree_vs_b_tree"
APP_PREFIX = "prefix_combinations"
RULE_CNTS = [
    '1',
    '32',
    '128',
    '256',
    '512',
    '2048',
    '4096',
    '6500',
    '8192',
    '10000',
    '13000',
    '16384',
]
TRAC_CNTS = [ '32768',
              ]
ALGS = ['hash', 'b_tree', ]
ALGS_PREFIX = ['tss', 'b_tree', ]
LOOKUP_CNTS = [
    # '10000'
    '1000000'
]
PREFIXES = [str(i) for i in range(33)]


def run_test(test, args):
    # args: tuple( cls, rules,  traces,  lookup)
    app = os.path.join(ROOT, test)
    cmd = [app, *args]
    res = subprocess.check_output(cmd)
    try:
        res = json.loads(res)
    except Exception:
        print(res)
        raise
    res["lookup_speed"] = float(res["lookup_speed"])
    res["construction_time"] = float(res["construction_time"])
    res["real_rule_cnt"] = int(res["real_rule_cnt"])
    return res


def graph_defaults(ax):
    ax.set_xlim(left=0)

    # ax.legend()
    # Put a legend below current axis
    # Shrink current axis by 20%
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])

    # Put a legend to the right of the current axis
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))

    plt.grid()


def build_graph_basic_throughput_X_rules(args, results):
    # args: tuples(cls, rules,  traces,  lookup)
    # each cls is series
    # lookup X rules
    fig, ax = plt.subplots()
    clsXlookup = {}
    for (cls, rules, _, _), res in zip(args, results):
        v = clsXlookup.get(cls, [])
        clsXlookup[cls] = v
        assert RULE_CNTS[len(v)] == rules, (len(v), rules)

        v.append(res)

    for name, res in clsXlookup.items():
        x = [ int(v) for v in RULE_CNTS]
        y = [ r["lookup_speed"] for r in res ]
        line1, = ax.plot(x, y, label=name)

    ax.set_ylabel('Throughput [MPkt/s]')
    ax.set_xlabel('Rule cnt')
    graph_defaults(ax)
    plt.savefig('fig/basic_throughput_X_rules.png')


def build_graph_basic_update_X_rules(args, results):
    fig, ax = plt.subplots()
    clsXupdate = {}
    for (cls, rules, _, _), res in zip(args, results):
        v = clsXupdate.get(cls, [])
        clsXupdate[cls] = v
        assert RULE_CNTS[len(v)] == rules, (len(v), rules)

        v.append(res)

    for name, res in clsXupdate.items():
        x = [ int(v) for v in RULE_CNTS]
        y = [ r["construction_time"] / float(rc) for r, rc in zip(res, RULE_CNTS) ]
        line1, = ax.plot(x, y, label=name)

    ax.set_ylabel('Update time [us]')
    ax.set_xlabel('Rule cnt')
    graph_defaults(ax)
    plt.savefig('fig/basic_update_X_rules.png')


def build_graph_basic_throughput_X_prefix_cnt(args, results):
    fig, ax = plt.subplots()
    clsXlookup = {}
    # cls, prefix, traces, lookup
    for (cls, prefix, _, _), res in zip(args, results):
        v = clsXlookup.get(cls, [])
        clsXlookup[cls] = v
        assert PREFIXES[len(v)] == prefix, (len(v), prefix)

        v.append(res)

    for name, res in clsXlookup.items():
        x = [ int(v) for v in PREFIXES]
        y = [ r["lookup_speed"] for r in res ]
        line1, = ax.plot(x, y, label=name)

    ax.set_ylabel('Throughput [MPkt/s]')
    ax.set_xlabel('Prefix groups')
    graph_defaults(ax)
    plt.savefig('fig/basic_throughput_X_prefix_cnt.png')


def build_graph_basic_throughput_X_update_time(args, results):
    fig, ax = plt.subplots()
    clsXupdate = {}
    # cls, prefix, traces, lookup
    for (cls, prefix, _, _), res in zip(args, results):
        v = clsXupdate.get(cls, [])
        clsXupdate[cls] = v
        assert PREFIXES[len(v)] == prefix, (len(v), prefix)

        v.append(res)

    for name, res in clsXupdate.items():
        x = [ int(v) for v in PREFIXES][1:]
        y = [ r["construction_time"] / (float(pc) + 1) for r, pc in zip(res, PREFIXES) ][1:]
        line1, = ax.plot(x, y, label=name)

    ax.set_ylabel('Update time [us]')
    ax.set_xlabel('Prefix groups')
    graph_defaults(ax)
    plt.savefig('fig/basic_update_X_prefix_cnt.png')


def run_basic_benchmarks():
    TEST_ARGS = list(
        cartesian(ALGS, RULE_CNTS, TRAC_CNTS, LOOKUP_CNTS)
    )
    results = []
    for args in TEST_ARGS:
        res = run_test(APP_BASIC, args)
        results.append(res)
        print((args, res))

    build_graph_basic_throughput_X_rules(TEST_ARGS, results)
    build_graph_basic_update_X_rules(TEST_ARGS, results)


def run_prefix_benchmarks():
    # cls, prefix, traces, lookup
    TEST_ARGS = list(
        cartesian(ALGS_PREFIX, PREFIXES, ['8192'], LOOKUP_CNTS)
    )
    results = []
    for args in TEST_ARGS:
        res = run_test(APP_PREFIX, args)
        results.append(res)
        print((args, res))

    build_graph_basic_throughput_X_prefix_cnt(TEST_ARGS, results)
    build_graph_basic_throughput_X_update_time(TEST_ARGS, results)


def main():
    run_basic_benchmarks()
    run_prefix_benchmarks()


if __name__ == "__main__":
    main()
