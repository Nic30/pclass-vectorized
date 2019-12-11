#include <vector>
#include <limits>
#include <algorithm>

#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/utils/benchmark_common.h>
#include "run_benchmark.h"

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

/*
 * Benchmark which uses random rules to test a PartitionSortClassifer
 * */
int main(int argc, const char *argv[]) {
	assert(argc == 1 + 3);
	size_t RANGE_CNT = atoll(argv[1]);
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);

	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 7, (1<<16) - 1, 8>>;
	using Classifier = PartitionSortClassifer<BTree, 64, 10>;
	Classifier cls;

	// load rules from the file
	vector<const Rule_Ipv4_ACL*> rules;
	{
		mt19937_64 rand(0);
		std::uniform_int_distribution<int> distr_size(0, std::max(1ul, std::numeric_limits<uint16_t>::max()/RANGE_CNT));
		// load rules in to a classifier tree
		size_t x = 0;
		for (size_t i = 0; i < RANGE_CNT; i++) {
			Rule_Ipv4_ACL _r();
			auto size = distr_size();
			_r.dip = {i, i + size};

			BTree::rule_spec_t r = { rule_to_array_16b(_r), {
					(Classifier::priority_t) _r->cummulative_prefix_len(),
					(Classifier::rule_id_t) i } };
			cls.insert(r);
		}
	}
	BenchmarkStats stats(LOOKUP_CNT, rules.size(), UNIQUE_TRACE_CNT);
	stats.construction_start();

	stats.construction_stop();

	// generate packets
	auto packets = generate_packets_from_ruleset(rules, UNIQUE_TRACE_CNT);
	stats.set_number_of_tries_or_tables(cls.tree_cnt);

	run_benchmark_lookup_struct(cls, stats, packets, LOOKUP_CNT);
	stats.dump();
	return 0;
}
