#pragma once
#include <pcv/partiton_sort/b_tree.h>

namespace pcv {

SearchResult search_seq(const BTree::Node & node, BTree::value_t val);
SearchResult search_seq(const BTree::Node & node,
		const Range1d<BTree::value_t> val);
/*
 * Search the value or range in single level of the tree
 *
 * @return pair<node where key resides, index of the key in node>
 * */
template<typename value_t>
std::pair<BTree::Node*, unsigned> search_possition_1d(BTree::Node * n,
		const value_t val) {
	while (n) {
		//auto s = search_avx2(*n, val);
		auto s = search_seq(*n, val);
		if (s.in_range) {
			return {n, s.val_index};
		} else if (n->is_leaf) {
			return {nullptr, 0};
		} else {
			n = n->child(s.val_index);
		}
	}
	return {nullptr, 0};
}

}
