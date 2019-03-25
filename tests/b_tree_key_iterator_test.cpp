#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partiton_sort/b_tree.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

BOOST_AUTO_TEST_CASE( iterate_100_ranges ) {
	size_t N = 100;
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = Range1d<uint32_t>;
	for (size_t i = 0; i < N; i++) {
		rule_t r = { { R1d(i, i), R1d(0, 0), }, i };
		t.insert(r);
	}

	size_t i = 0;
	for (auto k: t.root->iter_keys()) {
		BOOST_CHECK_EQUAL(k.key.low, i);
		BOOST_CHECK_EQUAL(k.key.high, i);
		i++;
	}

}

BOOST_AUTO_TEST_SUITE_END()
