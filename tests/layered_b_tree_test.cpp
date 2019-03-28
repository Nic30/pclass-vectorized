#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <limits>
#include <pcv/partiton_sort/b_tree.h>
#include <pcv/partiton_sort/b_tree_search.h>
#include <pcv/partiton_sort/b_tree_insert.h>
#include <pcv/partiton_sort/b_tree_remove.h>

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

	R r3 = { { R1d(1, 1), R1d(2, 2), R1d(3, 3), R1d(4, 4) }, 3 };
	insert(t, r3);
	res = search(t, v1);
	BOOST_CHECK_EQUAL(res, 3);

}
//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
