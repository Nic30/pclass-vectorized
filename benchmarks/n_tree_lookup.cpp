#include <vector>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/utils/benchmark_common.h>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

int main(int argc, const char * argv[]) {
	assert(argc == 1 + 4);
	const char * rule_file = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);
	bool dump_as_json = atoll(argv[4]);

	if (not dump_as_json)
		cout << "[INFO] Executing benchmark " << argv[0] << endl;
	using BTree = BTreeImp<uint16_t, 7, 8>;
	using Classifier = PartitionSortClassifer<BTree, 64, 10>;
	Classifier t;

	// load rules from the file
	auto _rules = parse_ruleset_file(rule_file);
	BenchmarkStats stats(LOOKUP_CNT, dump_as_json, _rules.size());
	stats.construction_start();
	vector<const Rule_Ipv4_ACL*> rules;
	{
		// load rules in to a classifier tree
		size_t i = 0;
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
			BTree::rule_spec_t r = { rule_to_array_16b(*__r), {
					(Classifier::priority_t)__r->cummulative_prefix_len(),
					(Classifier::rule_id_t)_r.second } };
			//std::cout << *__r << std::endl;
			t.insert(r);
			rules.push_back(__r);
			i++;
		}
	}
	stats.construction_stop();

	// generate packets
	auto packets = generate_packets_from_ruleset(rules, UNIQUE_TRACE_CNT);
	stats.set_number_of_tries_or_tables(t.tree_cnt);
	stats.lookup_start();

	Classifier::rule_id_t res = 0;
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
		auto r = t.search(p);
		//std::cout << r << std::endl;
		res |= r.rule_id;
	}
	// this is there to assert the search is not optimised out
	if (res == 0) {
		throw std::runtime_error("probably wrong result");
	}
	stats.lookup_stop();
	stats.dump();
	return 0;
}
