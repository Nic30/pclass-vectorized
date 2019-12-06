#pragma once

#include "lib/flow.h"
#include "openvswitch/match.h"
#undef atomic_store
#undef atomic_compare_exchange_strong_explicit
#undef atomic_compare_exchange_strong
#undef atomic_compare_exchange_weak_explicit
#undef atomic_compare_exchange_weak
#include <pcv/rule_parser/rule.h>
#include <pcv/common/range.h>

namespace pcv {

/*
 * Convert Rule_Ipv4_ACL from this library to a OvS representation of the rule
 * */
inline struct match rule_ipv4_acl_to_ovs_match(Rule_Ipv4_ACL &r) {
	struct match match;
	match_init_catchall(&match);

	auto sip = r.sip.to_be();
	match.flow.nw_src = sip.low;
	match.wc.masks.nw_src = sip.get_mask_be();

	auto dip = r.dip.to_be();
	match.flow.nw_dst = dip.low;
	match.wc.masks.nw_dst = dip.get_mask_be();

	auto sport = r.sport.to_be();
	match.flow.tp_src = sport.low;
	match.wc.masks.tp_src = sport.get_mask_be();

	auto dport = r.dport.to_be();
	match.flow.tp_dst = dport.low;
	match.wc.masks.tp_dst = sport.get_mask_be();

	match.flow.nw_proto = r.proto.low;
	match.wc.masks.nw_proto = r.proto.get_mask_le();
	return match;
}

}
