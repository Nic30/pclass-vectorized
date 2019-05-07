#include <vector>
#include <assert.h>
#include <chrono>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include "lib/classifier.h"

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

class OvsWrap {
public:
	classifier cls;
	ovs_version_t version;
	vector<struct cls_rule*> ovs_rules;

	OvsWrap() {
		classifier_init(&cls, nullptr);
		version = 0;
	}

	const struct cls_rule * search(packet_t & p) {
		struct flow f;
		f.nw_src = *((uint32_t*)&p[0]);
		f.nw_dst = *((uint32_t*)&p[2]);
		f.tp_src = p[4];
		f.tp_dst = p[5];
		f.nw_proto = p[6];

		return classifier_lookup(&cls, version, &f, nullptr);
	}

	void insert(Rule_Ipv4_ACL & r, size_t v) {
		struct match match;
		match_init_catchall(&match);

		auto sip = r.sip.to_be();
		match.flow.nw_src = sip.low;
		match.wc.masks.nw_src = r.sip.get_mask_bigendian();

		auto dip = r.dip.to_be();
		match.flow.nw_dst = dip.low;
		match.wc.masks.nw_dst = r.dip.get_mask_bigendian();

		auto sport = r.sport.to_be();
		match.flow.tp_src = sport.low;
		match.wc.masks.tp_src = r.sport.get_mask_bigendian();

		auto dport = r.dport.to_be();
		match.flow.tp_dst = dport.low;
		match.wc.masks.tp_dst = r.dport.get_mask_bigendian();

		match.flow.nw_proto = r.proto.low;
		match.wc.masks.nw_proto = r.proto.get_mask_littleendian();

		struct cls_rule * rule = (struct cls_rule *)xzalloc(sizeof *rule);
		int priority = 0;
		cls_rule_init(rule, &match, priority);
		auto existing_rule = classifier_find_rule_exactly(&cls, rule, version);
		if (!existing_rule) {
			classifier_insert(&cls, rule, version, nullptr, 0);
			ovs_rules.push_back(rule);
			//std::cout <<  r << " inserted" << std::endl;
		} else {
			//std::cout << r << " already exists" << std::endl;
		}
	}
	~OvsWrap() {
		classifier_destroy(&cls);
		for (auto r : ovs_rules)
			free(r);
	}
};

int main(int argc, const char * argv[]) {
	assert(argc == 1 + 4);
	const char * rule_file = argv[1];
	size_t UNIQUE_TRACE_CNT = atoll(argv[2]);
	size_t LOOKUP_CNT = atoll(argv[3]);
	bool dump_as_json = atoll(argv[4]);

	if (not dump_as_json)
		cout << "[INFO] Executing benchmark " << argv[0] << endl;

	OvsWrap cls;
	// load rules from the file
	vector<iParsedRule*> _rules;
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
	RuleReader rp;
	_rules = rp.parse_rules(rule_file);
	{
		// load rules in to a classifier tree
		size_t i = 0;
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r);
			cls.insert(*__r, i);
			i++;
		}
	}

	// generate packets
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&_rules),
			UNIQUE_TRACE_CNT, true);

	auto start = chrono::system_clock::now();
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
		auto v = cls.search(p);
		cout << "found:" << v << endl;
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
