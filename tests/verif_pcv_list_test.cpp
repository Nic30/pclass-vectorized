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
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/utils/benchmark_common.h>
#include "../benchmarks/list/list_classifier.h"

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv__testsuite )

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

using BTree = BTreeImp<_BTreeCfg<uint16_t, IntRuleValue, 7, 5000, 8>>;
using Classifier0 = PartitionSortClassifer<BTree, 64, 10>;
using rule_spec_t = typename Classifier0::rule_spec_t;
using Classifier1 = ListBasedClassifier<_BTreeCfg<uint16_t, IntRuleValue, 7>>;

template<class CLS_T>
void formater(std::ostream & str, const typename CLS_T::rule_spec_t & rule) {
	auto r = rule_from_array(rule.first);
	str << r << ": p=" << (rule.second.priority) << " id=" << (rule.second.rule_id);
}

void dump_trees(std::ostream & str, const Classifier0 & cls) {
	size_t tree_i = 0;
	for (auto & t : cls.trees) {
		if (t->rules.size()) {
			{
				ofstream of(string("dump/tree_") + to_string(tree_i) + ".dot",
						ofstream::out);
				of << t->tree;
				of.close();
			}
			{
				ofstream of(string("dump/tree_") + to_string(tree_i) + ".txt",
						ofstream::out);
				std::vector<Classifier0::rule_spec_t> tmp;
				BTree::ToRules tc(t->tree, tmp);
				tc.to_rules();
				for (auto d: t->tree.dimension_order)
					of << unsigned(d) << " ";
				of << endl;
				for (const auto & r : tmp) {
					formater<Classifier0>(of, r);
					of << std::endl;
				}
				of.close();
			}
		}
		tree_i++;
	}
}

void verify_tree_content(const Classifier0 & cls,
		const vector<rule_spec_t> & expected_rules) {
	vector<vector<rule_spec_t>> rules_in_tree;
	// collect rules from trees in classifier
	bool must_be_empty = false;
	for (auto & t : cls.trees) {
		if (t->rules.size()) {
			assert(!must_be_empty);
			std::vector<rule_spec_t> tmp;
			BTree::ToRules tc(t->tree, tmp);
			tc.to_rules();
			rules_in_tree.push_back(tmp);
		} else {
			must_be_empty = true;
		}
	}
	// check if everything is in classifier
	for (auto e_r : expected_rules) {
		bool found = false;
		for (auto & t_rules : rules_in_tree) {
			for (auto r : t_rules) {
				if (r == e_r) {
					found = true;
					break;
				}
			}
			if (found)
				break;
		}
		BOOST_CHECK_MESSAGE(found,
				rule_from_array(e_r.first) << ": id=" << e_r.second.rule_id << " is missing in classifier");
	}
	// check if everything in classifier is valid
	for (auto & t_rules : rules_in_tree) {
		for (auto r : t_rules) {
			bool found = false;
			for (auto e_r : expected_rules) {
				if (r == e_r) {
					found = true;
					break;
				}
			}
			if (!found)
				std::cout << endl;
			BOOST_CHECK_MESSAGE(found,
					rule_from_array(r.first) << ": id=" << r.second.rule_id << " is invalid in classifier");
		}
	}
	// check there are no duplicates
	size_t rule_in_cls_cnt = 0;
	for (auto & t_rules : rules_in_tree) {
		rule_in_cls_cnt += t_rules.size();
	}
	BOOST_CHECK_EQUAL(rule_in_cls_cnt, expected_rules.size());

	dump_trees(std::cout, cls);
}

void run_verification(const std::string & rule_file, size_t UNIQUE_TRACE_CNT,
		size_t LOOKUP_CNT) {
	Classifier0 cls0(rule_vec_format::Rule_Ipv4_ACL_formaters,
			rule_vec_format::Rule_Ipv4_ACL_names);
	Classifier1 cls1(formater<Classifier1>);

	// load rules from the file
	auto _rules = parse_ruleset_file(rule_file);
	vector<Classifier0::rule_spec_t> rules_for_check;
	vector<const Rule_Ipv4_ACL*> rules;
	{
		std::mt19937 rand(0);
		// load rules in to a classifier tree
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
			Classifier0::rule_spec_t r = { rule_to_array_16b(*__r), {
					(uint32_t)__r->cummulative_prefix_len(), (uint32_t)_r.second } };
			cls0.insert(r);
			cls1.insert({r.first, {r.second.priority, r.second.rule_id}});
			cls1.prepare();
			rules_for_check.push_back(r);
			verify_tree_content(cls0, rules_for_check);

			packet_t p;
			random_corner(*__r, p, 7, rand, false);
			auto r0 = cls0.search(p);
			auto _r1 = cls1.search(p);
			Classifier0::rule_value_t r1(_r1.priority, _r1.rule_id);

			BOOST_CHECK_EQUAL(r0.rule_id, r1.rule_id);
			BOOST_CHECK_EQUAL(r0.rule_id, _r.second);
			if (r0 != r1) {
				std::cout << exact_array_to_rule_le(p) << " - searched data"
						<< std::endl;
				std::cout << (*__r) << " id:" << _r.second
						<< " - corresponding rule" << std::endl;
				for (auto r2 : _rules) {
					if (r2.second == r0.rule_id or r2.second == r1.rule_id) {
						auto r3 = reinterpret_cast<Rule_Ipv4_ACL*>(r2.first);
						std::cout << *r3 << " id:" << r2.second << std::endl;
					}
				}

				std::cout << cls1;
				dump_trees(std::cout, cls0);
				exit(1);
			}

			rules.push_back(__r);
		}
	}

	// generate packets
	auto packets = generate_packets_from_ruleset(rules, UNIQUE_TRACE_CNT);
	auto print_find_rule =
			[&] (Classifier0::rule_value_t v, std::ostream & out) {
				std::cerr.flush();
				std::cout.flush();
				if (!v.is_valid()) {
					out << "<" << v.rule_id <<" NULL>";
					return;
				}
				for (auto r: _rules) {
					if (r.second == v.rule_id) {
						out << "<" << v.rule_id << " " << *reinterpret_cast<Rule_Ipv4_ACL*>(r.first) << ">";
						return;
					}
				}
				out << "<" << v.rule_id << " INVALID>";
			};
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
		if (i == 2)
			cout << endl;
		auto r0 = cls0.search(p);
		auto _r1 = cls1.search(p);
		Classifier0::rule_value_t r1(_r1.priority, _r1.rule_id);

		// pcv : list
		BOOST_CHECK_EQUAL(r0.rule_id, r1.rule_id);
		if (r0 != r1) {
			Rule_Ipv4_ACL tmp;
			cout << "searching: " << exact_array_to_rule_le(p) << std::endl;
			cout << "(tree) ";
			print_find_rule(r0, std::cout);
			std::cout << "  !=  ";
			cout << "(cam) ";
			print_find_rule(r1, std::cout);
			std::cout << std::endl << std::endl;

			dump_trees(std::cout, cls0);
			exit(1);
		}
	}
}

BOOST_AUTO_TEST_CASE( basic_acl1_100 ) {
	run_verification("tests/data/acl1_100", 8192, 8 * 8192);
}
BOOST_AUTO_TEST_CASE( basic_simple1_3 ) {
	run_verification("tests/data/simple1_3", 128, 4096);
}
BOOST_AUTO_TEST_CASE( basic_simple2_3 ) {
	run_verification("tests/data/simple2_3", 128, 4096);
}
// [TODO] now as the id is not part of the priority some rules which has same priority
//        are swapped between list classifier as the priority of rule is same
// BOOST_AUTO_TEST_CASE( basic_acl1_500 ) {
//
//    run_verification("tests/data/acl1_500", 8192, 8 * 8192);
// }
//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
