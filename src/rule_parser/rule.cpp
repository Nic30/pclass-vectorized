#include <pcv/rule_parser/rule.h>
#include <pcv/rule_parser/rule_vec_utils.h>

namespace pcv {

Rule_Ipv4_ACL::Rule_Ipv4_ACL() :
		sip(0, std::numeric_limits<uint32_t>::max()), dip(0,
				std::numeric_limits<uint32_t>::max()), sport(0,
				std::numeric_limits<uint16_t>::max()), dport(0,
				std::numeric_limits<uint16_t>::max()), proto(0,
				std::numeric_limits<uint16_t>::max()) {

}

size_t Rule_Ipv4_ACL::cummulative_prefix_len() {
	return sip.prefix_len_le() + dip.prefix_len_le() + sport.prefix_len_le()
			+ dport.prefix_len_le() + proto.prefix_len_le();
}

size_t Rule_Ipv4_ACL::max_cummulative_prefix_len() {
	return sip.max_prefix_len() + dip.max_prefix_len() + sport.max_prefix_len()
			+ dport.max_prefix_len() + proto.max_prefix_len();
}

void Rule_Ipv4_ACL::reverse_endianity() {
	sip = sip.to_be();
	dip = dip.to_be();
	sport = sport.to_be();
	dport = dport.to_be();
	proto = proto.to_be();
}


Range1d<uint16_t> * fill(Range1d<uint16_t> * begin,
		const Range1d<uint8_t> & val) {
	uint16_t high = val.high;
	if (high == std::numeric_limits<uint8_t>::max()) {
		high = std::numeric_limits<uint16_t>::max();
	}
	*begin = Range1d<uint16_t>(val.low, high);
	begin++;
	return begin;
}

Range1d<uint16_t> * fill(Range1d<uint16_t> * begin,
		const Range1d<uint16_t> & val) {
	*begin = val;
	begin++;
	return begin;
}

Range1d<uint16_t> * fill(Range1d<uint16_t> * begin,
		const Range1d<uint32_t> & val) {
	using R = Range1d<uint16_t>;
	auto m = std::numeric_limits<uint16_t>::max();
	*begin = R(val.low & m, val.high & m);
	begin++;
	*begin = R((val.low >> 16) & m, (val.high >> 16) & m);
	begin++;
	return begin;
}

Range1d<uint16_t> * fill_eth(Range1d<uint16_t> * begin,
		const Range1d<eth_t> & val) {
	using R = Range1d<uint16_t>;
	auto m = std::numeric_limits<uint16_t>::max();
	for (int i = 3 - 1; i >= 0; i--) {
		*begin = R((val.low >> (i * 16)) & m, (val.high >> (i * 16)) & m);
		begin++;
	}
	return begin;
}

Range1d<uint16_t> * fill(Range1d<uint16_t> * begin,
		const Range1d<uint64_t> & val) {
	using R = Range1d<uint16_t>;
	auto m = std::numeric_limits<uint16_t>::max();
	for (size_t i = 0; i < 4; i++) {
		*begin = R((val.low >> (i * 16)) & m,
				   (val.high >> (i * 16)) & m);
		begin++;
	}
	return begin;
}

Range1d<uint16_t> * fill(Range1d<uint16_t> * begin,
		const Range1d<ipv6_t> & val) {
	using R = Range1d<uint16_t>;
	auto m = std::numeric_limits<uint16_t>::max();
	for (size_t i = 0; i < 4; i++) {
		*begin = R((val.low.high >> (i * 16)) & m,
				(val.high.high >> (i * 16)) & m);
		begin++;
	}
	for (size_t i = 0; i < 4; i++) {
		*begin = R((val.low.low >> (i * 16)) & m,
				(val.high.low >> (i * 16)) & m);
		begin++;
	}
	return begin;
}

namespace rule_conv_fn {


Rule_Ipv4_ACL rule_from_array(const std::array<Range1d<uint16_t>, 7> & arr) {
	Rule_Ipv4_ACL r;
	auto a = &arr[0];
	size_t i = 0;
	vec_build::pop_32(r.sip, i, a);
	vec_build::pop_32(r.dip, i, a);
	r.sport = arr[i];
	r.dport = arr[i+1];
	r.proto = arr[i+2];
	return r;
}

std::array<Range1d<uint16_t>, 7> rule_to_array_16b(const Rule_Ipv4_ACL & r) {
	std::array<Range1d<uint16_t>, 7> _r;
	auto p = fill(&_r[0], r.sip);
	fill(p, r.dip);
	_r[4] = r.sport;
	_r[5] = r.dport;
	_r[6] = r.proto;

	return _r;
}

Rule_Ipv4_ACL exact_array_to_rule_le(const std::array<uint16_t, 7> & a) {
	Rule_Ipv4_ACL r;
	uint32_t v = *((uint32_t*) &a[0]);
	r.sip = {v, v};
	v = *((uint32_t*) &a[2]);
	r.dip = {v, v};
	r.sport = {a[4], a[4]};
	r.dport = {a[5], a[5]};
	r.proto = {a[6], a[6]};

	return r;
}

Rule_Ipv4_ACL exact_array_to_rule_be(const std::array<uint16_t, 7> & a) {
	auto v = exact_array_to_rule_le(a);
	v.reverse_endianity();
	return v;
}

std::array<Range1d<uint16_t>, 177> rule_to_array_16b(const Rule_OF_1_5_1 & r) {
	std::array<Range1d<uint16_t>, 177> _r;
	auto a = fill(&_r[0], r.in_port);
	a = fill(a, r.in_phy_port);
	a = fill(a, r.metadata);
	a = fill_eth(a, r.eth_dst);
	a = fill_eth(a, r.eth_src);
	a = fill(a, r.eth_type);
	a = fill(a, r.vlan_vid);
	a = fill(a, r.vlan_pcp);
	a = fill(a, r.ip_dscp);
	a = fill(a, r.ip_ecn);
	a = fill(a, r.ip_proto);
	a = fill(a, r.ipv4_src);
	a = fill(a, r.ipv4_dst);
	a = fill(a, r.tcp_src);
	a = fill(a, r.tcp_dst);
	a = fill(a, r.udp_src);
	a = fill(a, r.udp_dst);
	a = fill(a, r.sctp_src);
	a = fill(a, r.sctp_dst);
	a = fill(a, r.icmpv4_type);
	a = fill(a, r.icmpv4_code);
	a = fill(a, r.arp_op);
	a = fill(a, r.arp_spa);
	a = fill(a, r.arp_tpa);
	a = fill(a, r.arp_sha);
	a = fill(a, r.arp_tha);
	a = fill(a, r.ipv6_src);
	a = fill(a, r.ipv6_dst);
	a = fill(a, r.ipv6_flabel);
	a = fill(a, r.icmpv6_type);
	a = fill(a, r.icmpv6_code);
	a = fill(a, r.ipv6_nd_target);
	a = fill(a, r.ipv6_nd_sll);
	a = fill(a, r.ipv6_nd_tll);
	a = fill(a, r.mpls_label);
	a = fill(a, r.mpls_tc);
	a = fill(a, r.mpls_bos);
	a = fill(a, r.pbb_isid);
	a = fill(a, r.tunnel_id);
	a = fill(a, r.ipv6_exthdr);
	a = fill(a, r.pbb_uca);
	a = fill(a, r.tcp_flags);
	a = fill(a, r.actset_output);
	a = fill(a, r.packet_type);

	return _r;
}

}

Rule_OF_1_5_1::Rule_OF_1_5_1() {
	in_port.set_wildcard();
	in_phy_port.set_wildcard();
	metadata.set_wildcard();
	eth_dst.set_wildcard();
	eth_src.set_wildcard();
	eth_type.set_wildcard();
	vlan_vid.set_wildcard();
	vlan_pcp.set_wildcard();
	ip_dscp.set_wildcard();
	ip_ecn.set_wildcard();
	ip_proto.set_wildcard();
	ipv4_src.set_wildcard();
	ipv4_dst.set_wildcard();
	tcp_src.set_wildcard();
	tcp_dst.set_wildcard();
	udp_src.set_wildcard();
	udp_dst.set_wildcard();
	sctp_src.set_wildcard();
	sctp_dst.set_wildcard();
	icmpv4_type.set_wildcard();
	icmpv4_code.set_wildcard();
	arp_op.set_wildcard();
	arp_spa.set_wildcard();
	arp_tpa.set_wildcard();
	arp_sha.set_wildcard();
	arp_tha.set_wildcard();
	ipv6_src.set_wildcard();
	ipv6_dst.set_wildcard();
	ipv6_flabel.set_wildcard();
	icmpv6_type.set_wildcard();
	icmpv6_code.set_wildcard();
	ipv6_nd_target.set_wildcard();
	ipv6_nd_sll.set_wildcard();
	ipv6_nd_tll.set_wildcard();
	mpls_label.set_wildcard();
	mpls_tc.set_wildcard();
	mpls_bos.set_wildcard();
	pbb_isid.set_wildcard();
	tunnel_id.set_wildcard();
	ipv6_exthdr.set_wildcard();
	pbb_uca.set_wildcard();
	tcp_flags.set_wildcard();
	actset_output.set_wildcard();
	packet_type.set_wildcard();
}

namespace rule_vec_format {

void rule_vec_format_ipv4_part(std::ostream & str, const Range1d<uint16_t> & k) {
	auto p_len = k.prefix_len_le();
	str << (unsigned(k.low) >> 8) << ".";
	str << (unsigned(k.low) & 0xff) << "/" << p_len;
}

void rule_vec_format_ipv6_part(std::ostream & str, const Range1d<uint16_t> & k) {
	auto p_len = k.prefix_len_le();
	auto flags = str.flags();

	str << std::hex << (unsigned(k.low) >> 8) << ":";
	str << std::hex << (unsigned(k.low) & 0xff) << "/" << std::dec << p_len;

	str.flags(flags);
}
void rule_vec_format_eth_part(std::ostream & str, const Range1d<uint16_t> & k) {
	rule_vec_format_ipv6_part(str, k);
}

std::array<std::function<void(std::ostream &, const Range1d<uint16_t>)>, 7> Rule_Ipv4_ACL_formaters =
		{ rule_vec_format_ipv4_part,
		  rule_vec_format_ipv4_part,
		  rule_vec_format_ipv4_part,
		  rule_vec_format_ipv4_part,
		  rule_vec_format_default<uint16_t>,
		  rule_vec_format_default<uint16_t>,
		  rule_vec_format_default<uint16_t>,
		};
std::array<std::string, 7> Rule_Ipv4_ACL_names = { //
		"sip-low", "sip-high", //
				"dip-low", "dip-high", //
				"sport",              //
				"dport",              //
				"proto"               //
		};

};

}
