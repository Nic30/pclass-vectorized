#pragma once

#include <pcv/partition_sort/b_tree.h>

#include <pcv/partition_sort/b_tree_node_navigator.h>
namespace pcv {

template<typename BTree>
class _BTreeToRules: public _BTreeNodeNavigator<BTree> {
	using rule_spec_t = typename BTree::rule_spec_t;
	using Node = typename BTree::Node;
	using key_range_t = typename BTree::key_range_t;
	using rule_value_t = typename BTree::rule_value_t;
	static constexpr size_t D = BTree::D;
	using path_t = std::array<key_range_t, D>;

	std::vector<rule_spec_t> &res;
	path_t actual_path;
	const key_range_t any;

public:
	_BTreeToRules(BTree &_t, std::vector<rule_spec_t> &_res) :
		_BTreeNodeNavigator<BTree>(_t), res(_res), any(0,
					std::numeric_limits<typename key_range_t::T>::max()) {
		std::fill(actual_path.begin(), actual_path.end(), any);
	}

	/* Preorder extract all paths in tree which represent the rules
	 * */
	void to_rules() {
		to_rules(this->tree.root, 0);
	}

private:
	void save_actual_path(rule_value_t prio_and_id) {
		res.push_back( { actual_path, prio_and_id });
	}
	void to_rules(Node *n, size_t level) {
		if (n == nullptr)
			return;

		if (n->is_compressed) {
			for (size_t i = 0; i < n->key_cnt; i++) {
				size_t d = this->tree.dimension_order[level + i];
				assert(actual_path[d] == any);
				auto k = n->get_key(i);
				assert(n->get_dim(i) == d);
				actual_path[d] = k.key;
				if (k.value.is_valid()) {
					save_actual_path(k.value);
				}
				to_rules(this->get_next_layer(*n, i), level + i + 1);
			}
			for (size_t i = 0; i < n->key_cnt; i++) {
				size_t d = this->tree.dimension_order[level + i];
				actual_path[d] = any;
			}
#ifndef NDEBUG
			for (size_t i = 0; i < size_t(n->key_cnt) + 1; i++) {
				auto c = this->child(*n, i);
				assert(c == nullptr);
			}
#endif
		} else {
			size_t d = this->tree.dimension_order[level];
			for (size_t i = 0; i < n->key_cnt; i++) {
				auto k = n->get_key(i);
				actual_path[d] = k.key;
				if (k.value.is_valid()) {
					save_actual_path(k.value);
				}
				to_rules(this->get_next_layer(*n, i), level + 1);
			}
			actual_path[d] = any;
			for (size_t i = 0; i < size_t(n->key_cnt) + 1; i++) {
				auto c = this->child(*n, i);
				if (c) {
					to_rules(c, level);
				}
			}
		}
	}
};

}
