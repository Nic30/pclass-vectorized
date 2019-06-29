#pragma once

#include <pcv/partition_sort/b_tree.h>

namespace pcv {

template<typename Key_t, typename _Value_t, size_t _D, size_t _T, bool _PATH_COMPRESSION>
class _BTreeToRules {
	using BTree = _BTree<Key_t, _Value_t, _D, _T, _PATH_COMPRESSION>;

	using rule_spec_t = typename BTree::rule_spec_t;
	using Node = typename BTree::Node;
	using key_range_t = typename BTree::key_range_t;
	using rule_value_t = typename BTree::rule_value_t;
	static constexpr size_t D = BTree::D;
	using path_t = std::array<key_range_t, D>;

	const BTree & t;
	std::vector<rule_spec_t> & res;
	path_t actual_path;
	const key_range_t any;

public:
	_BTreeToRules(const BTree & _t, std::vector<rule_spec_t> & _res) :
			t(_t), res(_res), any(0,
					std::numeric_limits<typename key_range_t::T>::max()) {
		std::fill(actual_path.begin(), actual_path.end(), any);
	}

	/* Preorder extract all paths in tree which represent the rules
	 * */
	void to_rules() {
		to_rules(t.root, 0);
	}

private:
	void save_actual_path(rule_value_t prio_and_id) {
		res.push_back( { actual_path, prio_and_id });
	}
	void to_rules(const Node * n, size_t level) {
		if (n == nullptr)
			return;

		if (n->is_compressed) {
			for (size_t i = 0; i < n->key_cnt; i++) {
				size_t d = t.dimension_order[level + i];
				assert(actual_path[d] == any);
				auto k = n->get_key(i);
				assert(n->get_dim(i) == d);
				actual_path[d] = k.key;
				if (k.value.is_valid()) {
					save_actual_path(k.value);
				}
				to_rules(n->get_next_layer(i), level + i + 1);
			}
			for (size_t i = 0; i < n->key_cnt; i++) {
				size_t d = t.dimension_order[level + i];
				actual_path[d] = any;
			}
#ifndef NDEBUG
			for (size_t i = 0; i < size_t(n->key_cnt) + 1; i++) {
				auto c = n->child(i);
				assert(c == nullptr);
			}
#endif
		} else {
			size_t d = t.dimension_order[level];
			for (size_t i = 0; i < n->key_cnt; i++) {
				auto k = n->get_key(i);
				actual_path[d] = k.key;
				if (k.value.is_valid()) {
					save_actual_path(k.value);
				}
				to_rules(n->get_next_layer(i), level + 1);
			}
			actual_path[d] = any;
			for (size_t i = 0; i < size_t(n->key_cnt) + 1; i++) {
				auto c = n->child(i);
				if (c) {
					to_rules(c, level);
				}
			}
		}
	}
};

}
