#pragma once
#include <array>
#include <vector>
#include <unordered_map>
#include <pcv/partition_sort/dimension_order_resolver.h>
#include <boost/functional/hash.hpp>

namespace pcv {

template<typename rule_spec_t>
struct rule_spec_t_eq {
	std::size_t operator()(const rule_spec_t& a, const rule_spec_t& b) const {
		if (a.second != b.second)
			return false;
		auto _b = b.first.begin();
		for (auto v : a.first) {
			if (v != *_b)
				return false;
			++_b;
		}
		return true;
	}
};

template<typename rule_spec_t>
struct rule_spec_t_hasher {
	std::size_t operator()(const rule_spec_t& k) const {
		using boost::hash_value;
		using boost::hash_combine;

		std::size_t seed = 0;
		for (auto v : k.first) {
			hash_combine(seed, hash_value(v.low));
			hash_combine(seed, hash_value(v.high));
		}
		hash_combine(seed, hash_value(k.second));

		// Return the result.
		return seed;
	}
};

/*
 * Partition sort like packet classifier classifier with configurable
 * tree type
 *
 * @tparam TREE_FIXATION_THRESHOLD if this threshold is exceeded the dimension order is locked for this tree
 *	 	and is not checked for optimality
 *
 **/
template<typename TREE_T, const size_t MAX_TREE_CNT,
		size_t TREE_FIXATION_THRESHOLD = 10>
class PartitionSortClassifer {
public:
	using value_t = typename TREE_T::value_t;
	using Node = typename TREE_T::Node;
	using rule_spec_t = typename TREE_T::rule_spec_t;
	using index_t = typename TREE_T::index_t;
	using KeyInfo = typename TREE_T::KeyInfo;
	using rule_id_t= typename TREE_T::rule_id_t;
	using val_vec_t= typename TREE_T::val_vec_t;

	// @note the trees are sorted [0] has the greatest max priority rule
	struct tree_info {
		TREE_T tree;
		rule_id_t max_priority;
		std::vector<rule_spec_t> rules;
	};
	std::array<tree_info, MAX_TREE_CNT> trees;
	size_t tree_cnt;

	// used to keep track of where are the rules stored for removing;
	std::unordered_map<rule_spec_t, TREE_T *, rule_spec_t_hasher<rule_spec_t>,
			rule_spec_t_eq<rule_spec_t>> rule_to_tree;

	PartitionSortClassifer() :
			tree_cnt(0) {
	}

	inline void update_dimension_order(tree_info & ti) {
		TREE_T & tree = ti.tree;
		GreedyDimensionOrderResolver<rule_spec_t, TREE_T::D, value_t> resolver(
				ti.rules, tree.dimension_order);
		auto new_dim_order = resolver.resolve();
		if (tree.dimension_order != new_dim_order) {
			// rebuild the tree because it has suboptimal dimension order
			delete tree.root;
			tree.root = nullptr;
			tree.dimension_order = new_dim_order;
			for (auto &r : ti.rules) {
				if (tree.does_rule_colide(r)) {
					// try to pick a different tree for this rule as it does not fit to this tree anymore
					// because its dimension order has changed
					insert(r);
				} else {
					// reinsert this rule to this tree as it still fits
					tree.insert(r);
				}
			}
		}
		// else no change is required as the dimension order is optimal (or nearly optimal)
	}

	/*
	 * Resort the trees if maximum priority of the tree has changed
	 * (sorting alg is stable = the order of equal items is not changed)
	 *
	 * @param original_i the index of the tree which priority has changed
	 * */
	void resort_on_priority_change(const size_t original_i) {
		if (tree_cnt == 1)
			return; // nothing to sort

		tree_info t = std::move(trees[original_i]);
		auto & rules = t.rules;
		assert(
				rules.size() > 1
						&& "in this case it is useless to call this function");
		auto p = t.max_priority;
		// try to move it to the left it has larger max p. rule
		if (original_i > 0) {
			size_t i = original_i;
			while (trees[i - 1].max_priority < p) {
				i--;
			}
			if (i != original_i) {
				// shift all smaller items one to right
				for (size_t i2 = original_i; i2 > i; i2--) {
					trees[i2] = std::move(trees[i2 - 1]);
				}
				// put the actual tree on correct place
				trees[i] = std::move(t);
				return;
			}
		}

		// try to move the tree to the right if it has lower priority
		if (original_i < tree_cnt - 1) {
			size_t i = original_i;
			while (trees[i + 1].max_priority > p) {
				i++;
			}
			if (i != original_i) {
				// shift all larger items one to left
				for (size_t i2 = original_i; i2 < i; i2++) {
					trees[i2] = std::move(trees[i2 + 1]);
				}
				// put the actual tree on correct place
				trees[i] = std::move(t);
				return;
			}
		}

	}

	/*
	 * Insert rule to tree most suitable for this rule
	 * and sort the tress by the max priority rule in descending order
	 * */
	inline void insert(const rule_spec_t & rule) {
		size_t i = 0;
		for (; i < tree_cnt; i++) {
			auto & t = trees[i];
			if (not t.tree.does_rule_colide(rule)) {
				t.tree.insert(rule);
				if (rule.second > t.max_priority) {
					t.max_priority = rule.second;
					resort_on_priority_change(i);
				}
				t.rules.push_back(rule);
				rule_to_tree[rule] = &t.tree;
				update_dimension_order(t);
				return;
			}
		}
		if (i < MAX_TREE_CNT) {
			// the rule does not fit to any tree, generate new tree for this rule
			auto & t = trees[i];
			t.max_priority = rule.second;
			t.tree.insert(rule);
			t.rules.push_back(rule);
			rule_to_tree[rule] = &t.tree;
			update_dimension_order(t);
			tree_cnt++;
		} else {
			throw std::runtime_error(
					"all tress used (need to implement tree merging)");
		}
	}

	/*
	 * Remove the rule if it is stored in classifier
	 * */
	inline void remove(const rule_spec_t & rule) {
		auto t = rule_to_tree.find(rule);
		if (t != rule_to_tree.end()) {
			t->second->remove(rule);
			rule_to_tree.remove(t);
			update_dimension_order(*t);
		}
	}
	inline rule_id_t search(const val_vec_t & val) {
		rule_id_t actual_found = TREE_T::INVALID_RULE;

		for (size_t i = 0; i < tree_cnt; i++) {
			auto & t = trees[i];
			actual_found = t.tree.search(val);
			if (actual_found != TREE_T::INVALID_RULE and i + 1 < tree_cnt
					and trees[i + 1].max_priority < actual_found) {
				break;
			}
		}

		return actual_found;
	}
};

}
