
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/b_tree_insert.h>
#include <pcv/partition_sort/b_tree_collision_check.h>

using namespace pcv;
using namespace std;


BOOST_AUTO_TEST_SUITE( pcv_testsuite )

using BTree = _BTree<uint16_t, 2>;
void insert(BTree & t,  BTree::rule_spec_t & r) {
	BTreeInsert<BTree>::insert(t, r);
}

bool does_rule_colide(BTree & t, const BTree::rule_spec_t & r) {
	return BTreeCollisionCheck<BTree>::does_rule_colide(t, r);
}

void simple_colision_check(size_t N) {
	size_t STEP = 10;
	size_t SIZE = 4;
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::val_range_t;

	for (size_t i = 0; i < N; i++) {
		rule_t r0 = { { R1d(i * STEP, i * STEP + SIZE - 1), R1d(0, 0), }, i };
		rule_t r1 = { { R1d(i * STEP, i * STEP + SIZE), R1d(0, 0), }, i + 1 };

		bool collide = does_rule_colide(t, r0);
		BOOST_CHECK(not collide);

		collide = does_rule_colide(t, r1);
		BOOST_CHECK(not collide);

		insert(t, r0);

		collide = does_rule_colide(t, r1);
		BOOST_CHECK(collide);

		collide = does_rule_colide(t, r0);
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
