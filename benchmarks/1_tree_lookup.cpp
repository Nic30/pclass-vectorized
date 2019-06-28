#include <vector>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
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

	BTree t;

	// load rules from the file
	vector<iParsedRule*> _rules;
	auto rules = parse_ruleset_file(rule_file);
	BenchmarkStats stats(LOOKUP_CNT, dump_as_json, 0);
	stats.construction_start();
	{
		// load rules in to a classifier tree
		for (auto _r : rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
			BTree::rule_spec_t r = { rule_to_array_16b(*__r), {
					(BTree::priority_t) __r->cummulative_prefix_len(),
					(BTree::rule_id_t) _r.second } };
			if (not t.does_rule_colide(r)) {
				t.insert(r);
				_rules.push_back(__r);
			}
		}
		if (not dump_as_json)
			cout << "[INFO] Loaded non-colliding rules cnt:" << _rules.size()
					<< endl;
	}
	stats.construction_stop();
	stats.real_rule_cnt = _rules.size();

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&_rules),
			UNIQUE_TRACE_CNT);

	stats.set_number_of_tries_or_tables(1);
	stats.lookup_start();
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
		t.search(p);
	}
	stats.lookup_stop();

	stats.dump();

	return 0;
}
