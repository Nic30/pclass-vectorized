#include <iostream>
#include <assert.h>

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/utils/benchmark_common.h>
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

	BenchmarkStats stats(LOOKUP_CNT, dump_as_json, rules.size());
	stats.construction_start();
	DpdkAclContainer dpdk_acl(rules);
	stats.construction_stop();

	dpdk_acl.dump();

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&rules),
			UNIQUE_TRACE_CNT, 0, true);

	stats.set_number_of_tries_or_tables(dpdk_acl.get_number_of_tries());

	stats.lookup_start();
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto _i = i % packets.size();
		auto & p = packets[_i];
		//for (auto pp: p) {
		//	std::cout << pp << " ";
		//}
		//std::cout << std::endl;
		dpdk_acl.search(p);
	}
	stats.lookup_stop();

	stats.dump();
	return 0;
}
