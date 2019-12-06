#pragma once


#pragma once
#include <vector>
#include <map>
#include <iostream>

#include <pcv/rule_parser/rule.h>
#include <pcv/rule_parser/trace_tools.h>

#include "lib/pvector.h"
#include "ovs/lib/dpif-netdev.h"

#undef atomic_store
#undef atomic_compare_exchange_strong_explicit
#undef atomic_compare_exchange_strong
#undef atomic_compare_exchange_weak_explicit
#undef atomic_compare_exchange_weak

namespace pcv {
namespace ovs {

/*
 * Container for dpcls and it's functions
 * */
class OvsDpclsWrap {
public:
	struct dpcls cls;
	std::vector<struct dpcls_rule*> ovs_rules;
	std::map<const struct dpcls_rule*, const Rule_Ipv4_ACL *> ovs_rule_to_pcv_rule;

	using key_vec_t = struct netdev_flow_key;
	using rule_id_t = const struct dpcls_rule *;

	OvsDpclsWrap();
	rule_id_t search(const key_vec_t & f);

	void insert(Rule_Ipv4_ACL & r, size_t v);
	const Rule_Ipv4_ACL * cls_rule_get_pcv_rule(const struct dpcls_rule * r);

	// utils
	static struct netdev_flow_key flow_from_packet(const packet_t & p);
	static Rule_Ipv4_ACL flow_to_Rule_Ipv4_ACL(const struct flow & f);

	~OvsDpclsWrap();
};

}
}
