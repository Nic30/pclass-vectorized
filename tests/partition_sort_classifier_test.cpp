#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"
#include <vector>
#include <array>

#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/rule_value_int.h>

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>

using namespace pcv;
using namespace std;
using namespace pcv::rule_conv_fn;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

BOOST_AUTO_TEST_CASE( simple_non_overlapping ) {
	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 2>>;
	using Classifier = PartitionSortClassifer<BTree, 32>;
	using R1d = typename BTree::key_range_t;
	using rule_spec_t = typename BTree::rule_spec_t;
	using key_vec_t = typename BTree::key_vec_t;
	R1d any(0, numeric_limits<uint16_t>::max());
	Classifier::NodeAllocator mem(1024);
	Classifier cls(mem);
	vector<rule_spec_t> rules;
	// all this should fit in to a single tree where is only 1st dim used
	for (BTree::rule_id_t i = 0; i < 5; i++) {
		rule_spec_t r = { { any, R1d(i, i) }, { 0, i } };
		rules.push_back(r);
		cls.insert(r);

		key_vec_t v( { 0, (uint16_t) i });
		auto res = cls.search(v);
		BOOST_CHECK_EQUAL(res.rule_id, i);
	}
	BOOST_CHECK_EQUAL(cls.tree_cnt, 1);
}

BOOST_AUTO_TEST_CASE( ruleset_acl1_100 ) {
	string rule_file = "tests/data/acl1_100";
	constexpr size_t D = 7;
	constexpr size_t UNIQUE_TRACE_CNT = 128;
	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, D>>;
	using Classifier = PartitionSortClassifer<BTree, 32>;
	Classifier::NodeAllocator mem(1024);
	Classifier cls(mem);

	vector<iParsedRule*> _rules;
	RuleReader rp;
	_rules = rp.parse_rules(rule_file);
	{
		// load rules in to a classifier tree
		BTree::rule_id_t i = 0;
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r);
			BTree::rule_spec_t r = { rule_to_array_16b(*__r), {
					(BTree::priority_t) __r->cummulative_prefix_len(), i } };
			cls.insert(r);
		}
	}
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&_rules),
			UNIQUE_TRACE_CNT);
	for (auto p : packets) {
		auto res = cls.search(p);
		BOOST_CHECK_NE(res.rule_id, BTree::INVALID_RULE);
	}
	// size_t i = 0;
	// for (auto & t: cls.trees) {
	// 	if (t->rules.size() == 0)
	// 		continue;
	// 	assert(t->used_dim_cnt <= 7);
	// 	auto fname = string("pcv_") + to_string(i) + string(".dot");
	// 	ofstream of(fname);
	// 	of << t->tree;
	// 	of.close();
	// 	i++;
	// }
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
