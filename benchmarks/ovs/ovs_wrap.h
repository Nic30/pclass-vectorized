#pragma once
#include <vector>
#include <map>
#include <iostream>

#include <pcv/rule_parser/rule.h>
#include <pcv/rule_parser/trace_tools.h>

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

	OvsWrap();
	static struct flow flow_from_packet(const packet_t & p);
	const struct cls_rule * search(struct flow & f);
	const struct cls_rule * search(packet_t & p);

	void insert(Rule_Ipv4_ACL & r, size_t v);
	const Rule_Ipv4_ACL * cls_rule_get_pcv_rule(const struct cls_rule * r);

	~OvsWrap();
};

}
}
