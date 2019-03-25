#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partiton_sort/b_tree.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

void simple_colision_check(size_t N) {
	size_t STEP = 10;
	size_t SIZE = 4;
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = Range1d<uint32_t>;
	for (size_t i = 0; i < N; i++) {
		rule_t r0 = { { R1d(i * STEP, i * STEP + SIZE - 1), R1d(0, 0), }, i };
		rule_t r1 = { { R1d(i * STEP, i * STEP + SIZE), R1d(0, 0), }, i + 1 };

		bool collide = t.does_rule_colide(r0);
		BOOST_CHECK(not collide);

		collide = t.does_rule_colide(r1);
		BOOST_CHECK(not collide);

		t.insert(r0);

		collide = t.does_rule_colide(r1);
		BOOST_CHECK(collide);

		collide = t.does_rule_colide(r0);
		BOOST_CHECK(not collide);
	}
}

BOOST_AUTO_TEST_CASE(colision_1_item_in_1_node) {
	simple_colision_check(1);
}

BOOST_AUTO_TEST_CASE(colision_4_item_in_1_node) {
	simple_colision_check(4);
}

BOOST_AUTO_TEST_CASE(colision_500_nodes) {
	simple_colision_check(500);
}

BOOST_AUTO_TEST_SUITE_END()
