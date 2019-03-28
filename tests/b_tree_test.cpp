#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partiton_sort/b_tree.h>
#include <pcv/partiton_sort/b_tree_search.h>
#include <pcv/partiton_sort/b_tree_insert.h>
#include <pcv/partiton_sort/b_tree_remove.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )
using BTree = _BTree<uint16_t, 2>;

void insert(BTree & t, BTree::rule_spec_t & r) {
	BTreeInsert<BTree>::insert(t, r);
}
BTree::rule_id_t search(BTree & t, BTree::value_t s) {
	std::array<BTree::value_t, 2> v = {s, s};
	return BTreeSearch<BTree>::search(t, v);
}

BTree::rule_id_t search(BTree & t,
		const std::vector<typename BTree::value_t> & v) {
	std::array<BTree::value_t, 2> _v;
	std::copy(v.begin(), v.begin() + 2, _v.begin());
	return BTreeSearch<BTree>::search(t, _v);
}
void remove(BTree & t, const BTree::rule_spec_t & r) {
	BTreeRemove<BTree>::remove(t, r);
}

void test_insert_and_search(size_t STEP, size_t RANGE_SIZE, size_t N) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::val_range_t;
	for (size_t i = 0; i < N; i++) {
		rule_t r =
				{ { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), R1d(0, 0), }, i };
		insert(t, r);
	}

	//stringstream ss;
	//ss << "b_tree_" << STEP << "_" << RANGE_SIZE << "_" << N << ".dot";
	//ofstream o(ss.str());
	//o << t;
	//o.close();

	// test all values possible in the tree
	for (size_t i = 0; i < N; i++) {
		for (size_t i2 = 0; i2 < RANGE_SIZE; i2++) {
			BTree::value_t s = (i * STEP + i2);
			//cout << s  << " expecting " << i << endl;
			vector<BTree::value_t> v = { s, };
			auto res = search(t, v);
			BOOST_CHECK_EQUAL_MESSAGE(res, i,
					"searching:" << s << " i:" << i << " i2:" << i2);
		}
	}
}

void test_insert_remove_and_search(size_t STEP, size_t RANGE_SIZE, size_t N) {
	size_t allocated_node_cnt_before = BTree::Node::_Mempool_t::size();
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::val_range_t;
	for (size_t i = 0; i < N; i++) {
		rule_t r =
				{ { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), R1d(0, 0), }, i };
		insert(t, r);
		t.root->integrity_check();
	}

	//stringstream ss;
	//ss << "b_tree_" << STEP << "_" << RANGE_SIZE << "_" << N << ".dot";
	//ofstream o(ss.str());
	//o << t;
	//o.close();

	for (size_t i = 0; i < N; i++) {
		rule_t r =
				{ { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), R1d(0, 0), }, i };
		remove(t, r);
		BOOST_CHECK_EQUAL(t.size(), 2 * (N - i - 1));
		for (size_t i3 = i + 1; i3 < N; i3++) {
			for (size_t i2 = 0; i2 < RANGE_SIZE; i2++) {
				auto s = (i3 * STEP + i2);
				//cout << s  << " expecting " << i << endl;
				auto res = search(t, s);
				BOOST_CHECK_EQUAL_MESSAGE(res, i3,
						"searching:" << s << " step:" << i << " expected:" << i3 << " i2:" << i2);
			}
		}

		//{
		//	stringstream ss;
		//	ss << "b_tree_remove_" << i << "_" << STEP << "_" << RANGE_SIZE
		//			<< "_" << N << ".dot";
		//	ofstream o(ss.str());
		//	assert(o.is_open());
		//	o << t;
		//	o.close();
		//}

	}
	BOOST_CHECK(t.root == nullptr);

	// test all values possible in the tree
	for (size_t i = 0; i < N; i++) {
		auto s = (i * STEP);
		auto res = search(t, s);
		BOOST_CHECK_EQUAL_MESSAGE(res, BTree::INVALID_INDEX,
				"searching:" << s << " i:" << i);
	}
	BOOST_CHECK_EQUAL(BTree::Node::_Mempool_t::size(),
			allocated_node_cnt_before);
}

BOOST_AUTO_TEST_CASE( simple_search ) {
	BTree t;
	t.root = new BTree::Node;
	BTree::KeyInfo k( { 4, 6 }, 10, BTree::INVALID_INDEX);

	t.root->set_key(0, k);
	t.root->set_key_cnt(1);

	auto r = search(t, 0);
	BOOST_CHECK_EQUAL(r, BTree::INVALID_RULE);
	r = search(t, 4);
	BOOST_CHECK_EQUAL(r, 10);

	r = search(t, 5);
	BOOST_CHECK_EQUAL(r, 10);

	r = search(t, 6);
	BOOST_CHECK_EQUAL(r, 10);

	r = search(t, 7);
	BOOST_CHECK_EQUAL(r, BTree::INVALID_RULE);
}

BOOST_AUTO_TEST_CASE( simple_insert ) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::val_range_t;
	rule_t r1 = { { R1d(1, 1), R1d(0, 0), }, 1 };
	rule_t r2 = { { R1d(3, 6), R1d(0, 0), }, 2 };
	rule_t r3 = { { R1d(7, 10), R1d(0, 0), }, 3 };

	insert(t, r1);
	insert(t, r2);
	insert(t, r3);

	BOOST_CHECK_EQUAL(search(t, 0), BTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(search(t, 1), 1);
	BOOST_CHECK_EQUAL(search(t, 2), BTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(search(t, 3), 2);
	BOOST_CHECK_EQUAL(search(t, 6), 2);
	BOOST_CHECK_EQUAL(search(t, 7), 3);
	BOOST_CHECK_EQUAL(search(t, 10), 3);
	BOOST_CHECK_EQUAL(search(t, 11), BTree::INVALID_RULE);
}

BOOST_AUTO_TEST_CASE( search_in_one_full_node) {
	test_insert_and_search(13, 11, BTree::Node::MAX_DEGREE);
}

BOOST_AUTO_TEST_CASE( search_in_2_nodes) {
	test_insert_and_search(13, 11, BTree::Node::MAX_DEGREE + 1);
}

BOOST_AUTO_TEST_CASE( search_in_unit_ranges ) {
	for (int i = 5; i < 50; i++)
		test_insert_and_search(1, 1, i);
}

BOOST_AUTO_TEST_CASE( search_in_5000_ranges ) {
	test_insert_and_search(13, 11, 5000);
}

BOOST_AUTO_TEST_CASE( search_in_1_removed_ranges ) {
	test_insert_remove_and_search(1, 1, 1);
}

BOOST_AUTO_TEST_CASE( search_in_4_removed_ranges ) {
	test_insert_remove_and_search(1, 1, 4);
}

BOOST_AUTO_TEST_CASE( search_in_8_removed_ranges ) {
	test_insert_remove_and_search(1, 1, 8);
}

BOOST_AUTO_TEST_CASE( search_in_9_removed_ranges ) {
	test_insert_remove_and_search(1, 1, 9);
}
BOOST_AUTO_TEST_CASE( search_in_16_removed_ranges ) {
	test_insert_remove_and_search(1, 1, 16);
}

BOOST_AUTO_TEST_CASE( search_in_100_removed_ranges ) {
	test_insert_remove_and_search(13, 11, 100);
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
