#pragma once
#include <string>

#include <pcv/common/range.h>
#include <pcv/partiton_sort/b_tree_search.h>

namespace pcv {

template<typename BTree>
class BTreeCollisionCheck {
public:
	using rule_spec_t = typename BTree::rule_spec_t;
	using value_t = typename BTree::value_t;
	using Node = typename BTree::Node;
	using KeyInfo = typename BTree::KeyInfo;
	using KeyIterator = typename BTreeSearch<BTree>::KeyIterator;
private:
	static SearchResult search_closest_lower_or_equal_seq(const Node & node,
			value_t val) {
		SearchResult r;
		for (r.val_index = 0; r.val_index < node.key_cnt; r.val_index++) {
			KeyInfo cur = node.get_key(r.val_index);
			if (val < cur.key.low) {
				break;
			} else if (cur.in_range(val)) {
				r.in_range = true;
				break;
			}
		}
		return r;
	}

	static std::pair<Node*, unsigned> search_closest_lower_or_equal_key(
			Node * n, const value_t val) {
		while (n) {
			auto s = search_closest_lower_or_equal_seq(*n, val);
			if (s.in_range) {
				return {n, s.val_index};
			} else if (n->is_leaf) {
				if (not s.in_range) {
					KeyIterator _it(n, 0);
					auto it = _it.begin();
					--it;
					return {it.actual, it.index};
				} else {
					return {n, s.val_index};
				}
			} else {
				n = n->child(s.val_index);
			}
		}
		return {nullptr, 0};
	}
public:
	/*
	 * Check if the rule collides wit any other rule in the tree
	 *
	 * @note the rule however can overwrite other rule or can use its prefix without the collision
	 * */
	static bool does_rule_colide(BTree & tree, const rule_spec_t & rule) {
		// for each dimension
		auto t = tree.root;

		for (size_t i = 0; i < BTree::D; i++) {
			if (not t)
				return false;

			auto d = tree.dimension_order[i];
			auto d_val = rule.first[d];
			auto p_low = search_closest_lower_or_equal_key(t, d_val.low);
			Range1d<value_t> lk;
			bool lk_found = false;
			if (p_low.first) {
				lk = p_low.first->get_key(p_low.second).key;
				lk_found = true;
			}
			if (lk_found and lk == d_val) {
				// not overlapping on this level
			} else {
				Range1d<value_t> p_low_next;
				if (p_low.first) {
					KeyIterator _it(p_low.first, p_low.second);
					p_low_next = (*_it.begin()).key;
				} else {
					KeyIterator _it(tree.root, 0);
					p_low_next = (*_it.begin()).key;
				}
				if (p_low_next.overlaps(d_val)) {
					return true;
				}
				// there is space enough big to fir d_val
			}

			if (not lk_found) {
				// no need to search in other levels as there is no direct path to it
				return false;
			}

			bool last_it = i == BTree::D - 1;
			if (not last_it)
				t = p_low.first->get_next_layer(p_low.second);
			// search the key which starts on the larger or equal to this one
			// go left until the key which ends on lower value than this value starts
			// if there are some keys in this range and it is not single key which is equal to this one
			//		this rule collides with this ranges
		}
		return false;
	}
};
}
