#include <vector>
#include <fstream>
#include <assert.h>
#include <chrono>

#include "ovs_wrap.h"
#ifdef OVS_PCV
#include <classifier-private.h>
#endif

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;
using namespace pcv::ovs;

int main(int argc, const char * argv[]) {
	assert(argc == 1 + 4);
	const char * rule_file = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);
	bool dump_as_json = atoll(argv[4]);

	if (not dump_as_json)
		cout << "[INFO] Executing benchmark " << argv[0] << endl;

	OvsWrap cls;
	auto rules = parse_ruleset_file(rule_file);
	vector<iParsedRule*> _rules;
#ifdef OVS_PCV
	size_t rule_i = 0;
#endif
	for (auto _r : rules) {
		auto r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
		cls.insert(*r, _r.second);
		_rules.push_back(r);
#ifdef OVS_PCV
		size_t tree_i = 0;
		auto _cls = reinterpret_cast<struct classifier_priv*>(cls.cls.priv);
		for (auto & t : _cls->cls.trees) {
			if (t->rules.size()) {
				ofstream of(
						string("dump/tree_") + to_string(rule_i) + "_"
								+ to_string(tree_i) + ".dot", ofstream::out);
				of << t->tree;
				of.close();
			}
			tree_i++;
		}
		rule_i++;
#endif
	}

	// generate packets
	auto _packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&_rules),
			UNIQUE_TRACE_CNT, 0, true);
	// to rm flow generating overhead from look up
	vector<struct flow> packets;
	for (auto & _p : _packets) {
		packets.push_back(OvsWrap::flow_from_packet(_p));
	}

	auto start = chrono::system_clock::now();
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
		// cout << "search:" << exact_array_to_rule_be(p) << endl;
		// cout << "search:" << exact_array_to_rule_le(p) << endl;
		auto v = cls.search(p);
		//if (v) {
		//	cout << "found : " << *cls.cls_rule_get_pcv_rule(v) << " priority: "
		//			<< v->priority << endl;
		//}
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


//auto r = new Rule_Ipv4_ACL();
//r->sip = {1,1};
//_rules.push_back(r);
//r = new Rule_Ipv4_ACL();
//r->sip = {2,2};
//_rules.push_back(r);
//r = new Rule_Ipv4_ACL();
//r->dip = {2,2};
//_rules.push_back(r);
//r = new Rule_Ipv4_ACL();
//r->dip = {0,4};
//_rules.push_back(r);
