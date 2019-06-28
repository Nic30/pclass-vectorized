from math import ceil
import re
from pip._internal.cli.cmdoptions import pre

values = {
    "TUN_METADATA_TOT_OPT_SIZE": 256,
    "FLOW_MAX_VLAN_HEADERS" : 2,
    "FLOW_N_REGS": 16,
    "ROUND_UP(FLOW_MAX_MPLS_LABELS, 2)": int(ceil(3 / 2))
}

# typename: (size, be)
sizes = {
    "ovs_u128":   (16, False),
    "in6_addr":   (16, True),
    "uint64_t":   (8, False),
    "ovs_be64":   (8, True),
    "uint32_t":   (4, False),
    "flow_in_port":   (4, False),
    "ofp_port_t": (4, False),
    "ovs_be32":   (4, True),
    "flow_vlan_hdr": (4, True),
    "__be32":   (4, True),
    "uint16_t": (2, False),
    "ovs_be16": (2, True),
    "eth_addr": (6, True),
    "uint8_t":  (1, False),
    "__u8":     (1, False),
}
PTR_SIZE = 8

tun_metadata = """
    uint64_t present;
    tun_table* tab;
    uint8_t opts[TUN_METADATA_TOT_OPT_SIZE];
"""
flow_tnl = """
    ovs_be32 ip_dst;
    in6_addr ipv6_dst;
    ovs_be32 ip_src;
    in6_addr ipv6_src;
    ovs_be64 tun_id;
    uint16_t flags;
    uint8_t ip_tos;
    uint8_t ip_ttl;
    ovs_be16 tp_src;
    ovs_be16 tp_dst;
    ovs_be16 gbp_id;
    uint8_t  gbp_flags;
    uint8_t erspan_ver;
    uint32_t erspan_idx;
    uint8_t erspan_dir;
    uint8_t erspan_hwid;
    uint8_t pad1[6];
    tun_metadata metadata;
"""

ovs_key_nsh = """
    uint8_t flags;
    uint8_t ttl;
    uint8_t mdtype;
    uint8_t np;
    ovs_be32 path_hdr;
    ovs_be32 context[4];
"""

flow = """
    flow_tnl tunnel;
    ovs_be64 metadata;
    uint32_t regs[FLOW_N_REGS];
    uint32_t skb_priority;
    uint32_t pkt_mark;
    uint32_t dp_hash;

    flow_in_port in_port;
    uint32_t recirc_id;
    uint8_t ct_state;
    uint8_t ct_nw_proto;
    uint16_t ct_zone;
    uint32_t ct_mark;
    ovs_be32 packet_type;
    ovs_u128 ct_label;
    uint32_t conj_id;
    ofp_port_t actset_output;

    eth_addr dl_dst;
    eth_addr dl_src;
    ovs_be16 dl_type;

    uint8_t pad1[2];
    flow_vlan_hdr vlans[FLOW_MAX_VLAN_HEADERS];
    ovs_be32 mpls_lse[ROUND_UP(FLOW_MAX_MPLS_LABELS, 2)];
    ovs_be32 nw_src;
    ovs_be32 nw_dst;
    ovs_be32 ct_nw_src;
    ovs_be32 ct_nw_dst;
    in6_addr ipv6_src;
    in6_addr ipv6_dst;
    in6_addr ct_ipv6_src;
    in6_addr ct_ipv6_dst;
    ovs_be32 ipv6_label;
    uint8_t nw_frag;
    uint8_t nw_tos;
    uint8_t nw_ttl;
    uint8_t nw_proto;
    in6_addr nd_target;
    eth_addr arp_sha;
    eth_addr arp_tha;
    ovs_be16 tcp_flags;
    ovs_be16 pad2;
    ovs_key_nsh nsh;
    ovs_be16 tp_src;
    ovs_be16 tp_dst;
    ovs_be16 ct_tp_src;
    ovs_be16 ct_tp_dst;
    ovs_be32 igmp_group_ip4;
    ovs_be32 pad3;
"""

stucts = {
    "flow": flow,
    "ovs_key_nsh": ovs_key_nsh,
    "flow_tnl": flow_tnl,
    "tun_metadata": tun_metadata,
}

re_id = "[0-9a-zA-Z_*]+"
re_struct_field = re.compile("(%s)\s+(%s)(\[(.*)\])?;" % (re_id, re_id))


def is_non_matching_field(s, type_name):
    return s.startswith("pad") or type_name.endswith("*")


def generate_names_field(field_name, field_type):
    if field_type.endswith("*"):
        type_size = 8
    else:
        type_size, _ = sizes[field_type]
    if type_size <= 2:
        print(f'"{field_name}",')
    else:
        assert type_size % 2 == 0, type_size
        for i in range(type_size // 2):
            print(f'"{field_name}-{i}",')


def generate_names(parent_name, struct_str):
    lines = [ line.strip() for line in struct_str.split("\n") if line.strip()]

    for line in lines:
        m = re_struct_field.match(line)
        type_name = m.group(1)
        field_name = m.group(2)
        array_size = m.group(4)

        if is_non_matching_field(field_name, type_name):
            continue

        if type_name not in sizes and not type_name.endswith("*"):
            generate_names(parent_name + field_name + ".", stucts[type_name])
        elif array_size:
            try:
                _array_size = int(array_size)
            except ValueError:
                _array_size = values[array_size]
            if sizes[type_name][0] < 2:
                assert _array_size % 2 == 0
                for i in range(_array_size // 2):
                    n = parent_name + field_name + "[%d-%d]" % (i * 2, i * 2 + 1)
                    generate_names_field(n, type_name)
            else:
                for i in range(_array_size):
                    n = parent_name + field_name + "[%d]" % i
                    generate_names_field(n, type_name)
        else:
            n = parent_name + field_name
            generate_names_field(n, type_name)


ipv4_fields = {"ip_dst", "ip_src", "nw_src", "nw_dst", "ct_nw_src", "ct_nw_dst"}
ipv6_fields = {"ipv6_dst", "ipv6_src", "ipv6_src", "ipv6_dst", "ct_ipv6_src", "ct_ipv6_src", "nd_target"}
eth_fields = {"dl_dst", "dl_src", "arp_sha", "arp_tha"}



class CGen():
    def __init__(self):
        self.names = []
        self.formaters = []
        self.offsets = []


    def generate_formater_item(self, path, field_name):
        f_ipv4 = "rule_vec_format_ipv4_part"
        f_ipv6 = "rule_vec_format_ipv6_part"
        f_eth = "rule_vec_format_eth_part"
        f_normal = "rule_vec_format_default<uint16_t>"
        if field_name in ipv4_fields:
            f = f_ipv4
        elif field_name in ipv6_fields:
            f = f_ipv6
        elif field_name in f_eth:
            f = f_eth
        else:
            f = f_normal

        form = f" {f},// {path}"
        self.formaters.append(form)

    def generate_val(self, path, field_name, type_size, offset, be):
        if type_size <= 2:
            offset_str = f'in_packet_position_t({offset}, {type_size},  {be}), // {path}'
            self.names.append(f'"{path}"')
            self.offsets.append(offset_str)
            self.generate_formater_item(path, field_name)
        else:
            assert type_size % 2 == 0, type_size
            part_index = [ i * 2 for i in range(type_size // 2)]
            if not be:
                part_index = list(reversed(part_index))
            # high address first
            for i in part_index:
                _path = f"{path}-{i}"
                _offset = f"{offset} + {i}"
                offset_str = f'in_packet_position_t({_offset}, 2,  {be}), // {_path}'
                self.names.append(f'"{_path}"')
                self.offsets.append(offset_str)
                self.generate_formater_item(_path, field_name)

    def generate_item(self, path, field_name, parent_struct_name,
                      offset_expr_prefix, type_name, array_size):
        type_size, be = sizes[type_name]
        be = int(be)

        if offset_expr_prefix:
            offset = offset_expr_prefix + " + "
        else:
            offset = ""

        offset += f"offsetof({parent_struct_name}, {field_name})"

        if array_size is None:
            self.generate_val(path, field_name, type_size, offset, be)
        else:
            try:
                _array_size = int(array_size)
            except ValueError:
                _array_size = values[array_size]
            array_size = _array_size
            if type_size < 2:
                array_size = array_size//2
                type_size = 2

            for arr_item in range(array_size):
                _offset = arr_item * type_size
                _offset = f"{offset} + {_offset}"
                _path = f"{path}[{arr_item}]"
                self.generate_val(_path, field_name, type_size, _offset, be)


    def generate(self, parent_name, offset_expr, struct_name, struct_str):
        lines = [ line.strip() for line in struct_str.split("\n") if line.strip()]

        for line in lines:
            m = re_struct_field.match(line)
            type_name = m.group(1)
            field_name = m.group(2)
            array_size = m.group(4)

            if is_non_matching_field(field_name, type_name):
                continue

            if type_name not in sizes and not type_name.endswith("*"):
                n = parent_name + field_name + "."
                if offset_expr:
                    o = f"{offset_expr} + "
                else:
                    o = ""
                o = f"offsetof({struct_name}, {field_name})"

                self.generate(n, o, type_name, stucts[type_name])
            else:
                path = parent_name + field_name
                self.generate_item(
                    path, field_name, struct_name,
                    offset_expr, type_name, array_size)

if __name__ == "__main__":
    g = CGen()
    g.generate("", "", "flow", flow)

    assert len(g.formaters) == len(g.names) == len(g.offsets)

    formaters = "\n".join(["\t" + f for f in g.formaters])
    names = "\n".join(["\t" + f + ", //" for f in g.names])
    offsets = "\n".join(["\t" + f for f in g.offsets])

    template = f"""
typename Classifier::formaters_t struct_flow_packet_formaters = {{
{formaters}
}};

typename Classifier::names_t struct_flow_packet_names = {{
{names}
}};

typename Classifier::packet_spec_t struct_flow_packet_spec = {{
{offsets}
}};
"""
    print(template)

