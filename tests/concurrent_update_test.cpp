#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"
#include <vector>
#include <random>

#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <boost/thread.hpp>

using namespace pcv;
using namespace pcv::rule_conv_fn;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

constexpr size_t D = 7;
using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, D>>;
using Classifier = PartitionSortClassifer<BTree, 32>;

/*
 * Select random rule and remove it or add it to classifier
 * */
void updater_entrypoint(Classifier & cls, vector<BTree::rule_spec_t> & rules,
		size_t reapeat_cnt, size_t seed = 0) {
	mt19937_64 rand(seed);
	std::uniform_int_distribution<int> distr_insert(0, rules.size() - 1);
	std::uniform_int_distribution<int> distr_insert_or_rem_decision(0, 1);

	for (size_t i = 0; i < reapeat_cnt; i++) {
		if (distr_insert_or_rem_decision()) {
			int insert_i = distr_insert();
			int _insert_i = insert_i;
			// find the first rule which is not already in classifier
			auto r = &rules[_insert_i];
			while (cls.rule_to_tree.find(*r) != cls.rule_to_tree.end()) {
				_insert_i = (_insert_i + 1) % rules.size();
				if (_insert_i == insert_i) {
					r = nullptr;
					break;
				}
				r = &rules[_insert_i];
			}
			if (r)
				cls.insert(*r);
		} else if (cls.rule_to_tree.size() == 0) {
			std::uniform_int_distribution<int> distr_rem(0,
					cls.rule_to_tree.size() - 1);
			auto to_rem = cls.rule_to_tree.begin() + distr_rem();
			cls.remove(to_rem->first)
		}
	}
}

/*
 *
 **/
void searcher_entrypoint(const Classifier & cls,
		const std::vector<packet_t> & packets, size_t reapeat_cnt) {
	for (size_t i = 0; i < reapeat_cnt; i++) {
		auto & p = packets[i % packets.size()];
		cls.search(p);
	}
}

/*
 * @note that this test actually does not check correctness
 * as it has no synchronisation between the lookup threads and update threads
 * However it check if the update of did not caused fault in lookup threads
 * */
BOOST_AUTO_TEST_CASE( simple ) {
	string rule_file = "tests/data/acl1_100";

	constexpr size_t UNIQUE_TRACE_CNT = 128;
	constexpr size_t LOOKUP_THREAD_CNT = 4;
	constexpr size_t UPDATE_THREAD_CNT = 1;
	constexpr size_t LOOKUPS_PER_THR = 1000000;

	Classifier cls;

	vector<iParsedRule*> _rules;
	vector<BTree::rule_spec_t> rules;

	RuleReader rp;
	_rules = rp.parse_rules(rule_file);
	{
		// load rules in to a classifier tree
		BTree::rule_id_t i = 0;
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r);
			BTree::rule_spec_t r = { rule_to_array_16b(*__r), {0, i} };
			rules.push_back(r);
		}
	}
	auto packets = generate_packets_from_ruleset(
			*reinterpret_cast<vector<const Rule_Ipv4_ACL*>*>(&_rules),
			UNIQUE_TRACE_CNT);

	std::array<boost::thread *, UPDATE_THREAD_CNT> update_thr;
	std::array<boost::thread *, LOOKUP_THREAD_CNT> lookup_thr;
	for (size_t i = 0; i < LOOKUP_THREAD_CNT; i++) {
		update_thr[i] = new boost::thread(updater_entrypoint, cls, rules);
	}
	for (size_t i = 0; i < LOOKUP_THREAD_CNT; i++) {
		update_thr[i] = new boost::thread(searcher_entrypoint, cls, packets,
				LOOKUPS_PER_THR);
	}
	for (auto t : update_thr)
		t->join();
	for (auto t : lookup_thr)
		t->join();
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
