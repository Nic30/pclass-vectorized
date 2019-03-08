#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include <boost/test/unit_test.hpp>
#include <functional>
#include <iostream>
#include <fstream>
#include <stdint.h>

#include "../src/partition_sort/bplus_tree.h"

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

BOOST_AUTO_TEST_CASE( simple_insert ) {
	BPlusTree t;
	using rule_t = BPlusTree::rule_spec_t;
	rule_t r1 = {{1, 1}, 1};
	rule_t r2 = {{3, 6}, 2};
	rule_t r3 = {{7, 10}, 3};

	t.add(r1);
	t.add(r2);
	t.add(r3);

	BOOST_CHECK_EQUAL(t.search(0), BPlusTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(t.search(1), 1);
	BOOST_CHECK_EQUAL(t.search(2), BPlusTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(t.search(3), 2);
	BOOST_CHECK_EQUAL(t.search(6), 2);
	BOOST_CHECK_EQUAL(t.search(7), 3);
	BOOST_CHECK_EQUAL(t.search(10), 3);
	BOOST_CHECK_EQUAL(t.search(11), BPlusTree::INVALID_RULE);
}


//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
