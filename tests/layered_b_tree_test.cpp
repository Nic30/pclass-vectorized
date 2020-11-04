#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <limits>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/rule_value_int.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv__testsuite )


BOOST_AUTO_TEST_CASE( simple_search ) {
	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 2>>;
	BTree t;
	using K = BTree::KeyInfo;

	t.root = new BTree::Node;
	K k( { 4, 6 }, {0, 9}, BTree::INVALID_INDEX);

	t.root->set_key(0, k);
	t.root->set_key_cnt(1);
	auto nl = new BTree::Node;
	nl->set_key_cnt(1);
	t.root->set_next_layer(0, nl);

	K k2( { 10, 20 }, {0, 10}, BTree::INVALID_INDEX);
	nl->set_key(0, k2);

	using V = typename BTree::key_vec_t;
	{
		V v = { 0, 0 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r.rule_id, BTree::INVALID_RULE);
	}
	{
		V v = { 4, 0 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r.rule_id, 9);
	}
	{
		V v = { 4, 10 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r.rule_id, 10);
	}
	{
		V v = { 4, 11 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r.rule_id, 10);
	}
	{
		V v = { 4, 20 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r.rule_id, 10);
	}
	{
		V v = { 4, 30 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r.rule_id, 9);
	}
	{
		V v = { 3, 10 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r.rule_id, BTree::INVALID_RULE);
	}
}

BOOST_AUTO_TEST_CASE( ins_search_rem_4layer ) {
	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 4>>;
	BTree t;

	using V = typename BTree::key_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::key_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);
	{
		R r0 = { { R1d(0, 0), any, any, any }, {0, 0} };
		t.insert(r0);
		V v0 = { 1, 0, 0, 0 };
		auto res = t.search(v0);
		BOOST_CHECK_EQUAL(res.rule_id, BTree::INVALID_RULE);

		V v1 = { 0, 1, 1, 1 };
		res = t.search(v1);
		BOOST_CHECK_EQUAL(res.rule_id, 0);

		BOOST_CHECK_EQUAL(t.root->get_next_layer(0), nullptr);
		R r1 = { { R1d(0, 0), R1d(1, 1), any, any }, {0, 1} };
		t.insert(r1);

		res = t.search(v1);
		BOOST_CHECK_EQUAL(res.rule_id, 1);

		res = t.search(v0);
		BOOST_CHECK_EQUAL(res.rule_id, BTree::INVALID_RULE);

		V v2 = { 0, 0, 0, 0 };
		res = t.search(v2);
		BOOST_CHECK_EQUAL(res.rule_id, 0);
		//{
		//	stringstream ss;
		//	ofstream o("ins_search_rem_4layer_r0_r1.dot");
		//	o << t;
		//	o.close();
		//}
		res = t.search(v1);

		/*
		 * <0>0-0 <- removing this
		 *     |
		 * <1>1-1 <- while keeping this
		 *
		 * */
		t.remove(r0);

		res = t.search(v2);
		BOOST_CHECK_EQUAL(res.rule_id, BTree::INVALID_RULE);

		res = t.search(v1);
		BOOST_CHECK_EQUAL(res.rule_id, 1);
	}
}

BOOST_AUTO_TEST_CASE( rewrite_4layer ) {
	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 4>>;
	BTree t;

	using V = typename BTree::key_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::key_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);
	R r0 = { { R1d(0, 0), any, any, any }, {0, 0} };
	V v0 = { 0, 0, 0, 0 };

	t.insert(r0);
	auto res = t.search(v0);
	BOOST_CHECK_EQUAL(res.rule_id, 0);

	R r1 = { { R1d(0, 0), any, any, any }, {0, 1} };
	t.insert(r1);
	res = t.search(v0);
	BOOST_CHECK_EQUAL(res.rule_id, 1);

	V v1 = { 1, 2, 3, 4 };

	R r2 = { { R1d(1, 1), R1d(2, 2), R1d(3, 3), R1d(4, 4) }, {0, 2} };
	t.insert(r2);
	res = t.search(v1);
	BOOST_CHECK_EQUAL(res.rule_id, 2);

	//{
	//	stringstream ss;
	//	ofstream o("bt_r2.dot");
	//	o << t;
	//	o.close();
	//}

	R r3 = { { R1d(1, 1), R1d(2, 2), R1d(3, 3), R1d(4, 4) }, {0, 3} };
	t.insert(r3);
	res = t.search(v1);
	BOOST_CHECK_EQUAL(res.rule_id, 3);
	//{
	//	stringstream ss;
	//	ofstream o("bt_r3.dot");
	//	o << t;
	//	o.close();
	//}

	BOOST_CHECK_EQUAL(t.root->key_cnt, 2);
	BOOST_CHECK(not t.root->is_compressed);
	BOOST_CHECK(t.root->is_leaf);
	auto l1 = t.root->get_next_layer(1);

	BOOST_CHECK(l1->is_compressed);
	BOOST_CHECK(l1->is_leaf);
	BOOST_CHECK_EQUAL(l1->key_cnt, 3);

	R r4 = { { R1d(1, 1), R1d(2, 2), R1d(4, 4), R1d(4, 4) }, {0, 3} };
	t.insert(r4);

	//{
	//	stringstream ss;
	//	ofstream o("bt_r4.dot");
	//	o << t;
	//	o.close();
	//}
}

BOOST_AUTO_TEST_CASE( insert_nearly_wildcard ) {
	constexpr size_t D = 7;
	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, D, 8, true>>;
	using InsertCookie = typename BTree::Insert_t::InsertCookie;
	BTree t;
	using V = typename BTree::key_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::key_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);

	R r0 = { { any, any, any, any, any, any, R1d(6, 6) }, (BTree::rule_value_t){0, 0} };
	V v0 = { 0, 1, 2, 3, 4, 5, 6 };
	InsertCookie c(t, r0);
	BOOST_CHECK_EQUAL(c.requires_levels, D);
	BOOST_CHECK_EQUAL(c.additional_level_required_cnt(), D - 1);
	BOOST_CHECK_EQUAL(c.required_more_levels(), true);
	c.level = D - 2;
	BOOST_CHECK_EQUAL(c.additional_level_required_cnt(), 1);
	BOOST_CHECK_EQUAL(c.required_more_levels(), true);
	c.level = D - 1;
	BOOST_CHECK_EQUAL(c.additional_level_required_cnt(), 0);
	BOOST_CHECK_EQUAL(c.required_more_levels(), false);
	t.insert(r0);
	auto res = t.search(v0);
	BOOST_CHECK_EQUAL(res.rule_id, 0);

	R r_any = { { any, any, any, any, any, any, any }, (BTree::rule_value_t){0, 0} };
	InsertCookie c_any(t, r_any);
	BOOST_CHECK_EQUAL(c_any.requires_levels, 1);
	BOOST_CHECK_EQUAL(c_any.additional_level_required_cnt(), 0);
	BOOST_CHECK_EQUAL(c_any.required_more_levels(), false);

}

BOOST_AUTO_TEST_CASE( rewrite_on_demand ) {
	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 4>>;
	BTree t;

	using V = typename BTree::key_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::key_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);
	R r0 = { { R1d(0, 0), R1d(1, 1), R1d(2, 2), R1d(3, 3) }, {0, 0} };
	R r1 = { { R1d(0, 0), R1d(4, 4), R1d(2, 2), R1d(3, 3) }, {0, 1} };
	V v0 = { 0, 0, 0, 0 };
	V v1 = { 0, 1, 2, 3 };
	V v2 = { 0, 4, 2, 3 };
	t.insert(r0);
	auto res = t.search(v0);
	BOOST_CHECK_EQUAL(res.rule_id, BTree::INVALID_RULE);
	res = t.search(v1);
	BOOST_CHECK_EQUAL(res.rule_id, 0);
	res = t.search(v2);
	BOOST_CHECK_EQUAL(res.rule_id, BTree::INVALID_RULE);
	//{
	//	stringstream ss;
	//	ofstream o("rewrite_on_demand_0.dot");
	//	o << t;
	//	o.close();
	//}
	// the root node must stay only for dimension[0] otherwise the tree can not grow
	t.insert(r1);
	//{
	//	stringstream ss;
	//	ofstream o("rewrite_on_demand_1.dot");
	//	o << t;
	//	o.close();
	//}

	res = t.search(v0);
	BOOST_CHECK_EQUAL(res.rule_id, BTree::INVALID_RULE);
	res = t.search(v1);
	BOOST_CHECK_EQUAL(res.rule_id, 0);
	res = t.search(v2);
	BOOST_CHECK_EQUAL(res.rule_id, 1);
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
