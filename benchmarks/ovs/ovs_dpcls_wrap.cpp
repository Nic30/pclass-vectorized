#include "ovs_dpcls_wrap.h"

#include "rule_ipv4_acl_to_ovs_match.h"
#include <pcv/common/range.h>
#include <byteswap.h>

namespace pcv {
namespace ovs {

OvsDpclsWrap::OvsDpclsWrap() {
	dpcls_init(&cls);
}

struct netdev_flow_key OvsDpclsWrap::flow_from_packet(const packet_t &p) {
	struct flow f;
	memset(&f, 0, sizeof f);
	auto _p = rule_conv_fn::exact_array_to_rule_le(p);
	struct match match = rule_ipv4_acl_to_ovs_match(_p);
	struct netdev_flow_key k;
	struct netdev_flow_key mask;
	netdev_flow_mask_init(&mask, &match);
	netdev_flow_key_init_masked(&k, &match.flow, &mask);

	return k;
}
Rule_Ipv4_ACL OvsDpclsWrap::flow_to_Rule_Ipv4_ACL(const struct flow &f) {
	Rule_Ipv4_ACL r;
	r.sip.high = r.sip.low = __swab32p(&f.nw_src);
	r.dip.high = r.dip.low = __swab32p(&f.nw_dst);
	r.sport.high = r.sport.low = __swab16p(&f.tp_src);
	r.dport.high = r.dport.low = __swab16p(&f.tp_dst);
	r.proto.high = r.proto.low = f.nw_proto;
	return r;
}
OvsDpclsWrap::rule_id_t OvsDpclsWrap::search(const key_vec_t &f) {
	const struct netdev_flow_key *keys[1] = { const_cast<key_vec_t*>(&f), };
	struct dpcls_rule *rules[1];
	int num_lookups_p;
	bool all_miniflows_found_corresponding_rule = dpcls_lookup(&cls, keys,
			rules, 1, &num_lookups_p);
	if (num_lookups_p != 1) {
		assert(num_lookups_p == 0);
		assert(all_miniflows_found_corresponding_rule == false);
		return nullptr;
	} else {
		return rules[0];
	}
}

void OvsDpclsWrap::insert(Rule_Ipv4_ACL &r, size_t) {
	struct match match = rule_ipv4_acl_to_ovs_match(r);
	struct netdev_flow_key mask;

	match.wc.masks.in_port.odp_port = 0;
	netdev_flow_mask_init(&mask, &match);
	match.wc.masks.in_port.odp_port = 0;

	/* Make sure wc does not have metadata. */
	ovs_assert(
			!FLOWMAP_HAS_FIELD(&mask.mf.map, metadata) && !FLOWMAP_HAS_FIELD(&mask.mf.map, regs));

	/* Do not allocate extra space. */
	struct dp_netdev_flow *flow;
	flow = (struct dp_netdev_flow*) xmalloc(
			sizeof *flow - sizeof flow->cr.flow.mf + mask.len);
	memset(&flow->stats, 0, sizeof flow->stats);
	flow->dead = false;
	flow->batch = NULL;
	flow->mark = INVALID_FLOW_MARK;
	*CONST_CAST(unsigned*, &flow->pmd_id) = 0;
	*CONST_CAST(struct flow*, &flow->flow) = match.flow;
	*CONST_CAST(ovs_u128*, &flow->ufid) = { 0, 0 };
	ovs_refcount_init(&flow->ref_cnt);
	//struct nlattr *actions;
	// ovsrcu_set(&flow->actions, dp_netdev_actions_create(actions, 0));
	flow->actions = { };

	dp_netdev_get_mega_ufid(&match, CONST_CAST(ovs_u128*, &flow->mega_ufid));
	netdev_flow_key_init_masked(&flow->cr.flow, &match.flow, &mask);

	/* Select dpcls for in_port. Relies on in_port to be exact match. */
	dpcls_insert(&cls, &flow->cr, &mask);

	// cmap_insert(&pmd->flow_table, CONST_CAST(struct cmap_node *, &flow->node),
	//             dp_netdev_flow_hash(&flow->ufid));
}
const Rule_Ipv4_ACL* OvsDpclsWrap::cls_rule_get_pcv_rule(
		const struct dpcls_rule *r) {
	return ovs_rule_to_pcv_rule[r];
}

OvsDpclsWrap::~OvsDpclsWrap() {
	//dpcls_destroy(&cls);
	//for (auto r : ovs_rules)
	//	free(r);
}

}
}
