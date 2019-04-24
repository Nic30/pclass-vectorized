#include <vector>
#include <assert.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include "lib/classifier.h"

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

	classifier cls;
	classifier_init(&cls, nullptr);

	// load rules from the file
	vector<iParsedRule*> _rules;
	RuleReader rp;
	_rules = rp.parse_rules(rule_file);
	{
		// load rules in to a classifier tree
		size_t i = 0;
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r);
			 rule_to_array<uint16_t, 7>(*__r);
		}
	}

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&_rules),
			UNIQUE_TRACE_CNT);

	auto start = chrono::system_clock::now();
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
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
