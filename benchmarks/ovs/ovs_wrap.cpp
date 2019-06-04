#include "ovs_wrap.h"

namespace pcv {
namespace ovs {

OvsWrap::OvsWrap() {
	classifier_init(&cls, nullptr);
	version = 0;
}

struct flow OvsWrap::flow_from_packet(const packet_t & p) {
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
const struct cls_rule * OvsWrap::search(struct flow & f) {
	return classifier_lookup(&cls, version, &f, nullptr);
}

const struct cls_rule * OvsWrap::search(packet_t & p) {
	auto f = flow_from_packet(p);
	return classifier_lookup(&cls, version, &f, nullptr);
}

void OvsWrap::insert(Rule_Ipv4_ACL & r, size_t v) {
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

	struct cls_rule * rule = (struct cls_rule *) xzalloc(sizeof *rule);
	int priority = v;
	cls_rule_init(rule, &match, priority);
	auto existing_rule = classifier_find_rule_exactly(&cls, rule, version);
	if (existing_rule) {
		free(rule);
		std::cout << r << " already exists" << std::endl;
	} else {
		classifier_insert(&cls, rule, version, nullptr, 0);
		ovs_rules.push_back(rule);
		ovs_rule_to_pcv_rule[rule] = &r;
		//std::cout << r << " inserted" << std::endl;
	}
}
const Rule_Ipv4_ACL * OvsWrap::cls_rule_get_pcv_rule(
		const struct cls_rule * r) {
	return ovs_rule_to_pcv_rule[r];
}

OvsWrap::~OvsWrap() {
	//classifier_destroy(&cls);
	//for (auto r : ovs_rules)
	//	free(r);
}

}
}
