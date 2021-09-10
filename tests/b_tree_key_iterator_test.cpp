#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/b_tree_insert.h>
#include <pcv/partition_sort/rule_value_int.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )
using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 2>>;

using rule_t = BTree::rule_spec_t;
using R1d = Range1d<BTree::key_t>;

BOOST_AUTO_TEST_CASE( iterate_100_ranges ) {
	size_t N = 100;
	BTree::NodeAllocator mempool(10*1024);
	BTree t(mempool);

	for (BTree::rule_id_t i = 0; i < N; i++) {
		rule_t r = { { R1d(i, i), R1d(0, 0), }, {0, i} };
		t.insert(r);
	}

	size_t i = 0;
	BTree::Search_t search(t);
	for (auto k : search.iter_keys()) {
		BOOST_CHECK_EQUAL(k.key.low, i);
		BOOST_CHECK_EQUAL(k.key.high, i);
		i++;
	}
	BOOST_CHECK_EQUAL(i, N);

}

BOOST_AUTO_TEST_CASE( iterate_100_ranges_backward ) {
	size_t N = 100;
	BTree::NodeAllocator mempool(10*1024);
	BTree t(mempool);
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::key_range_t;
	for (BTree::rule_id_t i = 0; i < N; i++) {
		rule_t r = { { R1d(i, i), R1d(0, 0), }, {0, i} };
		t.insert(r);
	}


	BTree::Search_t search(t);
	auto _it = search.iter_keys();
	auto it = _it.begin();
	for (size_t i = 0; i < N - 1; i++) {
		BOOST_CHECK_EQUAL((*it).key.low, i);
		//BOOST_CHECK_EQUAL((*it).key.high, i);
		++it;
		BOOST_CHECK_MESSAGE(it != _it.end(), i);
	}
	BOOST_CHECK_EQUAL((*it).key.low, N - 1);
	BOOST_CHECK_EQUAL((*it).key.high, N - 1);

	size_t i = N - 1;
	for (; it != _it.end(); --it) {
		BOOST_CHECK_EQUAL((*it).key.low, i);
		//BOOST_CHECK_EQUAL((*it).key.high, i);
		i--;
	}

}

BOOST_AUTO_TEST_SUITE_END()
