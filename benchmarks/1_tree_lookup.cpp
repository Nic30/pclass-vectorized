#include <vector>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/partition_sort/rule_value_int.h>

#include <pcv/rule_parser/trace_tools.h>
#include <pcv/utils/benchmark_common.h>
#include "run_benchmark.h"

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

/*
 * Benchmark which uses the classbench-ng rules to tests properties of a single BTreeImp
 * */
int main(int argc, const char *argv[]) {
	assert(argc == 1 + 3);
	const char *rule_file = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);

	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 7, (1<<16) - 1, 8>>;
	BTree::NodeAllocator mempool(1024*1024);
	BTree cls(mempool);

	// load rules from the file
	vector<iParsedRule*> _rules;
	auto rules = parse_ruleset_file(rule_file);
	BenchmarkStats stats(LOOKUP_CNT, 0, UNIQUE_TRACE_CNT);
	stats.construction_start();
	{
		// load rules in to a classifier tree
		for (auto _r : rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
			BTree::rule_spec_t r = { rule_to_array_16b(*__r), {
					(BTree::priority_t) __r->cummulative_prefix_len(),
					(BTree::rule_id_t) _r.second } };
			if (not cls.does_rule_colide(r)) {
				cls.insert(r);
				_rules.push_back(__r);
			}
		}
	}
	stats.construction_stop();
	stats.real_rule_cnt = _rules.size();

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&_rules),
			UNIQUE_TRACE_CNT);

	stats.set_number_of_tries_or_tables(1);

	run_benchmark_lookup_struct(cls, stats, packets, LOOKUP_CNT);

	stats.dump();

	return 0;
}
