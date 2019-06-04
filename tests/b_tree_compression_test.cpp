#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partition_sort/b_tree_impl.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE (pcv_testsuite)

BOOST_AUTO_TEST_CASE( simple_insert_and_search ) {
	using BTree = BTreeImp<uint16_t, 4, 4>;
	BTree t;

	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::val_range_t;
	{
		rule_t r0 = { { R1d(0, 0), R1d(1, 1), R1d(2, 2), R1d(3, 3) }, 0 };
		t.insert(r0);
		typename BTree::val_vec_t v = { 0, 1, 2, 3 };
		auto s = t.search(v);
		BOOST_CHECK_EQUAL(s, 0);
	}

	for (size_t i = 0; i < 4; i++) {
		auto k = t.root->get_key(i);
		BOOST_CHECK_EQUAL(k.key.low, i);
		BOOST_CHECK_EQUAL(k.next_level, BTree::INVALID_INDEX);
		if (i == 4 - 1) {
			BOOST_CHECK_EQUAL(k.value, 0);
		} else {
			BOOST_CHECK_EQUAL(k.value, BTree::INVALID_RULE);
		}
	}
	//ofstream o("b_tree_simple_insert_and_search_init.dot");
	//o << t;
	//o.close();
	for (size_t i = 1; i < 8; i++) {
		rule_t r = { { R1d(i, i), R1d(1, 1), R1d(2, 2), R1d(3, 3) }, i };
		t.insert(r);
		//ofstream o("b_tree_simple_insert_and_search" + to_string(i) + ".dot");
		//o << t;
		//o.close();
	}

	for (uint16_t i = 0; i < 8; i++) {
		typename BTree::val_vec_t v = { i, 1, 2, 3 };
		auto s = t.search(v);
		BOOST_CHECK_EQUAL(s, i);
	}

}

BOOST_AUTO_TEST_CASE( insert_search_maytimes_any_in_center ) {
	using BTree = BTreeImp<uint16_t, 8, 4>;
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::val_range_t;
	using vv_t = typename BTree::val_vec_t;
	auto INV = BTree::INVALID_RULE;

	R1d any(0, std::numeric_limits<typename R1d::T>::max());
	{
		rule_t r0 = { { R1d(99, 99), any, any, any, any, any, any, R1d(7, 7) },
				0 };
		t.insert(r0);
		//{
		//	ofstream o("insert_search_maytimes_any_in_center_0.dot");
		//	o << t;
		//	o.close();
		//}
		vv_t v = { 99, 1, 2, 3, 4, 5, 6, 7 };
		auto s = t.search(v);
		BOOST_CHECK_EQUAL(s, 0);

		v = {99, 1, 2, 3, 4, 5, 6, 8};
		s = t.search(v);
		BOOST_CHECK_EQUAL(s, INV);

		v = {99, 1, 2, 3, 4, 5, 6, 6};
		s = t.search(v);
		BOOST_CHECK_EQUAL(s, INV);
	}

	BOOST_CHECK_EQUAL(t.root->key_cnt, 7);
	for (size_t i = 0; i < 7; i++) {
		auto k = t.root->get_key(i);
		BOOST_CHECK_EQUAL(k.key.low, i == 0 ? 99 : 0);
		BOOST_CHECK_EQUAL(k.value, INV);
		auto nl = t.root->get_next_layer(i);
		if (i == 7 - 1) {
			BOOST_CHECK_EQUAL(nl->key_cnt, 1);
			auto k = nl->get_key(0);
			BOOST_CHECK_EQUAL(k.value, 0);
			BOOST_CHECK_EQUAL(k.next_level, BTree::INVALID_INDEX);
		} else {
			BOOST_CHECK_EQUAL(nl, nullptr);
		}
	}

}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
