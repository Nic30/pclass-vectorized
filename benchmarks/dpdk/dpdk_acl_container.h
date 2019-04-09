#pragma once
#include <string>
#include <array>
#include <vector>
#include "dpdk_acl_field_defs.h"
#include <pcv/rule_parser/rule.h>

class DpdkAclContainer {
public:
	struct acl_config_t {
		struct rte_acl_ctx *acx_ipv4;
#ifdef L3FWDACL_DEBUG
		struct acl4_rule *rule_ipv4;
#endif
	};
	acl_config_t acl_config;
	DpdkAclContainer(const std::vector<pcv::iParsedRule*> & rules);
	~DpdkAclContainer();

	uint16_t search(std::array<uint16_t, 7> & val);
private:
	void acl_init(const std::vector<pcv::iParsedRule*> & rules);
	static constexpr const char *L3FWD_ACL_IPV4_NAME = "l3fwd-acl-ipv4";
	struct rte_acl_ctx* setup_acl(const std::vector<acl4_rule> rules_acl,
			const std::vector<acl4_rule> rules_route, int socketid);
};
