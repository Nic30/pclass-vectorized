#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <limits>
#include <vector>
#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/b_tree_search.h>
#include <pcv/partition_sort/b_tree_insert.h>
#include <pcv/partition_sort/b_tree_remove.h>
#include <pcv/partition_sort/b_tree_to_rules.h>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/rule_value_int.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/utils/benchmark_common.h>
#include <pcv/rule_parser/rule.h>

#include "../list/list_classifier.h"
#include "ovs_wrap.h"
#include "ovs_api/classifier-private.h"
#include "ovs_api/struct_flow_conversions.h"

BOOST_AUTO_TEST_SUITE(pcv__testsuite)

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;
using namespace pcv::ovs;

void init_cls_from_file(OvsWrap & cls, const std::string & ruleset_file_name) {
	auto _rules = parse_ruleset_file(ruleset_file_name);
	for (auto _r : _rules) {
		auto r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
		cls.insert(*r, _r.second);
	}
}

void check_matches_rule(OvsWrap & cls, packet_t & _p, struct cls_rule* rule) {
	struct flow p = OvsWrap::flow_from_packet(_p);
	auto res = cls.search(p);
	BOOST_CHECK_EQUAL(res, rule);

	// auto r = cls.flow_to_Rule_Ipv4_ACL(p);
	// auto r_i = distance(cls.ovs_rules.begin(),
	// 		std::find(cls.ovs_rules.begin(), cls.ovs_rules.end(), res)
	// );
	// std::cout << r_i << "> " << r << std::endl;
}

size_t index_of_name(const std::string &elm) {
	auto & arr = struct_flow_packet_names;
	auto it = std::find(arr.begin(), arr.end(), elm);
	assert(it != arr.end());
	return distance(arr.begin(), it);
}

void rule_formater(std::ofstream & str, const Classifier::rule_spec_t & _r) {
	//auto _r = OvsWrap::flow_to_Rule_Ipv4_ACL(r);
	Rule_Ipv4_ACL r;
	auto a = &_r.first[0];

	//size_t i = index_of_name("nw_src-2");
	//vec_build::pop_32be(r.sip, i, a);
    //
	//i = index_of_name("nw_dst-2");
	//vec_build::pop_32be(r.dip, i, a);
    //
	//i = index_of_name("tp_src");
	//vec_build::pop_16be(r.sport, i, a);
    //
	//i = index_of_name("tp_dst");
	//vec_build::pop_16be(r.dport, i , a);
    //
	//i = index_of_name("nw_proto");
	//r.proto = a[i];
	str << r;
}

void dump_trees(const OvsWrap & _cls) {
	auto & cls = static_cast<const classifier_priv*>(_cls.cls.priv)->cls;
	size_t tree_i = 0;
	for (auto & t : cls.trees) {
		if (t->rules.size()) {
			{
				std::ofstream of;
				of.exceptions(std::ofstream::failbit | std::ofstream::badbit);
				of.open(string("dump/tree_") + to_string(tree_i) + ".dot");
				of << t->tree;
				of.close();
			}
			{
				ofstream of(string("dump/tree_") + to_string(tree_i) + ".txt",
						ofstream::out);
				std::vector<Classifier::rule_spec_t> tmp;
				BTree::ToRules tc(t->tree, tmp);
				tc.to_rules();
				for (auto d: t->tree.dimension_order)
					of << unsigned(d)  << ": " << struct_flow_packet_names[d] << " ";
				of << endl;
				for (const auto & r : tmp) {
					rule_formater(of, r);
					of << std::endl;
				}
				of.close();
			}
		}
		tree_i++;
	}
}

BOOST_AUTO_TEST_CASE( basic_simple1_3 ) {
	// @0.0.0.1/32	0.0.0.0/32	0 : 65535	0 : 65535	0x00/0xFF
	// @0.0.1.0/32	0.0.0.0/32	0 : 65535	0 : 65535	0x00/0xFF
	// @0.1.0.0/32	0.0.0.0/32	0 : 65535	0 : 65535	0x00/0xFF
	OvsWrap cls;
	init_cls_from_file(cls, "tests/data/simple1_3");
	std::vector<packet_t> _packets = {
		{ 0x0000, 0x0100, 0, 0, 0, 0, 0 },
		{ 0x0000, 0x0001, 0, 0, 0, 0, 0 },
		{ 0x0100, 0x0000, 0, 0, 0, 0, 0 },
	};
	for (size_t i = 0; i < _packets.size(); i++) {
		check_matches_rule(cls, _packets[i], cls.ovs_rules[i]);
	}
	{
		packet_t p_invalid = { 0x0001, 0x0000, 0, 0, 0, 0, 0 };
		check_matches_rule(cls, p_invalid, nullptr);
	}
	dump_trees(cls);
}

BOOST_AUTO_TEST_CASE( basic_simple2_3 ) {
	// @0.0.0.1/32	0.0.0.0/32	0 : 65535	0 : 65535	0x00/0xFF
	// @0.0.1.0/24	0.0.0.0/32	0 : 65535	0 : 65535	0x00/0xFF
	// @0.1.0.0/16	0.0.0.0/32	0 : 65535	0 : 65535	0x00/0xFF
	OvsWrap cls;
	init_cls_from_file(cls, "tests/data/simple2_3");
	std::vector<std::pair<packet_t, size_t>> _packets = {
		{{ 0x0000, 0x0100, 0, 0, 0, 0, 0 }, 0},
		{{ 0x0000, 0x0100, 0, 0, 0, 12, 0 }, 0},
		{{ 0x0000, 0x0100, 0, 0, 12, 0, 0 }, 0},

		{{ 0x0000, 0x0001, 0, 0, 0, 0, 0 }, 1},
		{{ 0x0000, 0x0101, 0, 0, 0, 0, 0 }, 1},
		{{ 0x0000, 0x0901, 0, 0, 12, 0, 0 }, 1},

		{{ 0x0100, 0x0000, 0, 0, 0, 0, 0 }, 2},
		{{ 0x0100, 0x1234, 0, 0, 0, 0, 0 }, 2},
	};
	for (auto & p: _packets) {
		check_matches_rule(cls, p.first, cls.ovs_rules[p.second]);
	}
	//dump_trees(cls);
}

BOOST_AUTO_TEST_SUITE_END()
