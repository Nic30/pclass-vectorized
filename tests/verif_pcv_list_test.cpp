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

using BTree = BTreeImp<uint16_t, 7, 8>;
using Classifier0 = PartitionSortClassifer<BTree, 64, 10>;
using Classifier1 = ListBasedClassifier<uint16_t, 7>;
using BTreeToRules = _BTreeToRules<uint16_t, 7, 8>;
using rule_spec_t = typename Classifier0::rule_spec_t;

void formater(std::ostream & str, const Classifier1::rule_spec_t & rule) {
	auto r = rule_from_array(rule.first);
	str << r << ": id=" << rule.second;
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
				std::vector<rule_spec_t> tmp;
				BTreeToRules tc(t->tree, tmp);
				tc.to_rules();
				for (auto r : tmp) {
					formater(of, r);
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
			BTreeToRules tc(t->tree, tmp);
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
				rule_from_array(e_r.first) << ": id=" << e_r.second << " is missing in classifier");
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
			if (! found)
				std::cout << endl;
			BOOST_CHECK_MESSAGE(found,
					rule_from_array(r.first) << ": id=" << r.second << " is invalid in classifier");
		}
	}
	// check there are no duplicities
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
	Classifier1 cls1(formater);

	// load rules from the file
	auto _rules = parse_ruleset_file(rule_file);
	vector<const Rule_Ipv4_ACL*> rules;
	{
		std::mt19937 rand(0);
		// load rules in to a classifier tree
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
			BTree::rule_spec_t r = { rule_to_array_16b(*__r), _r.second };
			cls0.insert(r);
			cls1.insert(r);
			cls1.prepare();

			verify_tree_content(cls0, cls1.rules);

			packet_t p;
			random_corner(*__r, p, 7, rand, false);
			auto r0 = cls0.search(p);
			auto r1 = cls1.search(p);

			BOOST_CHECK_EQUAL(r0, r1);
			BOOST_CHECK_EQUAL(r0, _r.second);
			if (r0 != r1) {
				std::cout << exact_array_to_rule_le(p) << " - searched data"
						<< std::endl;
				std::cout << (*__r) << " id:" << _r.second
						<< " - corresponding rule" << std::endl;
				for (auto r2 : _rules) {
					if (r2.second == r0 or r2.second == r1) {
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
			[&] (Classifier0::rule_id_t r_id, std::ostream & out) {
				std::cerr.flush();
				std::cout.flush();
				if (r_id == Classifier0::INVALID_RULE) {
					out << "<" << r_id <<" NULL>";
					return;
				}
				for (auto r: _rules) {
					if (r.second == r_id) {
						out << "<" << r_id << " " << *reinterpret_cast<Rule_Ipv4_ACL*>(r.first) << ">";
						return;
					}
				}
				out << "<" << r_id << " INVALID>";
			};
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto & p = packets[i % packets.size()];
		auto r0 = cls0.search(p);
		auto r1 = cls1.search(p);
		// pcv : list
		BOOST_CHECK_EQUAL(r0, r1);
		if (r0 != r1) {
			print_find_rule(r0, std::cout);
			std::cout << "  !=  ";
			print_find_rule(r1, std::cout);
			std::cout << std::endl << std::endl;
		}
	}
}

BOOST_AUTO_TEST_CASE( basic ) {
	run_verification("tests/data/acl1_100", 8192, 8 * 8192);
}
//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
