#include "dpdk_acl_container.h"
#include "dpdk_acl_rule_dump.h"
#include "dpdk_acl_field_defs.h"
#include "log.h"
#include <rte_acl.h>

#include <string>
#include <pcv/rule_parser/rule.h>
#include <pcv/rule_parser/classbench_rule_parser.h>

using namespace pcv;

namespace dpdk_acl_configs {

template<typename RULE_T>
void get_config(struct rte_acl_config * ctx);

template<>
void get_config<pcv::Rule_Ipv4_ACL>(struct rte_acl_config * cfg) {
	struct ipv4_tuple {
		uint32_t sip;
		uint32_t dip;
		uint16_t srcp;
		uint16_t dstp;
		uint8_t proto;
	};

	const uint32_t ipv4_tuple_layout[] = { offsetof(struct ipv4_tuple, sip),
			offsetof(struct ipv4_tuple, dip), offsetof(struct ipv4_tuple, srcp),
			offsetof(struct ipv4_tuple, dstp), offsetof(struct ipv4_tuple,
					proto), };
	static const struct rte_acl_field_def ipv4_defs[] = {
	// sip
			{ .type = RTE_ACL_FIELD_TYPE_BITMASK, .size = sizeof(uint32_t),
					.field_index = 0, .input_index = 0, },
			// dip
			{ .type = RTE_ACL_FIELD_TYPE_BITMASK, .size = sizeof(uint32_t),
					.field_index = 1, .input_index = 1, },
			// sport
			{ .type = RTE_ACL_FIELD_TYPE_RANGE, .size = sizeof(uint16_t),
					.field_index = 2, .input_index = 2, },
			//dport
			{ .type = RTE_ACL_FIELD_TYPE_RANGE, .size = sizeof(uint16_t),
					.field_index = 3, .input_index = 3, },
			//proto
			{ .type = RTE_ACL_FIELD_TYPE_RANGE, .size = sizeof(uint16_t),
					.field_index = 4, .input_index = 4, }, };

	memcpy(&cfg->defs, ipv4_defs, sizeof(ipv4_defs));
	cfg->num_fields = RTE_DIM(ipv4_defs);

	for (int i = 0; i < 5; i++) {
		cfg->defs[i].offset = ipv4_tuple_layout[i];
	}
	cfg->num_categories = 5;
	cfg->num_fields = 5;
}

}

DpdkAclContainer::DpdkAclContainer(const std::vector<iParsedRule*> & rules) {
	acl_init(rules);
}

template<typename RULE_T>
void add_rules(const std::vector<iParsedRule*> & rules, std::vector<RULE_T> & rules_route,
		std::vector<RULE_T> & rules_acl) {
	RuleReader rr;
	for (auto r : rules) {
		RULE_T * next;
		auto ipv4_acl_r = dynamic_cast<Rule_Ipv4_ACL*>(r);
		std::cerr << *ipv4_acl_r << std::endl;
		if (ipv4_acl_r) {
			rules_acl.push_back({});
			next = &rules_acl.back();
			next->data.userdata = 0xf0000000 + rules_acl.size();
			next->field[PROTO_FIELD_IPV4].value.u16 = ipv4_acl_r->proto.low;
			next->field[PROTO_FIELD_IPV4].mask_range.u16 =
					ipv4_acl_r->proto.get_mask_littleendian();
			next->field[SRC_FIELD_IPV4].value.u32 = ipv4_acl_r->sip.low;
			next->field[SRC_FIELD_IPV4].mask_range.u32 =
					ipv4_acl_r->sip.get_mask_littleendian();
			next->field[DST_FIELD_IPV4].value.u32 = ipv4_acl_r->dip.low;
			next->field[DST_FIELD_IPV4].mask_range.u32 =
					ipv4_acl_r->dip.get_mask_littleendian();
			next->field[SRCP_FIELD_IPV4].value.u16 = ipv4_acl_r->sport.low;
			next->field[SRCP_FIELD_IPV4].mask_range.u16 =
					ipv4_acl_r->sport.get_mask_littleendian();
			next->field[DSTP_FIELD_IPV4].value.u16 = ipv4_acl_r->dport.low;
			next->field[DSTP_FIELD_IPV4].mask_range.u16 =
					ipv4_acl_r->dport.get_mask_littleendian();

		} else {
			rules_route.push_back({});
			throw std::runtime_error(std::string(__FUNCTION__) + "not implemented");
			next = &rules_route.back();
			/* Check the forwarding port number */
			next->data.userdata += FWD_PORT_SHIFT;
		}

		next->data.priority = RTE_ACL_MAX_PRIORITY
				- (rules_route.size() + rules_acl.size());
		next->data.category_mask = -1;
	}
}

void DpdkAclContainer::acl_init(const std::vector<iParsedRule*> & rules) {
	std::vector<acl4_rule> rules_acl, rules_route;

	/* Load  rules from the input file */
	add_rules<acl4_rule>(rules, rules_acl, rules_route);
	acl_log("IPv4 Route entries %zu:\n", rules_route.size());
	dump_ipv4_rules((acl4_rule *) &rules_route[0], rules_route.size(),
			1);

	acl_log("IPv4 ACL entries %zu:\n", rules_acl.size());
	dump_ipv4_rules((acl4_rule *) &rules_acl[0], rules_acl.size(), 1);

	memset(&acl_config, 0, sizeof(acl_config));

	acl_config.acx_ipv4 = setup_acl(rules_route, rules_acl, 0);
}

struct rte_acl_ctx*
DpdkAclContainer::setup_acl(const std::vector<acl4_rule> rules_acl,
		const std::vector<acl4_rule> rules_route, int socketid) {
	char name[PATH_MAX];
	struct rte_acl_param acl_param;
	struct rte_acl_config acl_build_param;
	dpdk_acl_configs::get_config<pcv::Rule_Ipv4_ACL>(&acl_build_param);
	struct rte_acl_ctx *context;
	int dim = RTE_DIM(ipv4_defs);

	/* Create ACL contexts */
	snprintf(name, sizeof(name), "%s%d", L3FWD_ACL_IPV4_NAME, socketid);

	acl_param.name = name;
	acl_param.socket_id = socketid;
	acl_param.rule_size = RTE_ACL_RULE_SZ(dim);
	acl_param.max_rule_num = std::numeric_limits<uint16_t>::max() - 1;

	if ((context = rte_acl_create(&acl_param)) == NULL)
		rte_exit(EXIT_FAILURE, "Failed to create ACL context\n");

	if (rte_acl_set_ctx_classify(context, RTE_ACL_CLASSIFY_DEFAULT) != 0)
		rte_exit(EXIT_FAILURE,
				"Failed to setup classify method for  ACL context\n");

	if (rules_route.size()) {
		if (rte_acl_add_rules(context,
				(const struct rte_acl_rule*) &rules_route[0],
				rules_route.size()) < 0)
			rte_exit(EXIT_FAILURE, "add rules failed\n");
	}
	if (rules_acl.size()) {
		if (rte_acl_add_rules(context,
				(const struct rte_acl_rule*) &rules_acl[0], rules_acl.size())
				< 0)
			rte_exit(EXIT_FAILURE, "add rules failed\n");
	}

	if (rte_acl_build(context, &acl_build_param) != 0)
		rte_exit(EXIT_FAILURE, "Failed to build ACL trie\n");

	rte_acl_dump(context);

	return context;
}

uint16_t DpdkAclContainer::search(std::array<uint16_t, 7> & val) {
	uint32_t res_cls[5];
	const uint8_t * data[1] = { (uint8_t *) &val[0] };
	auto res = rte_acl_classify(acl_config.acx_ipv4, data, &res_cls[0], 1, 1);
	if (res < 0) {
		throw std::runtime_error(
				std::string("Error in rte_acl_classify ")
						+ std::to_string(res));
	}
	return res_cls[0];
}

DpdkAclContainer::~DpdkAclContainer() {
	rte_acl_free(acl_config.acx_ipv4);
}
