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
	struct acl_config_t {
		struct rte_acl_ctx *acx_ipv4;
	};
	acl_config_t acl_config;
	/*
	 * @param rules the initial ruleset for this classifier
	 * */
	DpdkAclContainer(const std::vector<pcv::iParsedRule*> & rules);
	~DpdkAclContainer();

	uint16_t search(std::array<uint16_t, 7> & val);

private:
	static constexpr const char *L3FWD_ACL_IPV4_NAME = "l3fwd-acl-ipv4";

	void acl_init(const std::vector<pcv::iParsedRule*> & rules);
	struct rte_acl_ctx* setup_acl(const std::vector<acl4_rule> rules_acl,
			const std::vector<acl4_rule> rules_route, int socketid);
	//void dump();
};