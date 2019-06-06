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
	using formaters_t = typename TREE_T::formaters_t;
	using names_t = typename TREE_T::names_t;
	static constexpr index_t INVALID_INDEX = TREE_T::INVALID_INDEX;
	static constexpr rule_id_t INVALID_RULE = TREE_T::INVALID_RULE;

	// @note the trees are sorted [0] has the greatest max priority rule
	struct tree_info {
		TREE_T tree;
		rule_id_t max_priority;
		std::vector<rule_spec_t> rules;
		tree_info() :
				tree() {
		}

		tree_info(const formaters_t & _formaters, const names_t & _names) :
				tree(_formaters, _names) {

		}
	};
	std::array<tree_info*, MAX_TREE_CNT> trees;
	size_t tree_cnt;

	// used to keep track of where are the rules stored for removing;
	std::unordered_map<rule_spec_t, tree_info *,
			rule_spec_t_hasher<rule_spec_t>, rule_spec_t_eq<rule_spec_t>> rule_to_tree;

	PartitionSortClassifer() :
			tree_cnt(0) {
		for (size_t i = 0; i < MAX_TREE_CNT; i++) {
			trees[i] = new tree_info;
		}
	}
	PartitionSortClassifer(const formaters_t & _formaters,
			const names_t & _names) :
			tree_cnt(0) {
		for (size_t i = 0; i < MAX_TREE_CNT; i++) {
			trees[i] = new tree_info(_formaters, _names);
		}
	}

	// @return true if the dimension order changed
	inline bool update_dimension_order(tree_info & ti) {
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
			return true;
		}
		return false;
		// else no change is required as the dimension order is optimal (or nearly optimal)
	}
	inline void assert_all_trees_unique() {
#ifndef NDEBUG
		std::set<tree_info*> tree_set;
		for (auto ti : trees)
			tree_set.insert(ti);
		assert(tree_set.size() == MAX_TREE_CNT);
#endif
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

		// we can not use directly tree_info because the destructor of tmp variable would be called
		tree_info * t_tmp = trees[original_i];
		assert(
				t_tmp->rules.size() > 0
						&& "in this case it is useless to call this function");
		auto p = t_tmp->max_priority;
		// try to move it to the left it has larger max priority rule
		if (original_i > 0) {
			size_t i = original_i;
			// while predecesor has lower priority
			// find the index of the first item with the same or larger priority
			while (i >= 1 and trees[i - 1]->max_priority < p) {
				i--;
			}
			if (i != original_i) {
				// shift all items with the lower max priority one to right
				for (size_t i2 = original_i; i2 > i; i2--) {
					trees[i2] = std::move(trees[i2 - 1]);
				}
				// put the actual tree on correct place
				trees[i] = std::move(t_tmp);
				assert_all_trees_unique();
				return;
			}

		}

		// try to move the tree to the right if it has lower max priority
		if (original_i < tree_cnt - 1) {
			size_t i = original_i;
			// while successor has greater priority
			while (i < tree_cnt - 1 and trees[i + 1]->max_priority > p) {
				i++;
			}
			if (i != original_i) {
				// shift all items with the greater priority one to left
				for (size_t i2 = original_i; i2 < i; i2++) {
					trees[i2] = std::move(trees[i2 + 1]);
				}
				// put the actual tree on correct place
				trees[i] = std::move(t_tmp);
				assert_all_trees_unique();
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
			auto & t = *trees[i];
			if (not t.tree.does_rule_colide(rule)) {
				t.rules.push_back(rule);
				rule_to_tree[rule] = &t;
				if (t.rules.size() < TREE_FIXATION_THRESHOLD) {
					if (update_dimension_order(t) == false)
						t.tree.insert(rule);
					// the rule is inserted automatically as it is in t.rules
				} else {
					// [TODO] in new dimension is used in rule which was not used in tree previously
					//        it is required to update dimension order to put the new dimension,
					//        being previously used, in order to prevent sparse branches in tree.
					t.tree.insert(rule);
				}
				if (rule.second > t.max_priority) {
					t.max_priority = rule.second;
					resort_on_priority_change(i);
				}
				return;
			}
		}
		if (i < MAX_TREE_CNT) {
			// the rule does not fit to any tree, generate new tree for this rule
			auto & t = *trees[i];
			t.max_priority = rule.second;
			if (t.rules.size() == 0) {
				// update default dimension order to fit current rule
				// in order to avoid useless tree reconstruction
				std::vector<rule_spec_t> _rules = { rule, };
				GreedyDimensionOrderResolver<rule_spec_t, TREE_T::D, value_t> resolver(
						_rules, t.tree.dimension_order);
				t.tree.dimension_order = resolver.resolve();
			}
			t.tree.insert(rule);
			t.rules.push_back(rule);
			rule_to_tree[rule] = &t;
			if (t.rules.size() > 1) {
				update_dimension_order(t);
			}
			tree_cnt++;
			resort_on_priority_change(tree_cnt - 1);
			//std::cout << "tree_cnt = " << tree_cnt << std::endl;
		} else {
			throw std::runtime_error(
					"all tress used (need to implement tree merging)");
		}
	}

	/*
	 * Remove the rule if it is stored in classifier
	 * */
	inline void remove(const rule_spec_t & rule) {
		auto ti = rule_to_tree.find(rule);
		if (ti != rule_to_tree.end()) {
			ti->second->tree.remove(rule);
			rule_to_tree.erase(ti);
			update_dimension_order(*ti->second);
		}
	}
	inline rule_id_t search(const val_vec_t & val) const {
		rule_id_t actual_found = TREE_T::INVALID_RULE;

		for (size_t i = 0; i < tree_cnt; i++) {
			auto & t = *trees[i];
			actual_found = t.tree.search(val);
			if (actual_found != TREE_T::INVALID_RULE and i + 1 < tree_cnt
					and trees[i + 1]->max_priority < actual_found) {
				break;
			}
		}

		return actual_found;
	}
	~PartitionSortClassifer() {
		for (auto ti : trees)
			delete ti;
	}
};

}
