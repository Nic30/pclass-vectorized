#pragma once
#include <inttypes.h>

int add_rules(const char *rule_path, struct rte_acl_rule **proute_base,
		unsigned int *proute_num, struct rte_acl_rule **pacl_base,
		unsigned int *pacl_num, uint32_t rule_size,
		int (*parser)(char *, struct rte_acl_rule*, int));

int parse_cb_ipv4vlan_rule(char *str, struct rte_acl_rule *v, int has_userdata);
int parse_cb_ipv6_rule(char *str, struct rte_acl_rule *v, int has_userdata);
