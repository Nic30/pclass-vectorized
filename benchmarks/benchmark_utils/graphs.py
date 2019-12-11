import sqlite3
from os.path import basename
import matplotlib.pyplot as plt
import json


def get_last_exec_time(db_conn):
    res = db_conn.execute('SELECT MAX(timestamp) FROM benchmark_execs')
    return res.fetchone()[0]


def generate_graph_throughput_with_increasing_number_of_flows(db_file_name):
    c = sqlite3.connect(db_file_name)
    # last_exec = 0
    last_exec = get_last_exec_time(c)

    query = """
    SELECT app_name, ruleset_name, flow_cnt, packet_cnt, lookup_speed, packet_lookup_times
    FROM test_result WHERE timestamp >= ? ORDER BY flow_cnt
    """
    flow_cnt_values = []  # x
    app_namesXruleset_throughput = {}  # name of series :  lists of troughtputs for different rulesetns (y)
    last_packet_cnt = None
    for (app_name, ruleset_name, flow_cnt, packet_cnt, lookup_speed, packet_lookup_times
            ) in c.execute(query, (last_exec,)):
        assert last_packet_cnt == None or packet_cnt == last_packet_cnt, (packet_cnt, last_packet_cnt)
        last_packet_cnt = packet_cnt

        app_namesXruleset = "%s_%s" % (basename(app_name), basename(ruleset_name))
        throughput = app_namesXruleset_throughput.get(app_namesXruleset, [])
        app_namesXruleset_throughput[app_namesXruleset] = throughput

        throughput.append(lookup_speed)

        if not flow_cnt_values or flow_cnt_values[-1] != flow_cnt:
            flow_cnt_values.append(flow_cnt)

    fig, ax = plt.subplots(figsize=(10, 5))

    for name, throughputs in app_namesXruleset_throughput.items():
        x = flow_cnt_values
        y = throughputs
        line1, = ax.plot(x, y, label=name)

    ax.set_ylabel('Throughput [MPkt/s]')
    ax.set_xlabel('Flow count')
    ax.set_xlim(left=0)

    # ax.legend()
    # Put a legend below current axis
    # Shrink current axis by 20%
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])

    # Put a legend to the right of the current axis
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))

    plt.grid()
    plt.savefig('fig/throughput_with_increasing_number_of_flows.png')


def generate_ruleset_size_graph(order, data, x):
    """
    [ruleset size]  |
                   |-----
                       [rulesets ordered by decreasing lookup speed]
    """
    fig, ax = plt.subplots(figsize=(10, 5))

    ax.set_ylabel('ruleset size')  # we already handled the x-label with ax1
    data2 = [d[1] for d in sorted(data, key=lambda d: order[d[0]], reverse=True)]

    ax.plot(x, data2)

    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)
    ax.set_xticklabels(x, minor=False, rotation=-90)

    plt.grid()
    plt.savefig('fig/size_of_ruleset.png')


def generate_graph_throughput_for_ruleset(ruleset_order, app_data, x, file_name):
    """
    [throughput]  |
                  |-----
                     [rulesets ordered by decreasing lookup speed]
    """
    fig, ax = plt.subplots(figsize=(10, 5))
    rename_names = {
        "n_tree_lookup": "PCV (optimized B-trees)",
        "ovs_pcv_lookup": "OVS + PCV",
        "ovs_tss_lookup": "OVS native (TSS)",
    }

    for name, data in app_data.items():
        y = [d[4]
             for d in sorted(data, key=lambda d: ruleset_order[d[0]], reverse=True)
            ]
        name = rename_names.get(name, name)
        line1, = ax.plot(x, y, label=name)
        #packet_lookup_times = data[-1]
        #packet_lookup_times = json.loads(packet_lookup_times.decode("utf-8"))
        #ax.boxplot(packet_lookup_times)
        #lookup_time = data[4]
        #ax.plot(lookup_time)


    ax.set_ylabel('Throughput [Mpps]')
    ax.set_xlabel('rule set')
    ax.set_xlim(left=0)
    # ax1.set_xticks(list(range(len(x))))
    ax.set_xticklabels(x, minor=False, rotation=-90)

    # ax.legend()
    # Put a legend below current axis
    # Shrink current axis by 20%
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])

    # Put a legend to the right of the current axis
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))

    plt.grid()
    plt.savefig(file_name)


def generate_graph_construction_time_for_ruleset(ruleset_order, app_data, x, file_name):
    """
    [update time]  |
                   |-----
                       [rulesets ordered by decreasing lookup speed]
    """
    fig, ax = plt.subplots(figsize=(10, 5))
    # wildcard is excluded because it is only a single rule and measured time corresponds just to call overhead
    x = [_x for _x in x if _x != "wildcard"]
    for name, data in app_data.items():
        y = [ d[2] / d[1]
              for d in sorted(data, key=lambda d: ruleset_order[d[0]])
              if d[0] != "wildcard"
              ]
        line1, = ax.plot(x, y, label=name)

    ax.set_ylabel('Update time [us]')
    ax.set_xlabel('rule set')
    ax.set_xlim(left=0)
    # ax1.set_xticks(list(range(len(x))))
    ax.set_xticklabels(x, minor=False, rotation=-90)

    # ax.legend()
    # Put a legend below current axis
    # Shrink current axis by 20%
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])

    # Put a legend to the right of the current axis
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))

    plt.grid()
    plt.savefig(file_name)


def rulesets_ordered_by_cummulative_key(app_data, val_index, reverse):
    cumul_for_ruleset = {}
    for app_name, data in app_data.items():
        for _d in data:
            rule_file = _d[0]
            d = _d[val_index]
            try:
                cumul_for_ruleset[rule_file] += d
            except KeyError:
                cumul_for_ruleset[rule_file] = d

    x = sorted(cumul_for_ruleset.items(), key=lambda x: x[1], reverse=reverse)
    x = [_x[0] for _x in x]
    return cumul_for_ruleset, x


def generate_tree_cnt_for_ruleset(ruleset_order, app_data, x, file_name):
    """
    [tree cnt]  |
                |-----
                    [rulesets ordered by decreasing lookup speed]
    """
    fig, ax = plt.subplots(figsize=(10, 5))

    for name, data in app_data.items():
        y = [ int(d[3]) for d in sorted(data, key=lambda d: ruleset_order[d[0]], reverse=True)]
        # print(y)
        line1, = ax.plot(x, y, label=name)

    # ax.yaxis.set_major_locator(MaxNLocator(integer=True))
    ax.set_ylabel('trie/tree/table cnt')
    ax.set_xlabel('rule set')
    ax.set_xlim(left=0)
    # ax1.set_xticks(list(range(len(x))))
    ax.set_xticklabels(x, minor=False, rotation=-90)

    # ax.legend()
    # Put a legend below current axis
    # Shrink current axis by 20%
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])

    # Put a legend to the right of the current axis
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))

    plt.grid()
    plt.savefig(file_name)


def generate_graphs_depending_on_ruleset_size(db_file_name, number_of_flows, pkt_cnt):
    """
    [y]  |
         |-----
             [rulesets ordered by decreasing lookup speed]
    """

    c = sqlite3.connect(db_file_name)
    # last_exec = 0
    last_exec = get_last_exec_time(c)

    query = """
    SELECT app_name, ruleset_name, real_rule_cnt,
        construction_time, number_of_tries_or_tables, lookup_speed, packet_lookup_times
    FROM test_result WHERE
    packet_cnt == ? AND flow_cnt == ? AND timestamp >= ? ORDER BY flow_cnt
    """
    # group by app name
    app_data = {}  # {name: (name of rulse_set, construction_time, number_of_tries_or_tables, lookup_speed)}
    for (app_name, rule_file, real_rule_cnt, construction_time,
         number_of_tries_or_tables, lookup_speed, packet_lookup_times
         ) in c.execute(query, (pkt_cnt, number_of_flows, last_exec,)):

        app_names = basename(app_name)
        rule_file = basename(rule_file)
        data = app_data.get(app_names, [])
        app_data[app_names] = data
        data.append((
            rule_file, real_rule_cnt, construction_time,
            number_of_tries_or_tables, lookup_speed,
            packet_lookup_times))

    # it is expected that there is single data record for each rule set

    # used to sort the rulesets
    # (rule_file, _, _, _, lookup_speed)
    cumul_throughput_for_ruleset, x = rulesets_ordered_by_cummulative_key(app_data, 4, True)

    data2_values = list(app_data.values())[0]
    data2 = [
        (d[0], d[1])
        for d in sorted(data2_values,
                        key=lambda d: d[1],
                        reverse=False)
    ]
    ruleset_order_by_size = {
        d[0] : len(data2) - i
        for i, d in enumerate(data2)
    }
    x_ordered_by_size = [d[0] for d in data2]

    generate_graph_throughput_for_ruleset(
        cumul_throughput_for_ruleset, app_data, x,
        'fig/throughput_for_increasing_ruleset_complexity.png')
    generate_graph_throughput_for_ruleset(
        ruleset_order_by_size, app_data, x_ordered_by_size,
        'fig/throughput_for_increasing_ruleset_size.png')
    generate_ruleset_size_graph(cumul_throughput_for_ruleset, data, x)

    generate_tree_cnt_for_ruleset(ruleset_order_by_size, app_data, x_ordered_by_size,
        'fig/number_of_trees_for_increasing_ruleset_size.png')

    cumul_constr_time_for_ruleset, x = rulesets_ordered_by_cummulative_key(app_data, 2, False)
    generate_graph_construction_time_for_ruleset(
        cumul_constr_time_for_ruleset, app_data, x,
        "fig/average_update_time_for_increasing_ruleset_complexity.png")

