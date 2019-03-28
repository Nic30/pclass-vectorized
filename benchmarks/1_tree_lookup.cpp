#include <vector>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <chrono>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

int main(int argc, const char * argv[]) {
	assert(argc == 1 + 3);
	const char * rule_file = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);
	cout << "[INFO] Executing benchmark " << argv[0] << endl;
	using BTree = BTreeImp<uint16_t, 7, 8>;

	BTree t;

	// load rules from the file
	vector<iParsedRule*> _rules;
	RuleReader rp;
	_rules = rp.parse_rules(rule_file);
	{
		// load rules in to a classifier tree
		size_t i = 0;
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4*>(_r);
			BTree::rule_spec_t r = { rule_to_array<uint16_t, 7>(*__r), i };
			if (not t.does_rule_colide(r)) {
				t.insert(r);
				i++;
			}
		}
		cout << "[INFO] Loaded non-colliding rules cnt:" << i << endl;
	}

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4*>*>(&_rules),
			UNIQUE_TRACE_CNT);

	auto start = chrono::system_clock::now();
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
		t.search(p);
	}
	auto end = chrono::system_clock::now();
	auto us = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "[INFO] lookup speed:" << (LOOKUP_CNT / double(us)) << "MPkts/s" << endl;

	return 0;
}
