#pragma once
#include <stdint.h>
#include <array>
#include <pcv/common/range.h>

namespace pcv {

class iParsedRule {
public:
	int priority;
	int tag;
	uint16_t vlan;
	iParsedRule() :
			priority(-1), tag(-1), vlan(0) {
	}
};

class Rule_Ipv4: public iParsedRule {
public:
	// values stored in little-endian to be easily comparable on x86_64
	Range1d<uint32_t> sip;
	Range1d<uint32_t> dip;
	Range1d<uint16_t> sport;
	Range1d<uint16_t> dport;
	Range1d<uint16_t> proto;

	Rule_Ipv4();
	// serialize rule to classbench format
	friend std::ostream & operator<<(std::ostream & str, const Rule_Ipv4 & r);
	operator std::string() const;
};

using uint24_t = uint32_t;
using eth_t = uint64_t;
struct ipv6_t {
	uint64_t low, high;
};

class Rule_Ipv6: public iParsedRule {
public:

};

class Rule_OF_1_5_1: iParsedRule {
public:
	Range1d<uint32_t> in_port; /* Switch input port. */
	Range1d<uint32_t> in_phy_port; /* Switch physical input port. */
	Range1d<uint64_t> metadata; /* Metadata passed between tables. */
	Range1d<eth_t> eth_dst; /* Ethernet destination address. */
	Range1d<eth_t> eth_src; /*Ethernet source address. */
	Range1d<uint16_t> eth_type; /*Ethernet frame type. */
	Range1d<uint16_t> vlan_vid; /*VLAN id. */
	Range1d<uint8_t> vlan_pcp; /*VLAN priority. */
	Range1d<uint8_t> ip_dscp; /*IP DSCP (6 bits in ToS field). */
	Range1d<uint8_t> ip_ecn; /*IP ECN (2 bits in ToS field). */
	Range1d<uint8_t> ip_proto; /*IP protocol. */
	Range1d<uint32_t> ipv4_src; /*IPv4 source address. */
	Range1d<uint32_t> ipv4_dst; /*IPv4 destination address. */
	Range1d<uint16_t> tcp_src; /*TCP source port. */
	Range1d<uint16_t> tcp_dst; /*TCP destination port. */
	Range1d<uint16_t> udp_src; /*UDP source port. */
	Range1d<uint16_t> udp_dst; /*UDP destination port. */
	Range1d<uint16_t> sctp_src; /*SCTP source port. */
	Range1d<uint16_t> sctp_dst; /*SCTP destination port. */
	Range1d<uint8_t> icmpv4_type; /*ICMP type. */
	Range1d<uint8_t> icmpv4_code; /*ICMP code. */
	Range1d<uint16_t> arp_op; /*ARP opcode. */
	Range1d<uint32_t> arp_spa; /*ARP source IPv4 address. */
	Range1d<uint32_t> arp_tpa; /*ARP target IPv4 address. */
	Range1d<eth_t> arp_sha; /*ARP source hardware address. */
	Range1d<eth_t> arp_tha; /*ARP target hardware address. */
	Range1d<ipv6_t> ipv6_src; /*IPv6 source address. */
	Range1d<ipv6_t> ipv6_dst; /*IPv6 destination address. */
	Range1d<uint32_t> ipv6_flabel; /*IPv6 Flow Label */
	Range1d<uint8_t> icmpv6_type; /*ICMPv6 type. */
	Range1d<uint8_t> icmpv6_code; /*ICMPv6 code. */
	Range1d<ipv6_t> ipv6_nd_target;/*Target address for ND. */
	Range1d<eth_t> ipv6_nd_sll; /*Source link-layer for ND. */
	Range1d<eth_t> ipv6_nd_tll; /*Target link-layer for ND. */
	Range1d<uint32_t> mpls_label; /*MPLS label. */
	Range1d<uint8_t> mpls_tc; /*MPLS TC. */
	Range1d<uint8_t> mpls_bos; /*MPLS BoS bit. */
	Range1d<uint24_t> pbb_isid; /*PBB I-SID. */
	Range1d<uint64_t> tunnel_id; /*Logical Port Metadata. */
	Range1d<uint16_t> ipv6_exthdr; /*IPv6 Extension Header pseudo-field */
	Range1d<uint8_t> pbb_uca; /*PBB UCA header field. */
	Range1d<uint16_t> tcp_flags; /*TCP flags. */
	Range1d<uint32_t> actset_output; /*Output port from action set metadata. */
	Range1d<uint32_t> packet_type; /*Packet type value. */


	friend std::ostream & operator<<(std::ostream & str, const Rule_Ipv4 & r);
	operator std::string() const;
};

namespace rule_conv_fn {

template<typename T, size_t D>
std::array<Range1d<T>, D> rule_to_array(const Rule_Ipv4 & r);
template<typename T, size_t D>
std::array<Range1d<T>, D> rule_to_array(const Rule_OF_1_5_1 & r);

}

}
