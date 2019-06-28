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
		if (a.second.rule_id != b.second.rule_id
				|| a.second.priority != b.second.priority)
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
		hash_combine(seed, hash_value(k.second.priority));
		hash_combine(seed, hash_value(k.second.rule_id));

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
	using key_t = typename TREE_T::key_t;
	using Node = typename TREE_T::Node;
	using rule_spec_t = typename TREE_T::rule_spec_t;
	using index_t = typename TREE_T::index_t;
	using KeyInfo = typename TREE_T::KeyInfo;
	using rule_id_t= typename TREE_T::rule_id_t;
	using key_vec_t= typename TREE_T::key_vec_t;
	using formaters_t = typename TREE_T::formaters_t;
	using names_t = typename TREE_T::names_t;
	using priority_t = typename TREE_T::priority_t;
	using packet_spec_t = typename TREE_T::packet_spec_t;
	using Search_t = typename TREE_T::Search_t;
	using rule_value_t = typename TREE_T::rule_value_t;
	static constexpr size_t D = TREE_T::D;

	static constexpr index_t INVALID_INDEX = TREE_T::INVALID_INDEX;
	static constexpr rule_id_t INVALID_RULE = TREE_T::INVALID_RULE;

	// @note the trees are sorted [0] has the greatest max priority rule
	struct tree_info {
		TREE_T tree;
		priority_t max_priority;
		unsigned used_dim_cnt;
		std::vector<rule_spec_t> rules;

		tree_info() :
				tree(), max_priority(0), used_dim_cnt(0) {
		}
		tree_info(const formaters_t & _formaters, const names_t & _names) :
				tree(_formaters, _names), max_priority(0), used_dim_cnt(0) {

		}
		tree_info(const packet_spec_t & in_packet_pos,
				const formaters_t & _formaters, const names_t & _names) :
				tree(in_packet_pos, _formaters, _names), max_priority(0), used_dim_cnt(
						0) {
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
	PartitionSortClassifer(const packet_spec_t & in_packet_pos,
			const formaters_t & _formaters, const names_t & _names) :
			tree_cnt(0) {
		for (size_t i = 0; i < MAX_TREE_CNT; i++) {
			trees[i] = new tree_info(in_packet_pos, _formaters, _names);
		}
	}

	// @return true if the dimension order changed
	inline bool update_dimension_order(tree_info & ti) {
		TREE_T & tree = ti.tree;
		GreedyDimensionOrderResolver<rule_spec_t, TREE_T::D, key_t> resolver(
				ti.rules, tree.dimension_order);
		auto new_dim_order = resolver.resolve();
		if (tree.dimension_order != new_dim_order.first) {
			// rebuild the tree because it has suboptimal dimension order
			delete tree.root;
			tree.root = nullptr;
			tree.dimension_order = new_dim_order.first;
			ti.used_dim_cnt = new_dim_order.second;
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
		// try to insert rule to a tree with best compatibility
		size_t i = 0;
		for (; i < tree_cnt; i++) {
			auto & t = *trees[i];
			if (not t.tree.does_rule_colide(rule)) {
				t.rules.push_back(rule);
				rule_to_tree[rule] = &t;
				if (t.rules.size() < TREE_FIXATION_THRESHOLD) {
					if (update_dimension_order(t) == false) {
						assert(not t.tree.does_rule_colide(rule));
						t.tree.insert(rule);
					}
					// the rule is inserted automatically as it is in t.rules
				} else {
					if (t.used_dim_cnt < D) {
						// if new dimension is used in rule which was not used in tree previously
						// it is required to update dimension order to put the new dimension,
						// being previously used, in order to prevent sparse levels in tree.
						//for (size_t d_i = t.used_dim_cnt; d_i < D; d_i++) {
						//	// for all unused dimensions check if they are used in new rule
						//	// if it is used swap it with first unused
						//	auto d = t.tree.dimension_order[d_i];
						//	if (not rule.first[d].is_wildcard()) {
						//		std::swap(t.tree.dimension_order[t.used_dim_cnt],
						//				  t.tree.dimension_order[d_i]);
						//		t.used_dim_cnt++;
						//	}
						//}
					}
					assert(not t.tree.does_rule_colide(rule));
					t.tree.insert(rule);
				}
				if (rule.second.priority > t.max_priority) {
					t.max_priority = rule.second.priority;
					resort_on_priority_change(i);
				}
				return;
			}
		}
		// else create new tree for this rule
		if (i < MAX_TREE_CNT) {
			// the rule does not fit to any tree, generate new tree for this rule
			auto & t = *trees[i];
			t.max_priority = rule.second.priority;
			if (t.rules.size() == 0) {
				// update default dimension order to fit current rule
				// in order to avoid useless tree reconstruction
				std::vector<rule_spec_t> _rules = { rule, };
				GreedyDimensionOrderResolver<rule_spec_t, TREE_T::D, key_t> resolver(
						_rules, t.tree.dimension_order);
				auto dor = resolver.resolve();
				t.tree.dimension_order = dor.first;
				t.used_dim_cnt = dor.second;
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
	template<typename search_val_t>
	inline rule_value_t search(search_val_t val) const {
		rule_value_t actual_found = {0, TREE_T::INVALID_RULE};

		for (size_t i = 0; i < tree_cnt; i++) {
			auto & t = *trees[i];
			auto res = t.tree.search(val);
			if (res.is_valid() && (!actual_found.is_valid() || actual_found.priority < res.priority)) {
				actual_found = res;
			}
			if (actual_found.is_valid() and i + 1 < tree_cnt
					and trees[i + 1]->max_priority <= actual_found.priority) {
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
