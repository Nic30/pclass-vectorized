
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/partition_sort/b_tree_impl.h>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

BOOST_AUTO_TEST_SUITE (pcv_testsuite)

BOOST_AUTO_TEST_CASE( acl1_100 ) {
	auto file_name = "tests/data/acl1_100";
	vector<iParsedRule*> rules;
	{
		RuleReader rp;
		rules = rp.parse_rules(file_name);
	}
	BOOST_CHECK_EQUAL(rules.size(), 91);

	ifstream ref(file_name);
	for (auto r : rules) {
		string ref_line;
		getline(ref, ref_line);
		stringstream ss;
		ss << *reinterpret_cast<Rule_Ipv4*>(r);
		BOOST_CHECK_EQUAL(ref_line, ss.str());
	}
}

BOOST_AUTO_TEST_CASE( classifier_from_classbench ) {
	using BTree = BTreeImp<uint16_t, 7, 8>;
	auto file_name = "tests/data/acl1_100";

	BTree t;

	vector<iParsedRule*> _rules;
	{
		RuleReader rp;
		_rules = rp.parse_rules(file_name);
	}
	size_t i = 0;
	for (auto _r : _rules) {
		auto __r = reinterpret_cast<Rule_Ipv4*>(_r);
		BTree::rule_spec_t r = { rule_to_array<uint16_t, 7>(*__r), i };
		if (not t.does_rule_colide(r)) {
			//std::cout << i << " " << *__r << std::endl;
			//if (i == 9) {
			//	std::cout << endl;
			//}
			t.insert(r);
			i++;

			//stringstream ss;
			//ss << "b_tree_acl1_100_" << i << ".dot";
			//ofstream o(ss.str());
			//o << t;
			//o.close();
		}
	}
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
