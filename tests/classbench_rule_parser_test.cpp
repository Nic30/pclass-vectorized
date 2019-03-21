#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/rule_parser/classbench_rule_parser.h>

using namespace pcv;
using namespace std;

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
	for (auto r: rules) {
		string ref_line;
		getline(ref, ref_line);
		stringstream ss;
		ss << *reinterpret_cast<Rule_Ipv4*>(r);
		BOOST_CHECK_EQUAL(ref_line, ss.str());
	}
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()