import sqlite3
from os.path import basename
import matplotlib.pyplot as plt


def get_last_exec_time(db_conn):
    res = db_conn.execute('SELECT MAX(timestamp) FROM benchmark_execs')
    return res.fetchone()[0]


def generate_graph_troughput_with_increasing_number_of_flows(db_file_name):
    c = sqlite3.connect(db_file_name)
    #last_exec = 0
    last_exec = get_last_exec_time(c)
    
    query = "SELECT * from test_result WHERE timestamp >= ? ORDER BY flow_cnt"
    flow_cnt_values = []  # x 
    app_namesXruleset_troughput = {}  # name of series :  lists of troughtputs for different rulesetns (y)
    last_packet_cnt = None
    for (_, app_name, rule_file, flow_cnt, packet_cnt, real_rule_cnt,
            construction_time, number_or_tries_or_tables, lookup_speed) in c.execute(query, (last_exec,)):
        assert last_packet_cnt == None or packet_cnt == last_packet_cnt, (packet_cnt, last_packet_cnt)
        last_packet_cnt = packet_cnt

        app_namesXruleset = "%s_%s" % (basename(app_name), basename(rule_file))
        troughput = app_namesXruleset_troughput.get(app_namesXruleset, [])
        app_namesXruleset_troughput[app_namesXruleset] = troughput

        troughput.append(lookup_speed)
        
        if not flow_cnt_values or flow_cnt_values[-1] != flow_cnt:
            flow_cnt_values.append(flow_cnt)

    fig, ax = plt.subplots(figsize=(20, 8))
    
    for name, troughputs in app_namesXruleset_troughput.items():
        x = flow_cnt_values
        y = troughputs
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
    plt.savefig('fig/troughput_with_increasing_number_of_flows.png')

def generate_graph_troughput_with_increasing_number_of_rules(db_file_name, number_of_flows):
    """
    [lookup speed]  |
                    |-----
                        [rulesets ordered by decreasing lookup speed]
    """
    pass

    c = sqlite3.connect(db_file_name)
    #last_exec = 0
    last_exec = get_last_exec_time(c)
    
    query = """SELECT app_name, ruleset_name, real_rule_cnt,
                       construction_time, number_or_tries_or_tables, lookup_speed
               from test_result WHERE
               packet_cnt == 10000000 AND flow_cnt == ? AND timestamp >= ? ORDER BY flow_cnt"""
    # group by app name
    app_data = {} # {name: (name of rulse_set, construction_time, number_or_tries_or_tables, lookup_speed)}
    for (app_name, rule_file, real_rule_cnt, construction_time, number_or_tries_or_tables, lookup_speed
            ) in c.execute(query, (number_of_flows, last_exec,)):

        app_names = basename(app_name)
        rule_file = basename(rule_file)
        data = app_data.get(app_names, [])
        app_data[app_names] = data
        data.append((rule_file, real_rule_cnt, construction_time, number_or_tries_or_tables, lookup_speed))
    
    # it is expected that there is single data record for each rule set
    
    # used to sort the rulesets
    cumul_troughput_for_ruleset = {}
    for app_name, data in app_data.items():
        for (rule_file, _, _, _, lookup_speed) in data:
            try:
                cumul_troughput_for_ruleset[rule_file] += lookup_speed
            except KeyError:
                cumul_troughput_for_ruleset[rule_file] = lookup_speed

    x = sorted(cumul_troughput_for_ruleset.items(), key=lambda x: x[1], reverse=True)
    x = [_x[0] for _x in x]
    fig, ax = plt.subplots(figsize=(20, 8))
    
    for name, data in app_data.items():
        y = [ d[4] for d in sorted(data, key=lambda d: cumul_troughput_for_ruleset[d[0]], reverse=True)]
        line1, = ax.plot(x, y, label=name)

    ax.set_ylabel('Throughput [MPkt/s]')
    ax.set_xlabel('rule set')
    ax.set_xlim(left=0)
    #ax1.set_xticks(list(range(len(x))))
    ax.set_xticklabels(x, minor=False, rotation=45)

    # ax.legend()
    # Put a legend below current axis
    # Shrink current axis by 20%
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
    
    # Put a legend to the right of the current axis
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))

    plt.grid()
    plt.savefig('fig/troughput_with_increasing_number_of_rules.png')
