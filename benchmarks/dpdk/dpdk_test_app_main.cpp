#include <iostream>
#include <assert.h>

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/utils/benchmark_common.h>
#include "dpdk_acl_container.h"
#include "../run_benchmark.h"

using namespace pcv;
using namespace std;

/*
 * DPDK ACL classifier benchmark
 * */
int main(int argc, char **argv) {
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
	argc -= ret;
	argv += ret;

	assert(argc == 1 + 3 && "expected <rule file> <UNIQUE_TRACE_CNT> <LOOKUP_CNT>");
	const char * rules_file_name = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);

	// load rules from the file
	RuleReader rp;
	auto rules = rp.parse_rules(rules_file_name);

	BenchmarkStats stats(LOOKUP_CNT, rules.size(), UNIQUE_TRACE_CNT);
	stats.construction_start();
	DpdkAclContainer cls(rules);
	stats.construction_stop();

	cls.dump();

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&rules),
			UNIQUE_TRACE_CNT, 0, true);

	stats.set_number_of_tries_or_tables(cls.get_number_of_tries());

	run_benchmark_lookup<DpdkAclContainer>(cls, stats, packets, LOOKUP_CNT);

	stats.dump();
	return 0;
}
