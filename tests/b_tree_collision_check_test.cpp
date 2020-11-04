#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/rule_value_int.h>


using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 2>>;

void simple_collision_check(size_t N) {
	size_t STEP = 10;
	size_t SIZE = 4;
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::key_range_t;

	for (uint32_t i = 0; i < N; i++) {
		rule_t r0 = { { R1d(i * STEP, i * STEP + SIZE - 1), R1d(0, 0), },
				{ 0, i } };
		rule_t r1 = { { R1d(i * STEP, i * STEP + SIZE), R1d(0, 0), },
				{ 0, i + 1 } };

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

BOOST_AUTO_TEST_CASE( collision_check_wildcard ) {

	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::key_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);
	R r0 = { { any, any }, { 0, 0 } };
	R r1 = { { R1d(0, 0), R1d(4, 4), }, { 0, 1 } };
	{
		BTree t;
		t.insert(r0);
		bool collide = t.does_rule_colide(r1);
		BOOST_CHECK(collide);
	}
	R r3 = { { any, R1d(4, 4) }, { 0, 0 } };
	{
		BTree t2;
		t2.insert(r3);
		bool collide = t2.does_rule_colide(r1);
		BOOST_CHECK(collide);

		collide = t2.does_rule_colide(r0);
		BOOST_CHECK(collide);
	}
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
