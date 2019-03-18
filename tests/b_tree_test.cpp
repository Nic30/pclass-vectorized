
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include <boost/test/unit_test.hpp>
#include <functional>
#include <iostream>
#include <fstream>
#include <stdint.h>

#include <pcv/partiton_sort/b_tree.h>

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

BOOST_AUTO_TEST_CASE( simple_insert ) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::Range1d<uint32_t>;
	rule_t r1 = {{R1d(1, 1),}, 1};
	rule_t r2 = {{R1d(3, 6),}, 2};
	rule_t r3 = {{R1d(7, 10),}, 3};

	t.insert(r1);
	t.insert(r2);
	t.insert(r3);

	BOOST_CHECK_EQUAL(t.search(0), BTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(t.search(1), 1);
	BOOST_CHECK_EQUAL(t.search(2), BTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(t.search(3), 2);
	BOOST_CHECK_EQUAL(t.search(6), 2);
	BOOST_CHECK_EQUAL(t.search(7), 3);
	BOOST_CHECK_EQUAL(t.search(10), 3);
	BOOST_CHECK_EQUAL(t.search(11), BTree::INVALID_RULE);
}


//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
