#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partiton_sort/b_tree.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv__testsuite )

BOOST_AUTO_TEST_CASE( simple_search ) {
	BTree t;
	using K = BTree::KeyInfo;

	t.root = new BTree::Node;
	K k( { 4, 6 }, 9, BTree::INVALID_INDEX);

	t.root->set_key<uint32_t>(0, k);
	t.root->set_key_cnt(1);
	auto nl = new BTree::Node;
	nl->set_key_cnt(1);
	t.root->set_next_layer(0, nl);

	K k2( { 10, 20 }, 10, BTree::INVALID_INDEX);
	nl->set_key<BTree::value_t>(0, k2);

	using V = vector<BTree::value_t>;
	{
		V v = { 0, 0 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r, BTree::INVALID_RULE);
	}
	{
		V v = { 4, 0 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r, 9);
	}
	{
		V v = { 4, 10 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r, 10);
	}
	{
		V v = { 4, 11 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r, 10);
	}
	{
		V v = { 4, 20 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r, 10);
	}
	{
		V v = { 4, 30 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r, 9);
	}
	{
		V v = { 3, 10 };
		auto r = t.search(v);
		BOOST_CHECK_EQUAL(r, BTree::INVALID_RULE);
	}
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
