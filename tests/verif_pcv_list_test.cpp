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

void run_verification(const std::string & rule_file, size_t UNIQUE_TRACE_CNT,
		size_t LOOKUP_CNT) {
	using BTree = BTreeImp<uint16_t, 7, 8>;
	using Classifier0 = PartitionSortClassifer<BTree, 64, 10>;
	using Classifier1 = ListBasedClassifier<uint16_t, 7>;
	Classifier0 cls0(rule_vec_format::Rule_Ipv4_ACL_formaters, rule_vec_format::Rule_Ipv4_ACL_names);
	Classifier1 cls1;

	// load rules from the file
	auto _rules = parse_ruleset_file(rule_file);
	vector<const Rule_Ipv4_ACL*> rules;
	{
		// load rules in to a classifier tree
		for (auto _r : _rules) {
			auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
			BTree::rule_spec_t r =
					{ rule_to_array<uint16_t, 7>(*__r), _r.second };
			cls0.insert(r);
			cls1.insert(r);
			rules.push_back(__r);
		}
	}
	cls1.prepare();

	{
		size_t tree_i = 0;
		for (auto & t : cls0.trees) {
			if (t->rules.size()) {
				ofstream of(string("dump/tree_") + to_string(tree_i) + ".dot",
						ofstream::out);
				of << t->tree;
				of.close();
			}
			tree_i++;
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
