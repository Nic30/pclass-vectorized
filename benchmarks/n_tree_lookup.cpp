#include <vector>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/utils/benchmark_common.h>
#include "run_benchmark.h"

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

int main(int argc, const char * argv[]) {
	assert(argc == 1 + 3);
	const char * rule_file = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);

	using BTree = BTreeImp<uint16_t, IntRuleValue, 7, 8>;
	using Classifier = PartitionSortClassifer<BTree, 64, 10>;
	Classifier cls;

	// load rules from the file
	auto _rules = parse_ruleset_file(rule_file);
	BenchmarkStats stats(LOOKUP_CNT, _rules.size(), UNIQUE_TRACE_CNT);
	stats.construction_start();
	vector<const Rule_Ipv4_ACL*> rules;
	{
		// load rules in to a classifier tree
		size_t i = 0;
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
			BTree::rule_spec_t r = { rule_to_array_16b(*__r), {
					(Classifier::priority_t) __r->cummulative_prefix_len(),
					(Classifier::rule_id_t) _r.second } };
			//std::cout << *__r << std::endl;
			cls.insert(r);
			rules.push_back(__r);
			i++;
		}
	}
	stats.construction_stop();

	// generate packets
	auto packets = generate_packets_from_ruleset(rules, UNIQUE_TRACE_CNT);
	stats.set_number_of_tries_or_tables(cls.tree_cnt);

	run_benchmark_lookup_struct(cls, stats, packets, LOOKUP_CNT);
	stats.dump();
	return 0;
}
