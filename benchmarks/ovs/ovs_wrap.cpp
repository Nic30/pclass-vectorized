#include "ovs_wrap.h"

#include "rule_ipv4_acl_to_ovs_match.h"

#include <pcv/common/range.h>
#include <byteswap.h>

namespace pcv {
namespace ovs {

OvsWrap::OvsWrap() {
	classifier_init(&cls, nullptr);
	version = 0;
}

struct flow OvsWrap::flow_from_packet(const packet_t &p) {
	struct flow f;
	memset(&f, 0, sizeof f);
	auto _p = rule_conv_fn::exact_array_to_rule_le(p);

	f.nw_src = _p.sip.low;
	f.nw_dst = _p.dip.low;
	f.tp_src = _p.sport.low;
	f.tp_dst = _p.dport.low;
	f.nw_proto = _p.proto.low;
	return f;
}
Rule_Ipv4_ACL OvsWrap::flow_to_Rule_Ipv4_ACL(const struct flow &f) {
	Rule_Ipv4_ACL r;
	r.sip.high = r.sip.low = __swab32p(&f.nw_src);
	r.dip.high = r.dip.low = __swab32p(&f.nw_dst);
	r.sport.high = r.sport.low = __swab16p(&f.tp_src);
	r.dport.high = r.dport.low = __swab16p(&f.tp_dst);
	r.proto.high = r.proto.low = f.nw_proto;
	return r;
}
OvsWrap::rule_id_t OvsWrap::search(const key_vec_t &f) {
	return classifier_lookup(&cls, version, const_cast<key_vec_t*>(&f), nullptr);
}

bool OvsWrap::insert(struct cls_rule *rule) {
	auto existing_rule = classifier_find_rule_exactly(&cls, rule, version);
	if (existing_rule) {
		return false;
	} else {
		classifier_insert(&cls, rule, version, nullptr, 0);
		ovs_rules.push_back(rule);
		return true;
	}
}

OvsWrap::rule_id_t OvsWrap::insert(Rule_Ipv4_ACL &r, size_t prio) {
	struct match match = rule_ipv4_acl_to_ovs_match(r);
	struct cls_rule *rule = (struct cls_rule*) xzalloc(sizeof *rule);
	int priority = prio;
	cls_rule_init(rule, &match, priority);
	if (insert(rule)) {
		// std::cout << r << " inserted" << std::endl;
		ovs_rule_to_pcv_rule[rule] = &r;
		return rule;
	} else {
		free(rule);
		std::cout << r << " already exists" << std::endl;
		return nullptr;
	}
}
const Rule_Ipv4_ACL* OvsWrap::cls_rule_get_pcv_rule(const struct cls_rule *r) {
	return ovs_rule_to_pcv_rule[r];
}

OvsWrap::~OvsWrap() {
	for (auto r : ovs_rules)
		classifier_remove(&cls, r);
	classifier_destroy(&cls);
	for (auto r : ovs_rules)
		free(r);
}

}
}
