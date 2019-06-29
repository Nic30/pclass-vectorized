#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"
#include <vector>

#include <pcv/partition_sort/b_tree_impl.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

class BTree: public BTreeImp<uint16_t, IntRuleValue, 2, 4, false> {
public:
	rule_value_t search(key_t _v) const {
		key_vec_t v = { _v, _v };
		return BTreeImp::search(v);
	}

	rule_value_t search(const std::vector<key_t> & _v) const {
		key_vec_t v;
		std::copy(_v.begin(), _v.begin() + D, v.begin());
		return BTreeImp::search(v);
	}
};

void test_insert_and_search(size_t STEP, size_t RANGE_SIZE, size_t N) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::key_range_t;
	R1d any(0, numeric_limits<BTree::key_t>::max());
	for (BTree::rule_id_t i = 0; i < N; i++) {
		rule_t r = { { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), any, },
				{ 0, i } };
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
			BTree::key_t s = (i * STEP + i2);
			//cout << s  << " expecting " << i << endl;
			vector<BTree::key_t> v = { s, };
			auto res = t.search(v);
			BOOST_CHECK_EQUAL_MESSAGE(res.rule_id, i,
					"searching:" << s << " i:" << i << " i2:" << i2);
		}
	}
}

void test_insert_remove_and_search(size_t STEP, size_t RANGE_SIZE, size_t N) {
	size_t allocated_node_cnt_before = BTree::Node::_Mempool_t::size();
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::key_range_t;
	R1d any(0, numeric_limits<BTree::key_t>::max());
	for (BTree::rule_id_t i = 0; i < N; i++) {
		rule_t r = { { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), any, },
				{ 0, i } };
		t.insert(r);
		t.root->integrity_check(t.dimension_order);
	}

	//stringstream ss;
	//ss << "b_tree_" << STEP << "_" << RANGE_SIZE << "_" << N << ".dot";
	//ofstream o(ss.str());
	//o << t;
	//o.close();

	for (BTree::rule_id_t i = 0; i < N; i++) {
		rule_t r = { { R1d(i * STEP, i * STEP + RANGE_SIZE - 1), any, },
				{ 0, i } };
		t.remove(r);
		BOOST_CHECK_EQUAL(t.size(), (N - i - 1));
		for (size_t i3 = i + 1; i3 < N; i3++) {
			for (size_t i2 = 0; i2 < RANGE_SIZE; i2++) {
				auto s = (i3 * STEP + i2);
				auto res = t.search(s);
				BOOST_CHECK_EQUAL_MESSAGE(res.rule_id, i3,
						"searching:" << s << " step:" << i << " expected:" << i3 << " i2:" << i2 << " got:" << res.rule_id);
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
		BOOST_CHECK_EQUAL_MESSAGE(res.rule_id, BTree::INVALID_RULE,
				"searching:" << s << " i:" << i);
	}
	BOOST_CHECK_EQUAL(BTree::Node::_Mempool_t::size(),
			allocated_node_cnt_before);
}

BOOST_AUTO_TEST_CASE( simple_search ) {
	BTree t;
	t.root = new BTree::Node;
	BTree::KeyInfo k( { 4, 6 }, { 0, 10 }, BTree::INVALID_INDEX);

	t.root->set_key(0, k);
	t.root->set_key_cnt(1);

	auto r = t.search(0);
	BOOST_CHECK_EQUAL(r.rule_id, BTree::INVALID_RULE);
	r = t.search(4);
	BOOST_CHECK_EQUAL(r.rule_id, 10);

	r = t.search(5);
	BOOST_CHECK_EQUAL(r.rule_id, 10);

	r = t.search(6);
	BOOST_CHECK_EQUAL(r.rule_id, 10);

	r = t.search(7);
	BOOST_CHECK_EQUAL(r.rule_id, BTree::INVALID_RULE);
}

BOOST_AUTO_TEST_CASE( simple_insert ) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::key_range_t;
	R1d any(0, numeric_limits<BTree::key_t>::max());
	rule_t r1 = { { R1d(1, 1), any, }, { 0, 1 } };
	rule_t r2 = { { R1d(3, 6), any, }, { 0, 2 } };
	rule_t r3 = { { R1d(7, 10), any, }, { 0, 3 } };

	t.insert(r1);
	t.insert(r2);
	t.insert(r3);

	BOOST_CHECK_EQUAL(t.search(0).rule_id, BTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(t.search(1).rule_id, 1);
	BOOST_CHECK_EQUAL(t.search(2).rule_id, BTree::INVALID_RULE);
	BOOST_CHECK_EQUAL(t.search(3).rule_id, 2);
	BOOST_CHECK_EQUAL(t.search(6).rule_id, 2);
	BOOST_CHECK_EQUAL(t.search(7).rule_id, 3);
	BOOST_CHECK_EQUAL(t.search(10).rule_id, 3);
	BOOST_CHECK_EQUAL(t.search(11).rule_id, BTree::INVALID_RULE);
}

BOOST_AUTO_TEST_CASE( simple_insert_unordered ) {
	vector<pair<uint16_t, uint16_t>> values =
			{ { 21, 21 }, { 135, 135 }, { 30211, 30211 }, { 1521, 1521 }, {
					1526, 1526 }, { 5632, 5632 }, { 1432, 1432 },
					{ 1704, 1704 }, { 1707, 1707 }, { 6849, 6849 },
					{ 1221, 1221 }, { 1490, 1490 }, { 2121, 2121 }, { 19856,
							19856 }, { 1525, 1525 }, { 1733, 1733 }, { 2026,
							2026 }, { 1708, 1708 }, { 1724, 1724 },
					{ 1717, 1717 }, { 1707, 1707 }, { 3031, 3031 },
					{ 5540, 5540 }, { 32201, 32201 }, { 6890, 6890 }, { 14753,
							14753 }, { 1433, 1433 }, { 5631, 5631 }, { 6000,
							6000 }, { 1712, 1712 }, { 80, 80 }, { 1706, 1706 },
					{ 1550, 1550 }, { 1706, 1706 }, { 5555, 5555 }, };
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::key_range_t;
	R1d any(0, numeric_limits<BTree::key_t>::max());
	BTree::rule_id_t i = 0;
	for (auto val : values) {
		rule_t r = { { R1d(val.first, val.second), any, }, { 0, i } };
		t.insert(r);
		//{
		//	stringstream ss;
		//	ss << "simple_insert_unordered+" << i << ".dot";
		//	ofstream o(ss.str());
		//	assert(o.is_open());
		//	o << t;
		//	o.close();
		//}

		t.root->integrity_check(t.dimension_order);
		i++;
	}
}

BOOST_AUTO_TEST_CASE( simple_insert_same ) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::key_range_t;
	R1d any(0, numeric_limits<BTree::key_t>::max());
	for (BTree::rule_id_t i = 0; i < 1024; i++) {
		rule_t r = { { R1d(0, 0), any, }, { 0, i } };
		t.insert(r);
	}
	t.root->integrity_check(t.dimension_order);
}

BOOST_AUTO_TEST_CASE( simple_insert_same_into_something ) {
	BTree t;
	using rule_t = BTree::rule_spec_t;
	using R1d = BTree::key_range_t;
	R1d any(0, numeric_limits<BTree::key_t>::max());
	for (BTree::rule_id_t i = 0; i < 1024; i++) {
		rule_t r = { { R1d(i, i), any, }, { 0, i } };
		t.insert(r);
	}
	for (BTree::rule_id_t i = 0; i < 1024; i++) {
		rule_t r = { { R1d(512, 512), any, }, { 0, i } };
		t.insert(r);
	}
	t.root->integrity_check(t.dimension_order);
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
