#include <iostream>
#include <assert.h>
#include "dpdk_acl_field_defs.h"
#include "dpdk_acl_container.h"

int main(int argc, char **argv) {
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
	argc -= ret;
	argv += ret;

	assert(argc == 1 + 2 && "expected ipv4 and ipv6 rule file");
	const char * rule_ipv4_name = argv[1];
	const char * rule_ipv6_name = argv[2];
	DpdkAclContainer dpdk_acl(rule_ipv4_name, rule_ipv6_name);

	std::cout << "[EXIT]" << std::endl;
	return 0;
}
