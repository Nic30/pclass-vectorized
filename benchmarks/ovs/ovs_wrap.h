#pragma once
#include <vector>
#include <map>
#include <iostream>

#include <pcv/rule_parser/rule.h>
#include <pcv/rule_parser/trace_tools.h>

#include "lib/pvector.h"
#include "lib/classifier.h"
#undef atomic_store
#undef atomic_compare_exchange_strong_explicit
#undef atomic_compare_exchange_strong
#undef atomic_compare_exchange_weak_explicit
#undef atomic_compare_exchange_weak

namespace pcv {
namespace ovs {

class OvsWrap {
public:
	classifier cls;
	ovs_version_t version;
	std::vector<struct cls_rule*> ovs_rules;
	std::map<const struct cls_rule*, const Rule_Ipv4_ACL *> ovs_rule_to_pcv_rule;

	using key_vec_t = struct flow;
	using rule_id_t = const struct cls_rule *;

	OvsWrap();
	rule_id_t search(const key_vec_t & f);

	bool insert(struct cls_rule * rule);
	void insert(Rule_Ipv4_ACL & r, size_t v);
	const Rule_Ipv4_ACL * cls_rule_get_pcv_rule(const struct cls_rule * r);

	// utils
	static struct flow flow_from_packet(const packet_t & p);
	static Rule_Ipv4_ACL flow_to_Rule_Ipv4_ACL(const struct flow & f);

	~OvsWrap();
};

}
}
