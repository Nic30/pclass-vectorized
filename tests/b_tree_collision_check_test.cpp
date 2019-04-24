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
void insert(BTree & t, BTree::rule_spec_t & r) {
	BTreeInsert<BTree>::insert(t, r);
}

bool does_rule_colide(BTree & t, const BTree::rule_spec_t & r) {
	return BTreeCollisionCheck<BTree>::does_rule_colide(t, r);
}

void simple_collision_check(size_t N) {
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

BOOST_AUTO_TEST_CASE( collision_check_wildcard ) {
	BTree t;

	using V = typename BTree::val_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::val_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);
	R r0 = { { any, any }, 0 };
	R r1 = { { R1d(0, 0), R1d(4, 4), }, 1 };
	insert(t, r0);
	bool collide = does_rule_colide(t, r1);
	BOOST_CHECK(collide);
}

BOOST_AUTO_TEST_CASE(collision_1_item_in_1_node) {
	simple_collision_check(1);
}

BOOST_AUTO_TEST_CASE(collision_4_item_in_1_node) {
	simple_collision_check(4);
}

BOOST_AUTO_TEST_CASE(collision_500_nodes) {
	simple_collision_check(500);
}

BOOST_AUTO_TEST_SUITE_END()
