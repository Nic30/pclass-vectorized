#pragma once
#include <string>

#include <pcv/common/range.h>
#include <pcv/partition_sort/b_tree_search.h>
#include <pcv/partition_sort/b_tree_key_iterator.h>
#include <pcv/partition_sort/b_tree_node_navigator.h>

namespace pcv {

template<typename BTree>
class BTreeCollisionCheck: public _BTreeNodeNavigator<BTree> {
	using BTreeSearch_t = BTreeSearch<BTree>;
public:
	using rule_spec_t = typename BTree::rule_spec_t;
	using key_t = typename BTree::key_t;
	using Node = typename BTree::Node;
	using KeyInfo = typename BTree::KeyInfo;
	using KeyIterator = BTreeKeyIterator<BTree, KeyInfo>;
	using level_t = typename BTree::level_t;
private:

	using _BTreeNodeNavigator<BTree>::_BTreeNodeNavigator;
	SearchResult search_closest_lower_or_equal_seq(const Node &node,
			key_t val) {
		SearchResult r;
		for (r.val_index = 0; r.val_index < node.key_cnt; r.val_index++) {
			KeyInfo cur = node.get_key(r.val_index);
			if (val < cur.key.low) {
				break;
			} else if (cur.key.in_range(val)) {
				r.in_range = true;
				break;
			}
		}
		return r;
	}

	// [TODO] replace pair with iterator
	std::pair<Node*, unsigned> search_closest_lower_or_equal_key(Node *n,
			const key_t val) {
		while (n) {
			auto s = search_closest_lower_or_equal_seq(*n, val);
			if (s.in_range) {
				return {n, s.val_index};
			} else if (n->is_leaf) {
				if (not s.in_range) {
					KeyIterator _it(this->tree, n, 0);
					auto it = _it.begin();
					--it;
					return {it.actual, it.index};
				} else {
					return {n, s.val_index};
				}
			} else {
				n = this->child(*n, s.val_index);
			}
		}
		return {nullptr, 0};
	}
public:
	/*
	 *
	 * Check if the rule collides wit any other rule in the tree
	 *
	 * [TODO] return also level and node where can insert continue
	 * @note the rule however can overwrite other rule or can use its prefix without the collision
	 * */
	bool does_rule_colide(const rule_spec_t &rule) {
		// for each dimension
		Node* n = this->tree.root;

		level_t start_of_this_node = 0;
		for (level_t level = 0; level < BTree::D; level++) {
			if (not n) {
				return false;
			}
			auto d = this->tree.dimension_order[level];
			auto d_val = rule.first[d];
			if (n->is_compressed) {
				// now walking linear list of keys stored in compressed node
				level_t in_node_key_i = level - start_of_this_node;
				auto k = n->get_key(in_node_key_i);
				if (k.key == d_val) {
				} else if (k.key.overlaps(d_val)) {
					return true;
				} else {
					return false;
				}
				if (in_node_key_i + 1 == n->key_cnt) {
					n = this->get_next_layer(*n, in_node_key_i);
				}
				continue;
			}

			auto p_low = search_closest_lower_or_equal_key(n, d_val.low);
			Range1d<key_t> lk;
			bool lk_found = false;
			if (p_low.first) {
				lk = p_low.first->get_key(p_low.second).key;
				lk_found = true;
			}

			if (lk_found and lk == d_val) {
				// not overlapping on this level, but we need to check the next level
			} else {
				KeyIterator _it(this->tree, nullptr, 0);
				if (p_low.first) {
					// we know we found lower key range which is not equal to key range from rule
					_it = KeyIterator(this->tree, (Node*)p_low.first, (unsigned)p_low.second);
				} else {
					auto _min = _it._begin.get_most_left(n);
					_it = KeyIterator(this->tree, _min, 0);
				}
				// before this position the keys in tree can not collide
				auto k_it = _it.begin();
				while (true) {
					if (k_it == _it.end())
						return false;
					auto k = (*k_it).key;
					if (k.overlaps(d_val)) {
						return true;
					}
					if (k.low > d_val.high) {
						// the range from rule does not collide with any key in tree
						return false;
					}
					++k_it;
				}

				// there is space big enough to fit d_val
				return false;
			}
			if (not lk_found) {
				// no need to search in other levels as there is no direct path to it
				return false;
			}

			bool last_it = level == BTree::D - 1;
			if (not last_it) {
				n = this->get_next_layer(*p_low.first, p_low.second);
				start_of_this_node = level;
			}
			// search the key which starts on the larger or equal to this one
			// go left until the key which ends on lower value than this value starts
			// if there are some keys in this range and it is not single key which is equal to this one
			//		this rule collides with this ranges
		}
		return false;
	}
};
}
