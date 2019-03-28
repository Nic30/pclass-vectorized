#pragma once

#include <pcv/partiton_sort/b_tree.h>
#include <pcv/partiton_sort/b_tree_insert.h>
#include <pcv/partiton_sort/b_tree_remove.h>
#include <pcv/partiton_sort/b_tree_collision_check.h>

namespace pcv {

template<typename Key_t, size_t _D, size_t _T = 4>
class BTreeImp: public _BTree<Key_t, _D, _T> {
public:
	using BTree = _BTree<Key_t, _D, _T>;
	using Node = typename BTree::Node;
	using rule_id_t = typename BTree::rule_id_t;
	static constexpr size_t D = BTree::D;
	using val_range_t = typename BTree::val_range_t;
	using rule_spec_t = typename BTree::rule_spec_t;
	using value_t = typename BTree::value_t;
	using index_t = typename BTree::index_t;
	using KeyInfo = typename BTree::KeyInfo;
	using val_vec_t = typename BTree::val_vec_t;

	static constexpr index_t INVALID_INDEX = BTree::INVALID_INDEX;
	static constexpr rule_id_t INVALID_RULE = BTree::INVALID_RULE;

	void insert(const rule_spec_t & r) {
		BTreeInsert<BTreeImp>::insert(*this, r);
	}

	bool does_rule_colide(const rule_spec_t & r) {
		return BTreeCollisionCheck<BTreeImp>::does_rule_colide(*this, r);
	}

	rule_id_t search(const val_vec_t & v) {
		return BTreeSearch<BTreeImp>::search(*this, v);
	}
	void remove(const rule_spec_t & r) {
		BTreeRemove<BTreeImp>::remove(*this, r);
	}

};

}
