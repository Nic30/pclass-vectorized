#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partiton_sort/b_tree.h>
using namespace pcv;

BOOST_AUTO_TEST_SUITE( pcv__testsuite )

BOOST_AUTO_TEST_CASE( simple_search ) {
	BTree t;
	t.root = new BTree::Node();
	BTree::Node::KeyInfo<uint32_t> k( { 4, 6 }, 10, BTree::INVALID_INDEX);

	t.root->set_key<uint32_t>(0, k);
	t.root->set_key_cnt(1);

	auto r = t.search(0);
	BOOST_CHECK_EQUAL(r, BTree::INVALID_RULE);
	r = t.search(4);
	BOOST_CHECK_EQUAL(r, 10);

	r = t.search(5);
	BOOST_CHECK_EQUAL(r, 10);

	r = t.search(6);
	BOOST_CHECK_EQUAL(r, 10);

	r = t.search(7);
	BOOST_CHECK_EQUAL(r, BTree::INVALID_RULE);
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
