#include "dpdk_acl_container.h"
#include "classbench_rule_parser.h"
#include "dpdk_acl_rule_dump.h"
#include "log.h"
#include <pcv/rule_parser/rule.h>

namespace dpdk_acl_configs {

template<typename RULE_T>
void get_config(struct rte_acl_config * ctx);

template<>
void get_config<pcv::Rule_Ipv4>(struct rte_acl_config * cfg) {
	struct ipv4_tuple {
		uint32_t sip;
		uint32_t dip;
		uint16_t srcp;
		uint16_t dstp;
		uint8_t proto;
	};


	const uint32_t ipv4_tuple_layout[] = {
		offsetof(struct ipv4_tuple, sip),
		offsetof(struct ipv4_tuple, dip),
		offsetof(struct ipv4_tuple, srcp),
		offsetof(struct ipv4_tuple, dstp),
		offsetof(struct ipv4_tuple, proto),
	};
	static const struct rte_acl_field_def ipv4_defs[] = {
		{
			// sip
			.type = RTE_ACL_FIELD_TYPE_BITMASK,
			.size = sizeof(uint32_t),
			.field_index = 0,
			.input_index = 0,
		},
		{
			// dip
			.type = RTE_ACL_FIELD_TYPE_BITMASK,
			.size = sizeof(uint32_t),
			.field_index = 1,
			.input_index = 1,
		},
		{
			// sport
			.type = RTE_ACL_FIELD_TYPE_RANGE,
			.size = sizeof(uint16_t),
			.field_index = 2,
			.input_index = 2,
		},
		{
			//dport
			.type = RTE_ACL_FIELD_TYPE_RANGE,
			.size = sizeof(uint16_t),
			.field_index = 3,
			.input_index = 3,
		},
		{
			//proto
			.type = RTE_ACL_FIELD_TYPE_RANGE,
			.size = sizeof(uint16_t),
			.field_index = 4,
			.input_index = 4,
		},
	};

	memcpy(&cfg->defs, ipv4_defs, sizeof(ipv4_defs));
	cfg->num_fields = RTE_DIM(ipv4_defs);

	for (int i = 0; i < 5; i++) {
		cfg->defs[i].offset = ipv4_tuple_layout[i];
	}
	cfg->num_categories = 5;
	cfg->num_fields = 5;
}

}


DpdkAclContainer::DpdkAclContainer(const std::string & rule_ipv4_name,
		const std::string & rule_ipv6_name) {
	acl_init(rule_ipv4_name, rule_ipv6_name);
}

void DpdkAclContainer::acl_init(const std::string & rule_ipv4_name,
		const std::string & rule_ipv6_name) {
	struct rte_acl_rule *acl_base_ipv4, *route_base_ipv4, *acl_base_ipv6,
			*route_base_ipv6;
	unsigned int acl_num_ipv4 = 0, route_num_ipv4 = 0, acl_num_ipv6 = 0,
			route_num_ipv6 = 0;

	/* Load  rules from the input file */
	if (rule_ipv4_name != "") {
		if (add_rules(rule_ipv4_name.c_str(), &route_base_ipv4, &route_num_ipv4,
				&acl_base_ipv4, &acl_num_ipv4, sizeof(struct acl4_rule),
				&parse_cb_ipv4vlan_rule) < 0)
			rte_exit(EXIT_FAILURE, "Failed to add rules\n");
	} else {
		acl_base_ipv4 = nullptr;
	}
	acl_log("IPv4 Route entries %u:\n", route_num_ipv4);
	dump_ipv4_rules((struct acl4_rule *) route_base_ipv4, route_num_ipv4, 1);

	acl_log("IPv4 ACL entries %u:\n", acl_num_ipv4);
	dump_ipv4_rules((struct acl4_rule *) acl_base_ipv4, acl_num_ipv4, 1);

	if (rule_ipv6_name != "") {
		if (add_rules(rule_ipv6_name.c_str(), &route_base_ipv6, &route_num_ipv6,
				&acl_base_ipv6, &acl_num_ipv6, sizeof(struct acl6_rule),
				&parse_cb_ipv6_rule) < 0)
			rte_exit(EXIT_FAILURE, "Failed to add rules\n");
	} else {
		acl_base_ipv6 = nullptr;
	}
	acl_log("IPv6 Route entries %u:\n", route_num_ipv6);
	dump_ipv6_rules((struct acl6_rule *) route_base_ipv6, route_num_ipv6, 1);

	acl_log("IPv6 ACL entries %u:\n", acl_num_ipv6);
	dump_ipv6_rules((struct acl6_rule *) acl_base_ipv6, acl_num_ipv6, 1);

	memset(&acl_config, 0, sizeof(acl_config));

	size_t i = 0;
	acl_config.acx_ipv4 = setup_acl(route_base_ipv4, acl_base_ipv4,
			route_num_ipv4, acl_num_ipv4, 0, i);

	acl_config.acx_ipv6 = setup_acl(route_base_ipv6, acl_base_ipv6,
			route_num_ipv6, acl_num_ipv6, 1, i);

	free(route_base_ipv4);
	free(route_base_ipv6);

#ifdef L3FWDACL_DEBUG
	acl_config.rule_ipv4 = (struct acl4_rule *)acl_base_ipv4;
	acl_config.rule_ipv6 = (struct acl6_rule *)acl_base_ipv6;
#else
	free(acl_base_ipv4);
	free(acl_base_ipv6);
#endif
}

struct rte_acl_ctx*
DpdkAclContainer::setup_acl(struct rte_acl_rule *route_base,
		struct rte_acl_rule *acl_base, unsigned int route_num,
		unsigned int acl_num, int ipv6, int socketid) {
	char name[PATH_MAX];
	struct rte_acl_param acl_param;
	struct rte_acl_config acl_build_param;
	dpdk_acl_configs::get_config<pcv::Rule_Ipv4>(&acl_build_param);
	struct rte_acl_ctx *context;
	int dim = ipv6 ? RTE_DIM(ipv6_defs) : RTE_DIM(ipv4_defs);

	/* Create ACL contexts */
	snprintf(name, sizeof(name), "%s%d",
			ipv6 ? L3FWD_ACL_IPV6_NAME : L3FWD_ACL_IPV4_NAME, socketid);

	acl_param.name = name;
	acl_param.socket_id = socketid;
	acl_param.rule_size = RTE_ACL_RULE_SZ(dim);
	acl_param.max_rule_num = std::numeric_limits<uint16_t>::max() - 1;

	if ((context = rte_acl_create(&acl_param)) == NULL)
		rte_exit(EXIT_FAILURE, "Failed to create ACL context\n");

	if (rte_acl_set_ctx_classify(context, RTE_ACL_CLASSIFY_DEFAULT) != 0)
		rte_exit(EXIT_FAILURE,
				"Failed to setup classify method for  ACL context\n");

	if (route_num) {
		if (rte_acl_add_rules(context, route_base, route_num) < 0)
			rte_exit(EXIT_FAILURE, "add rules failed\n");
	}
	if (acl_num) {
		if (rte_acl_add_rules(context, acl_base, acl_num) < 0)
			rte_exit(EXIT_FAILURE, "add rules failed\n");
	}

	if (rte_acl_build(context, &acl_build_param) != 0)
		rte_exit(EXIT_FAILURE, "Failed to build ACL trie\n");

	rte_acl_dump(context);

	return context;
}
