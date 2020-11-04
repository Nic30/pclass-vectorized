#include <vector>
#include <fstream>
#include <assert.h>
#include <chrono>

#ifdef OVS_DPCLS
#include "ovs_dpcls_wrap.h"
using OvsWrap = pcv::ovs::OvsDpclsWrap;
#else

#include "ovs_wrap.h"
#ifdef OVS_PCV
#include <classifier-private.h>
#endif

#endif

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/utils/benchmark_common.h>
#include "../run_benchmark.h"

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;
using namespace pcv::ovs;


int main(int argc, const char * argv[]) {
	const char * rule_file = argv[1];
	const char * trace_file = argv[2];
	size_t LOOKUP_CNT = atoll(argv[3]);

	OvsWrap cls;
	auto rules = parse_ruleset_file(rule_file);
	auto packets = OvsTraceLoder().load_from_file(trace_file);
	vector<iParsedRule*> _rules;
	BenchmarkStats stats(LOOKUP_CNT, rules.size(), packets.size());
	stats.construction_start();
	for (auto _r : rules) {
		auto r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
		cls.insert(*r, _r.second);
		_rules.push_back(r);
	}
	stats.construction_stop();

	run_benchmark_lookup_ptr(cls, stats, packets, LOOKUP_CNT);

#ifdef OVS_PCV
	auto pcv_cls = reinterpret_cast<struct classifier_priv*>(cls.cls.priv);
	stats.set_number_of_tries_or_tables(pcv_cls->cls.tree_cnt);
	// size_t i = 0;
	for (auto & t: pcv_cls->cls.trees) {
		if (t->rules.size() == 0)
			continue;
		assert(t->used_dim_cnt <= 7);
		// auto fname = string("pcv_") + to_string(i) + string(".dot");
		// ofstream of(fname);
		// of << t->tree;
		// of.close();
		// i++;
	}
#else
	auto v = reinterpret_cast<const pvector_impl**>(&cls.cls.subtables);
	stats.set_number_of_tries_or_tables((*v)->size);
#endif

	stats.dump();
	return 0;
}

