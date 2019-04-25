#include <rte_acl.h>

#include <algorithm>
#include <string>
#include <byteswap.h>

#include <pcv/rule_parser/rule.h>
#include <pcv/rule_parser/classbench_rule_parser.h>

#include "log.h"
#include "dpdk_acl_container.h"
#include "dpdk_acl_rule_dump.h"
#include "dpdk_acl_field_defs.h"

using namespace pcv;

namespace dpdk_acl_configs {

template<typename RULE_T>
void get_config(struct rte_acl_config * ctx, size_t num_categories);

template<>
void get_config<pcv::Rule_Ipv4_ACL>(struct rte_acl_config * cfg,
		size_t num_categories) {
	memcpy(&cfg->defs, ipv4_defs, sizeof(ipv4_defs));
	cfg->num_fields = RTE_DIM(ipv4_defs);
	cfg->num_categories = num_categories;
	cfg->max_size = 0;
}

}

DpdkAclContainer::DpdkAclContainer(const std::vector<iParsedRule*> & rules) {
	acl_init(rules);
}

template<typename RULE_T>
void add_rules(const std::vector<iParsedRule*> & rules,
		std::vector<RULE_T> & rules_acl) {
	int i = 0;
	for (auto r : rules) {
		RULE_T * next;
		auto ipv4_acl_r = dynamic_cast<Rule_Ipv4_ACL*>(r);
		//std::cerr << *ipv4_acl_r << std::endl;
		rules_acl.push_back( { });
		next = &rules_acl.back();
		memset(next, 0, sizeof(*next));

		next->field[SRC_FIELD_IPV4].value.u32 = __bswap_32(ipv4_acl_r->sip.low);
		next->field[SRC_FIELD_IPV4].mask_range.u32 =
				ipv4_acl_r->sip.get_mask_bigendian();

		next->field[DST_FIELD_IPV4].value.u32 = __bswap_32(ipv4_acl_r->dip.low);
		next->field[DST_FIELD_IPV4].mask_range.u32 =
				ipv4_acl_r->dip.get_mask_bigendian();

		next->field[SRCP_FIELD_IPV4].value.u16 = ipv4_acl_r->sport.low;
		next->field[SRCP_FIELD_IPV4].mask_range.u16 =
				ipv4_acl_r->sport.high;

		next->field[DSTP_FIELD_IPV4].value.u16 = ipv4_acl_r->dport.low;
		next->field[DSTP_FIELD_IPV4].mask_range.u16 =
				ipv4_acl_r->dport.high;

		next->field[PROTO_FIELD_IPV4].value.u16 = ipv4_acl_r->proto.low;
		next->field[PROTO_FIELD_IPV4].mask_range.u16 =
				ipv4_acl_r->proto.get_mask_littleendian();

		//print_one_ipv4_rule(next, 0);
		next->data.userdata = i + 1;
		next->data.priority = RTE_ACL_MAX_PRIORITY - rules_acl.size();
		next->data.category_mask = -1;
		i++;
	}
}

void DpdkAclContainer::acl_init(const std::vector<iParsedRule*> & rules) {
	std::vector<acl4_rule> rules_acl;

	/* Load  rules from the input file */
	add_rules<acl4_rule>(rules, rules_acl);
	acl_ctx = setup_acl(rules_acl, 0);
}

struct rte_acl_ctx*
DpdkAclContainer::setup_acl(const std::vector<acl4_rule> & rules_acl,
		int socketid) {
	struct rte_acl_param acl_param;
	struct rte_acl_config acl_build_param;
	dpdk_acl_configs::get_config<pcv::Rule_Ipv4_ACL>(&acl_build_param,
			NUM_CATEGORIES);
	int dim = RTE_DIM(ipv4_defs);

	acl_param.name = ACL_NAME;
	acl_param.socket_id = socketid;
	acl_param.rule_size = RTE_ACL_RULE_SZ(dim);
	acl_param.max_rule_num = std::numeric_limits<uint16_t>::max() - 1;

	struct rte_acl_ctx *context;
	if ((context = rte_acl_create(&acl_param)) == NULL)
		rte_exit(EXIT_FAILURE, "Failed to create ACL context\n");

	if (rte_acl_set_ctx_classify(context, RTE_ACL_CLASSIFY_DEFAULT) != 0)
		rte_exit(EXIT_FAILURE,
				"Failed to setup classify method for  ACL context\n");

	if (rules_acl.size()) {
		if (rte_acl_add_rules(context,
				(const struct rte_acl_rule*) &rules_acl[0], rules_acl.size())
				< 0)
			rte_exit(EXIT_FAILURE, "add rules failed\n");
	}

	if (rte_acl_build(context, &acl_build_param) != 0)
		rte_exit(EXIT_FAILURE, "Failed to build ACL trie\n");

	return context;
}

uint16_t DpdkAclContainer::search(std::array<uint16_t, 7> & val_big_endian) {
	std::array<uint32_t, NUM_CATEGORIES> res_cls;
	const uint8_t * data[1] = { (uint8_t *) &val_big_endian[0] };
	auto res = rte_acl_classify(acl_ctx, data, &res_cls[0], 1, NUM_CATEGORIES);
	if (res < 0) {
		throw std::runtime_error(
				std::string("Error in rte_acl_classify ")
						+ std::to_string(res));
	}
	//std::cout << "search:";
	//for (auto i : res_cls) {
	//	std::cout << i << " ";
	//}
    //
	//std::cout << res << std::endl;
	return (uint16_t) *std::max_element(res_cls.begin(), res_cls.end());
}

DpdkAclContainer::~DpdkAclContainer() {
	rte_acl_free(acl_ctx);
}

/*
 * Original structure from dpdk is not in public headers and this is only beggining of it
 * */
struct __rte_acl_ctx__only_part {
	char                name[RTE_ACL_NAMESIZE];
	/** Name of the ACL context. */
	int32_t             socket_id;
	/** Socket ID to allocate memory from. */
	enum rte_acl_classify_alg alg;
	void               *rules;
	uint32_t            max_rules;
	uint32_t            rule_sz;
	uint32_t            num_rules;
	uint32_t            num_categories;
	uint32_t            num_tries;
	uint32_t            match_index;
	uint64_t            no_match;
	uint64_t            idle;
	uint64_t           *trans_table;
	uint32_t           *data_indexes;
};

int DpdkAclContainer::get_number_of_tries() {
	return ((__rte_acl_ctx__only_part *)acl_ctx)->num_tries;
}

void DpdkAclContainer::dump() {
//	acl_log("IPv4 Route entries %zu:\n", rules_route.size());
//	dump_ipv4_rules((acl4_rule *) &rules_route[0], rules_route.size(), 1);
//
	rte_acl_dump(acl_ctx);
}
