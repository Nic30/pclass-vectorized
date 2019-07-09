#include <vector>

#include "list_classifier.h"
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/utils/benchmark_common.h>
#include "../run_benchmark.h"

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

using Classifier = ListBasedClassifier<uint16_t, IntRuleValue, 7>;

int main(int argc, const char * argv[]) {
	assert(argc == 1 + 3);
	const char * rule_file = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);

	Classifier cls;
	// load rules from the file
	vector<iParsedRule*> _rules;
	auto rules = parse_ruleset_file(rule_file);
	BenchmarkStats stats(LOOKUP_CNT, rules.size(), UNIQUE_TRACE_CNT);
	stats.construction_start();
	{
		// load rules in to a classifier tree
		for (auto _r : rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
			Classifier::rule_spec_t r = { rule_to_array_16b(*__r), {
					(Classifier::priority_t) __r->cummulative_prefix_len(),
					(Classifier::rule_id_t) _r.second } };
			cls.insert(r);
			_rules.push_back(__r);
		}
	}
	cls.prepare();
	stats.construction_stop();

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&_rules),
			UNIQUE_TRACE_CNT);

	stats.set_number_of_tries_or_tables(1);

	run_benchmark_lookup_struct(cls, stats, packets, LOOKUP_CNT);

	stats.dump();

	return 0;
}
