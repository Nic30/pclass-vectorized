#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <pcv/partiton_sort/b_tree.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

void test_insert_and_search(size_t STEP, size_t RANGE_SIZE, size_t N) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = Range1d<uint32_t>;
	for (size_t i = 0; i < N; i++) {
		rule_t r =
				{ { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), R1d(0, 0), }, i };
		t.insert(r);
	}

	//stringstream ss;
	//ss << "b_tree_" << STEP << "_" << RANGE_SIZE << "_" << N << ".dot";
	//ofstream o(ss.str());
	//o << t;
	//o.close();

	// test all values possible in the tree
	for (size_t i = 0; i < N; i++) {
		for (size_t i2 = 0; i2 < RANGE_SIZE; i2++) {
			auto s = (i * STEP + i2);
			//cout << s  << " expecting " << i << endl;
			auto res = t.search(s);
			BOOST_CHECK_EQUAL_MESSAGE(res, i,
					"searching:" << s << " i:" << i << " i2:" << i2);
		}
	}
}

void test_insert_remove_and_search(size_t STEP, size_t RANGE_SIZE, size_t N) {
	size_t allocated_node_cnt_before = BTree::Node::_Mempool_t::size();
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = Range1d<uint32_t>;
	for (size_t i = 0; i < N; i++) {
		rule_t r =
				{ { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), R1d(0, 0), }, i };
		t.insert(r);
		integrity_check(*t.root);
	}

	//stringstream ss;
	//ss << "b_tree_" << STEP << "_" << RANGE_SIZE << "_" << N << ".dot";
	//ofstream o(ss.str());
	//o << t;
	//o.close();

	for (size_t i = 0; i < N; i++) {
		rule_t r =
				{ { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), R1d(0, 0), }, i };
		t.remove(r);
		BOOST_CHECK_EQUAL(t.size(), 2 * (N - i - 1));
		for (size_t i3 = i + 1; i3 < N; i3++) {
			for (size_t i2 = 0; i2 < RANGE_SIZE; i2++) {
				auto s = (i3 * STEP + i2);
				//cout << s  << " expecting " << i << endl;
				auto res = t.search(s);
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
		auto res = t.search(s);
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

BOOST_AUTO_TEST_CASE( simple_insert ) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = Range1d<uint32_t>;
	rule_t r1 = { { R1d(1, 1), R1d(0, 0), }, 1 };
	rule_t r2 = { { R1d(3, 6), R1d(0, 0), }, 2 };
	rule_t r3 = { { R1d(7, 10), R1d(0, 0), }, 3 };

	t.insert(r1);
	t.insert(r2);
	t.insert(r3);

	BOOST_CHECK_EQUAL(t.search(0), BTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(t.search(1), 1);
	BOOST_CHECK_EQUAL(t.search(2), BTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(t.search(3), 2);
	BOOST_CHECK_EQUAL(t.search(6), 2);
	BOOST_CHECK_EQUAL(t.search(7), 3);
	BOOST_CHECK_EQUAL(t.search(10), 3);
	BOOST_CHECK_EQUAL(t.search(11), BTree::INVALID_RULE);
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
