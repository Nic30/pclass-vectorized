#include <iostream>
#include <assert.h>
#include <chrono>

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include "dpdk_acl_container.h"

using namespace pcv;
using namespace std;

int main(int argc, char **argv) {
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
	argc -= ret;
	argv += ret;

	assert(argc == 1 + 4 && "expected <rule file> <UNIQUE_TRACE_CNT> <LOOKUP_CNT>");
	const char * rules_file_name = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);
	bool dump_as_json = atoll(argv[4]);

	// load rules from the file
	RuleReader rp;
	auto rules = rp.parse_rules(rules_file_name);

	DpdkAclContainer dpdk_acl(rules);

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&rules),
			UNIQUE_TRACE_CNT);

	auto start = chrono::system_clock::now();
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
		dpdk_acl.search(p);
	}
	auto end = chrono::system_clock::now();
	auto us = chrono::duration_cast<chrono::microseconds>(end - start).count();
	auto lookup_speed = (LOOKUP_CNT / double(us));
	if (dump_as_json) {
		cout << "{ \"lookup_speed\":" << lookup_speed << "}";
	} else {
		cout << "[INFO] lookup speed:" << lookup_speed << "MPkts/s" << endl;
	}
	return 0;
}
