#pragma once
#include <rte_acl.h>
#include <string>

class DpdkAclContainer {
public:
	struct acl_config_t {
		struct rte_acl_ctx *acx_ipv4;
		struct rte_acl_ctx *acx_ipv6;
#ifdef L3FWDACL_DEBUG
		struct acl4_rule *rule_ipv4;
		struct acl6_rule *rule_ipv6;
#endif
	};
	acl_config_t acl_config;
	DpdkAclContainer(const std::string & rule_ipv4_name,
			const std::string & rule_ipv6_name);

private:
	void acl_init(const std::string & rule_ipv4_name,
			const std::string & rule_ipv6_name);
	static constexpr const char *L3FWD_ACL_IPV4_NAME = "l3fwd-acl-ipv4";
	static constexpr const char *L3FWD_ACL_IPV6_NAME = "l3fwd-acl-ipv6";
	struct rte_acl_ctx* setup_acl(struct rte_acl_rule *route_base,
			struct rte_acl_rule *acl_base, unsigned int route_num,
			unsigned int acl_num, int ipv6, int socketid);

};
