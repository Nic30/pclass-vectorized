
from subprocess import check_output, call, run
import os
from time import sleep


def init(flow_cnt):
    call("ovs-vsctl set Open_vSwitch . other_config:pmd-cpu-mask=0x3".split(" "))
    call("ovs-vsctl del-br br0".split(" "))
    call("ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev".split(sep=" "))
    ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "test_data"))

    rx_pcap = os.path.join(ROOT, f"flows_{flow_cnt}.pcap")
    dummy_rx_pcap = os.path.join(ROOT, f"flows_1.pcap")
    tx_pcap = os.path.join(ROOT, f"flows_{flow_cnt}_out.pcap")
    call(f"ovs-vsctl add-port br0 tg0 -- set int tg0 type=dpdk options:dpdk-devargs=vdev:net_pcap0,rx_pcap={rx_pcap},infinite_rx=1".split())
    call(f"ovs-vsctl add-port br0 tg1 -- set int tg1 type=dpdk options:dpdk-devargs=vdev:net_pcap1,rx_pcap={dummy_rx_pcap}".split())
    # call(f"ovs-vsctl add-port br0 tg1 -- set int tg1 type=dpdk options:dpdk-devargs=vdev:net_pcap1,rx_pcap={dummy_rx_pcap},tx_pcap={tx_pcap}".split())
    call("ovs-ofctl del-flows br0".split())
    call("ovs-ofctl mod-port br0 1 down".split())

    with open(os.path.join(ROOT, f"rules_{flow_cnt}.txt")) as f:
        rules = f.read()
        # res = run("ovs-ofctl add-flows br0 -".split(" "), input=rules, encoding='ascii')
        # res = run("ovs-ofctl add-flows br0 -".split(" "), input=rules.split("\n")[0] + "\n", encoding='ascii')
        res = run("ovs-ofctl add-flows br0 -".split(" "), input=f"in_port=1,ip,nw_dst=1.0.0.0/8,nw_src=2.0.0.0/8,tcp,tp_src=1,tp_dst=1,actions=output:2", encoding='ascii')
        assert res.returncode == 0

    call("ovs-ofctl mod-port br0 1 up".split())


def get_pkt_cnt(port_name):
    res = check_output("ovs-ofctl dump-ports br0".split())
    res = res.decode()
    #     example = """
    #  OFPST_PORT reply (xid=0x2): 3 ports
    #    port LOCAL: rx pkts=0, bytes=0, drop=0, errs=0, frame=0, over=0, crc=0
    #             tx pkts=0, bytes=0, drop=3643502459, errs=0, coll=0
    #    port  tg0: rx pkts=3643502688, bytes=160314118272, drop=0, errs=0, frame=?, over=?, crc=?
    #             tx pkts=0, bytes=0, drop=0, errs=0, coll=?
    #    port  tg1: rx pkts=1, bytes=44, drop=0, errs=0, frame=?, over=?, crc=?
    #             tx pkts=303371168, bytes=13348331392, drop=0, errs=0, coll=?
    # """
    # res = example
    found = False
    for line in res.split("\n"):
        if found:
            s = line.split(",")
            s = s[0].strip()
            assert s.startswith("tx pkts="), s
            s = s.split("=")
            return int(s[1])

        if port_name in line:
            found = True
    raise KeyError(port_name, res)


if __name__ == "__main__":
    print("Flow cnt MPkt/s")
    for flow_cnt in [#1,
                     100,
                     #5000,
                     #10000,
                     #50000,
                     #75000,
                     #85000,
                     #95000,
                     #int(100e3)
                    ]:
        init(flow_cnt)
        sleep(3)
        print(flow_cnt, "%.2f" % (get_pkt_cnt("2:") / 10 / 1e6))
