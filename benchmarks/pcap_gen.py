from scapy.all import *


def basic_flows():
    flow_numbers = [
        #1,
        #100,
        #5000,
        10000,
        50000,
        75000,
        85000,
        95000,
        #100000
    ]

    for f_n in flow_numbers:
        pkts = []
        rules = []
        for i in range(f_n):
            a, b, c = ((i >> 16) & 0xff, (i >> 8) & 0xff, i & 0xff)
            src = f"2.{a}.{b}.{c}"
            dst = f"1.{a}.{b}.{c}"
            pkt = Ether(dst="FF:FF:FF:FF:FF:FF") / IP(dst=dst, src=src) / TCP(sport=1, dport=1) / "0000"
            pkts.append(pkt)
            r = f"in_port=1,ip,nw_dst={dst},nw_src={src},tcp,tp_src=1,tp_dst=1,actions=output:2"
            rules.append(r)

        wrpcap(f'test_data/flows_{f_n}.pcap', pkts)
        with open(f"test_data/rules_{f_n}.txt", "w") as f:
            for r in rules:
                f.write(r + "\n")

        print(f"done {f_n}")


def lpm_flows():
    for i in range(1, 32 + 32 + 16 + 16 + 1):
        pass


basic_flows()