#pragma once
#include <string>
#include <array>
#include <vector>

#include "dpdk_acl_field_defs.h"
#include <pcv/rule_parser/rule.h>

/*
 * The container of dpdk rte_acl object
 * @note the rte_acl is an implementation of the packet classifier
 * */
class DpdkAclContainer {
public:
	using key_vec_t = std::array<uint16_t, 7>;
	using rule_id_t = uint16_t;
	struct rte_acl_ctx *acl_ctx;
	/*
	 * @param rules the initial ruleset for this classifier
	 * */
	DpdkAclContainer(const std::vector<pcv::iParsedRule*> & rules);
	~DpdkAclContainer();

	rule_id_t search(key_vec_t & val);
	void dump();
	int get_number_of_tries();

private:
	static constexpr const char *ACL_NAME = "acl";
	static constexpr const size_t NUM_CATEGORIES = 1;

	void acl_init(const std::vector<pcv::iParsedRule*> & rules);
	struct rte_acl_ctx* setup_acl(const std::vector<acl4_rule> & rules_acl,
			int socketid);
};
