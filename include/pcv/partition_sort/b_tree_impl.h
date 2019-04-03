#pragma once

#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/b_tree_collision_check.h>
#include <pcv/partition_sort/b_tree_insert.h>
#include <pcv/partition_sort/b_tree_remove.h>

namespace pcv {

/*
 * Wrapper around _BTree which connects the algorithms and data part to a single class
 *
 * @tparam Key_t the type of key which will be used by the nodes in the tree
 * @tparam _D maximal number of levels of the tree (number of fields in packet/dimensions)
 * @tparam _T parameter which specifies the number of items per node
 * */
template<typename Key_t, size_t _D, size_t _T = 4, bool _PATH_COMPRESSION = true>
class BTreeImp: public _BTree<Key_t, _D, _T, _PATH_COMPRESSION> {
public:
	using BTree = _BTree<Key_t, _D, _T, _PATH_COMPRESSION>;
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

	inline void insert(const rule_spec_t & r) {
		BTreeInsert<BTreeImp>::insert(*this, r);
	}

	inline bool does_rule_colide(const rule_spec_t & r) {
		return BTreeCollisionCheck<BTreeImp>::does_rule_colide(*this, r);
	}

	inline rule_id_t search(const val_vec_t & v) {
		return BTreeSearch<BTreeImp>::search(*this, v);
	}
	inline void remove(const rule_spec_t & r) {
		BTreeRemove<BTreeImp>::remove(*this, r);
	}

};

}
