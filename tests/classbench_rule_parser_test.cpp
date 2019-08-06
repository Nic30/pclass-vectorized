#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include <string>
#include "test_common.h"

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/partition_sort/b_tree_impl.h>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

BOOST_AUTO_TEST_SUITE (pcv_testsuite)

size_t count_lines(const string & file_name) {
	ifstream input(file_name);
	size_t lines = 0;
	for (std::string line; std::getline(input, line);) {
		lines++;
	}
	return lines;
}

template<typename RULE_T>
void test_file_parsing(const string & file_name, size_t header_size = 0) {
	vector<iParsedRule*> rules;
	{
		RuleReader rp;
		rules = rp.parse_rules(file_name);
	}

	auto expected_size = count_lines(file_name) - header_size;
	BOOST_CHECK_EQUAL(rules.size(), expected_size);

	ifstream ref(file_name);
	// skip header
	for (size_t i = 0; i < header_size; i++) {
		string ref_line;
		getline(ref, ref_line);
	}
	// check if the rule was parsed correctly by converting it back to string
	for (auto r : rules) {
		string ref_line;
		getline(ref, ref_line);
		stringstream ss;
		auto _r = dynamic_cast<RULE_T*>(r);
		BOOST_ASSERT(_r);
		ss << *_r;
		BOOST_CHECK_EQUAL(ref_line, ss.str());
	}
}

BOOST_AUTO_TEST_CASE( parse_acl1_100 ) {
	test_file_parsing<Rule_Ipv4_ACL>("tests/data/acl1_100");
}

BOOST_AUTO_TEST_CASE( parse_acl1_500 ) {
	test_file_parsing<Rule_Ipv4_ACL>("tests/data/acl1_500");
}

BOOST_AUTO_TEST_CASE( parse_openflow_1 ) {
	test_file_parsing<Rule_OF_1_5_1>("tests/data/openflow_1", 1);
}

template<typename BTree>
void test_b_tree(const std::string & file_name) {
	BTree t;

	vector<iParsedRule*> _rules;
	{
		RuleReader rp;
		_rules = rp.parse_rules(file_name);
	}
	typename BTree::rule_id_t i = 0;
	for (auto _r : _rules) {
		auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r);
		typename BTree::rule_spec_t r = { rule_to_array_16b(*__r), {0, i} };
		if (not t.does_rule_colide(r)) {
			//std::cout << i << " " << *__r << std::endl;
			t.insert(r);
			i++;

			//stringstream ss;
			//ss << "b_tree_" << i << ".dot";
			//ofstream o(ss.str());
			//o << t;
			//o.close();
		}
	}
	//stringstream ss;
	//ss << "b_tree_acl1_100_all" ".dot";
	//ofstream o(ss.str());
	//o << t;
	//o.close();
}

using NormalBTree = BTreeImp<_BTreeCfg<uint16_t, IntRuleValue, 7, 65000, 8, false>>;
using CompressedBTree = BTreeImp<_BTreeCfg<uint16_t, IntRuleValue, 7, 65000, 8, true>>;
BOOST_AUTO_TEST_CASE( classifier_from_classbench_acl1_100 ) {
	auto file_name = "tests/data/acl1_100";
	test_b_tree<NormalBTree>(file_name);
}
BOOST_AUTO_TEST_CASE( classifier_from_classbench_acl1_500 ) {
	auto file_name = "tests/data/acl1_500";
	test_b_tree<NormalBTree>(file_name);
}

BOOST_AUTO_TEST_CASE( classifier_from_classbench_comp_en_acl1_100 ) {
	auto file_name = "tests/data/acl1_100";
	test_b_tree<CompressedBTree>(file_name);
}
BOOST_AUTO_TEST_CASE( classifier_from_classbench_comp_en_acl1_500 ) {
	auto file_name = "tests/data/acl1_500";
	test_b_tree<CompressedBTree>(file_name);
}
//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
