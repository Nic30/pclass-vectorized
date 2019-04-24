#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <limits>
#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/b_tree_search.h>
#include <pcv/partition_sort/b_tree_insert.h>
#include <pcv/partition_sort/b_tree_remove.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv__testsuite )

template<typename BTree>
typename BTree::rule_id_t search(BTree & t,
		const typename BTree::val_vec_t & v) {
	return BTreeSearch<BTree>::search(t, v);
}

template<typename BTree>
void insert(BTree & t, typename BTree::rule_spec_t & r) {
	BTreeInsert<BTree>::insert(t, r);
}

template<typename BTree>
void remove(BTree & t, const typename BTree::rule_spec_t & r) {
	BTreeRemove<BTree>::remove(t, r);
}

BOOST_AUTO_TEST_CASE( simple_search ) {
	using BTree = _BTree<uint16_t, 2>;
	BTree t;
	using K = BTree::KeyInfo;

	t.root = new BTree::Node;
	K k( { 4, 6 }, 9, BTree::INVALID_INDEX);

	t.root->set_key(0, k);
	t.root->set_key_cnt(1);
	auto nl = new BTree::Node;
	nl->set_key_cnt(1);
	t.root->set_next_layer(0, nl);

	K k2( { 10, 20 }, 10, BTree::INVALID_INDEX);
	nl->set_key(0, k2);

	using V = typename BTree::val_vec_t;
	{
		V v = { 0, 0 };
		auto r = search(t, v);
		BOOST_CHECK_EQUAL(r, BTree::INVALID_RULE);
	}
	{
		V v = { 4, 0 };
		auto r = search(t, v);
		BOOST_CHECK_EQUAL(r, 9);
	}
	{
		V v = { 4, 10 };
		auto r = search(t, v);
		BOOST_CHECK_EQUAL(r, 10);
	}
	{
		V v = { 4, 11 };
		auto r = search(t, v);
		BOOST_CHECK_EQUAL(r, 10);
	}
	{
		V v = { 4, 20 };
		auto r = search(t, v);
		BOOST_CHECK_EQUAL(r, 10);
	}
	{
		V v = { 4, 30 };
		auto r = search(t, v);
		BOOST_CHECK_EQUAL(r, 9);
	}
	{
		V v = { 3, 10 };
		auto r = search(t, v);
		BOOST_CHECK_EQUAL(r, BTree::INVALID_RULE);
	}
}

BOOST_AUTO_TEST_CASE( ins_search_rem_4layer ) {
	using BTree = _BTree<uint16_t, 4>;
	BTree t;

	using V = typename BTree::val_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::val_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);
	{
		R r0 = { { R1d(0, 0), any, any, any }, 0 };
		insert(t, r0);
		V v0 = { 1, 0, 0, 0 };
		auto res = search(t, v0);
		BOOST_CHECK_EQUAL(res, BTree::INVALID_RULE);

		V v1 = { 0, 1, 1, 1 };
		res = search(t, v1);
		BOOST_CHECK_EQUAL(res, 0);

		BOOST_CHECK_EQUAL(t.root->get_next_layer(0), nullptr);
		R r1 = { { R1d(0, 0), R1d(1, 1), any, any }, 1 };
		insert(t, r1);

		res = search(t, v1);
		BOOST_CHECK_EQUAL(res, 1);

		res = search(t, v0);
		BOOST_CHECK_EQUAL(res, BTree::INVALID_INDEX);

		V v2 = { 0, 0, 0, 0 };
		res = search(t, v2);
		BOOST_CHECK_EQUAL(res, 0);
		//{
		//	stringstream ss;
		//	ofstream o("ins_search_rem_4layer_r0_r1.dot");
		//	o << t;
		//	o.close();
		//}
		res = search(t, v1);

		/*
		 * <0>0-0 <- removing this
		 *     |
		 * <1>1-1 <- while keeping this
		 *
		 * */
		remove(t, r0);

		res = search(t, v2);
		BOOST_CHECK_EQUAL(res, BTree::INVALID_INDEX);

		res = search(t, v1);
		BOOST_CHECK_EQUAL(res, 1);
	}

}

BOOST_AUTO_TEST_CASE( rewrite_4layer ) {
	using BTree = _BTree<uint16_t, 4>;
	BTree t;

	using V = typename BTree::val_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::val_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);
	R r0 = { { R1d(0, 0), any, any, any }, 0 };
	V v0 = { 0, 0, 0, 0 };

	insert(t, r0);
	auto res = search(t, v0);
	BOOST_CHECK_EQUAL(res, 0);

	R r1 = { { R1d(0, 0), any, any, any }, 1 };
	insert(t, r1);
	res = search(t, v0);
	BOOST_CHECK_EQUAL(res, 1);

	V v1 = { 1, 2, 3, 4 };

	R r2 = { { R1d(1, 1), R1d(2, 2), R1d(3, 3), R1d(4, 4) }, 2 };
	insert(t, r2);
	res = search(t, v1);
	BOOST_CHECK_EQUAL(res, 2);

	//{
	//	stringstream ss;
	//	ofstream o("bt_r2.dot");
	//	o << t;
	//	o.close();
	//}

	R r3 = { { R1d(1, 1), R1d(2, 2), R1d(3, 3), R1d(4, 4) }, 3 };
	insert(t, r3);
	res = search(t, v1);
	BOOST_CHECK_EQUAL(res, 3);
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

	R r4 = { { R1d(1, 1), R1d(2, 2), R1d(4, 4), R1d(4, 4) }, 3 };
	insert(t, r4);

	//{
	//	stringstream ss;
	//	ofstream o("bt_r4.dot");
	//	o << t;
	//	o.close();
	//}
}

BOOST_AUTO_TEST_CASE( insert_nearly_wildcard ) {
	constexpr size_t D = 7;
	using BTree = _BTree<uint16_t, D, 8, true>;
	BTree t;
	using V = typename BTree::val_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::val_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);

	R r0 = { { any, any, any, any, any, any, R1d(6, 6) }, 0 };
	V v0 = { 0, 1, 2, 3, 4, 5, 6 };
	BTreeInsert<BTree>::InsertCookie c(t, r0);
	BOOST_CHECK_EQUAL(c.requires_levels, D);
	BOOST_CHECK_EQUAL(c.additional_level_required_cnt(), D - 1);
	BOOST_CHECK_EQUAL(c.required_more_levels(), true);
	c.level = D - 2;
	BOOST_CHECK_EQUAL(c.additional_level_required_cnt(), 1);
	BOOST_CHECK_EQUAL(c.required_more_levels(), true);
	c.level = D - 1;
	BOOST_CHECK_EQUAL(c.additional_level_required_cnt(), 0);
	BOOST_CHECK_EQUAL(c.required_more_levels(), false);
	insert(t, r0);
	auto res = search(t, v0);
	BOOST_CHECK_EQUAL(res, 0);

	R r_any = { { any, any, any, any, any, any, any }, 0 };
	BTreeInsert<BTree>::InsertCookie c_any(t, r_any);
	BOOST_CHECK_EQUAL(c_any.requires_levels, 1);
	BOOST_CHECK_EQUAL(c_any.additional_level_required_cnt(), 0);
	BOOST_CHECK_EQUAL(c_any.required_more_levels(), false);

}

BOOST_AUTO_TEST_CASE( rewrite_on_demand ) {
	using BTree = _BTree<uint16_t, 4>;
	BTree t;

	using V = typename BTree::val_vec_t;
	using R = typename BTree::rule_spec_t;
	using R1d = typename BTree::val_range_t;
	auto const U16_MAX = std::numeric_limits<uint16_t>::max();
	R1d any(0, U16_MAX);
	R r0 = { { R1d(0, 0), R1d(1, 1), R1d(2, 2), R1d(3, 3) }, 0 };
	R r1 = { { R1d(0, 0), R1d(4, 4), R1d(2, 2), R1d(3, 3) }, 1 };
	V v0 = { 0, 0, 0, 0 };
	V v1 = { 0, 1, 2, 3 };
	V v2 = { 0, 4, 2, 3 };
	insert(t, r0);
	auto res = search(t, v0);
	BOOST_CHECK_EQUAL(res, BTree::INVALID_RULE);
	res = search(t, v1);
	BOOST_CHECK_EQUAL(res, 0);
	res = search(t, v2);
	BOOST_CHECK_EQUAL(res, BTree::INVALID_RULE);
	//{
	//	stringstream ss;
	//	ofstream o("rewrite_on_demand_0.dot");
	//	o << t;
	//	o.close();
	//}
	// the root node must stay only for dimension[0] otherwise the tree can not grow
	insert(t, r1);
	//{
	//	stringstream ss;
	//	ofstream o("rewrite_on_demand_1.dot");
	//	o << t;
	//	o.close();
	//}

	res = search(t, v0);
	BOOST_CHECK_EQUAL(res, BTree::INVALID_RULE);
	res = search(t, v1);
	BOOST_CHECK_EQUAL(res, 0);
	res = search(t, v2);
	BOOST_CHECK_EQUAL(res, 1);
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
