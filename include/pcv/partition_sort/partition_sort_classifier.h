#pragma once
#include <array>
#include <vector>
#include <unordered_map>
#include <assert.h>

#include <pcv/partition_sort/dimension_order_resolver.h>
#include <pcv/common/array_hasher.h>
#include <pcv/common/rulespec_hasher.h>
#include <pcv/common/array_resorter.h>

namespace pcv {

/*
 * Partitionsort like packet classifier classifier with configurable
 * tree type
 *
 * @tparam TREE_T the type of tree used for classification
 * @tparam MAX_TREE_CNT maximal number of tree structures used for classification
 * @tparam TREE_FIXATION_THRESHOLD if this threshold is exceeded the dimension order is locked for this tree
 *	 	and is not checked for optimality
 *
 **/
template<typename TREE_T, const size_t MAX_TREE_CNT,
		size_t TREE_FIXATION_THRESHOLD = 10>
class PartitionSortClassifer {
public:
	using Tree = TREE_T;
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
	using rule_mask_t = key_vec_t;
	static constexpr size_t D = TREE_T::D;

	static constexpr index_t INVALID_INDEX = TREE_T::INVALID_INDEX;
	static constexpr rule_id_t INVALID_RULE = TREE_T::INVALID_RULE;

	/*
	 *  Container of an information about a single classifier tree used in this classifier
	 *
	 *  @note the trees are sorted [0] has the greatest max priority rule
	 */
	struct tree_info {
		// The tree for classification
		TREE_T tree;
		// maximum value of rule priority for rules sorted in this classification tree
		priority_t max_priority;
		// number of fields used during classification, the number is used to limit the number of combinations
		// for field order resolution algorithm, and this variable is used to cache this value in order to avoid
		// recomputation before each update
		unsigned used_dim_cnt;
		// rules stored in classifier tree in their original form, (used to prevent costly tree<->rule list conversions)
		std::vector<rule_spec_t> rules;
		// flag which is used to detect cycles during rule inserts
		bool update_pending;

		std::unordered_map<rule_mask_t, std::size_t, array_hasher<rule_mask_t>,
				array_eq<rule_mask_t>> used_rule_masks;

		tree_info() :
				tree(), max_priority(0), used_dim_cnt(0), update_pending(false) {
		}
		tree_info(const formaters_t &_formaters, const names_t &_names) :
				tree(_formaters, _names), max_priority(0), used_dim_cnt(0), update_pending(
						false) {
		}
		tree_info(const packet_spec_t &in_packet_pos,
				const formaters_t &_formaters, const names_t &_names) :
				tree(in_packet_pos, _formaters, _names), max_priority(0), used_dim_cnt(
						0), update_pending(false) {
		}
	};
	// The trees used for classification itself
	std::array<tree_info*, MAX_TREE_CNT> trees;
	// number of trees actively used (rest of trees are just preallocated and does not contain any rules nor are used)
	size_t tree_cnt;

	// used to keep track of where are the rules stored for removing;
	std::unordered_map<rule_spec_t, tree_info*, rule_spec_t_hasher<rule_spec_t>,
			rule_spec_t_eq<rule_spec_t>> rule_to_tree;

	PartitionSortClassifer() :
			tree_cnt(0) {
		for (size_t i = 0; i < MAX_TREE_CNT; i++) {
			trees[i] = new tree_info;
		}
	}
	PartitionSortClassifer(const formaters_t &_formaters, const names_t &_names) :
			tree_cnt(0) {
		for (size_t i = 0; i < MAX_TREE_CNT; i++) {
			trees[i] = new tree_info(_formaters, _names);
		}
	}
	PartitionSortClassifer(const packet_spec_t &in_packet_pos,
			const formaters_t &_formaters, const names_t &_names) :
			tree_cnt(0) {
		for (size_t i = 0; i < MAX_TREE_CNT; i++) {
			trees[i] = new tree_info(in_packet_pos, _formaters, _names);
		}
	}

	/*
	 * Try to find better order of the fields used during tree construction,
	 * reconstruct tree if required,
	 * resort all tress by max rule priority if required
	 * */
	// @return true if the dimension order changed
	void update_dimension_order(tree_info &ti,
			const rule_spec_t *rule_to_add_before = nullptr) {
		TREE_T &tree = ti.tree;
		if (ti.update_pending) {
			assert(not tree.does_rule_colide(*rule_to_add_before));
			// @note it has to be checked before that the insertion is possible
			if (rule_to_add_before) {
				tree.insert(*rule_to_add_before);
				ti.rules.push_back(*rule_to_add_before);
			}
			return;
		}
		ti.update_pending = true;
		if (rule_to_add_before)
			ti.rules.push_back(*rule_to_add_before);

		GreedyDimensionOrderResolver<rule_spec_t, TREE_T::D, key_t> resolver(
				ti.rules, tree.dimension_order);
		auto new_dim_order = resolver.resolve();
		if (tree.dimension_order != new_dim_order.first) {
			// rebuild the tree because it has sub-optimal dimension order
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
		} else if (rule_to_add_before) {
			tree.insert(*rule_to_add_before);
		}
		ti.update_pending = false;
		// else no change is required as the dimension order is optimal (or nearly optimal)
	}

	struct max_priority_getter {
		priority_t operator()(const tree_info *t) const {
			return t->max_priority;
		}
	};
	/*
	 * Resort the trees if maximum priority of the tree has changed
	 * (sorting alg is stable = the order of equal items is not changed)
	 *
	 * @param original_i the index of the tree which priority has changed
	 * */
	void resort_on_priority_change(const size_t original_i) {
		assert(
				trees[original_i]->rules.size() > 0
						&& "in this case it is useless to call this function");
		array_resort<tree_info*, MAX_TREE_CNT, max_priority_getter>(trees,
				tree_cnt, original_i);
	}
	inline static void used_rule_mask_incr(tree_info &t, const rule_mask_t &m) {
		auto mi = t.used_rule_masks.find(m);
		if (mi == t.used_rule_masks.end()) {
			t.used_rule_masks[m] = 1;
		} else {
			mi->second += 1;
		}
	}
	inline static void used_rule_mask_decr(tree_info &t, const rule_mask_t &m) {
		auto mi = t.used_rule_masks.find(m);
		assert(mi != t.used_rule_masks.end());
		mi->second -= 1;
		if (mi->second == 0) {
			t.used_rule_masks.erase(mi);
		}
	}
	/*
	 * Insert rule to tree most suitable for this rule
	 * and sort the tress by the max priority rule in descending order
	 *
	 * @return true if insert was successful
	 * */
	inline bool try_insert_to_tree_without_collisions(const rule_spec_t &rule,
			const rule_mask_t &this_rule_mask, size_t tree_i) {
		// try to insert rule to a tree with best compatibility
		assert(tree_i < MAX_TREE_CNT);
		auto &t = *trees[tree_i];
		if (t.tree.does_rule_colide(rule))
			return false;

		t.rules.push_back(rule);
		rule_to_tree[rule] = &t;
		used_rule_mask_incr(t, this_rule_mask);

		if (t.rules.size() < TREE_FIXATION_THRESHOLD) {
			update_dimension_order(t, &rule);
		} else {
			if (t.used_dim_cnt < D) {
				// if new dimension is used in rule which was not used in tree previously
				// it is required to update dimension order to put the new dimension,
				// being previously used, in order to prevent sparse levels in tree.
				for (size_t d_i = t.used_dim_cnt; d_i < D; d_i++) {
					// for all unused dimensions check if they are used in new rule
					// if it is used swap it with first unused
					auto d = t.tree.dimension_order[d_i];
					if (not rule.first[d].is_wildcard()) {
						std::swap(t.tree.dimension_order[t.used_dim_cnt],
								t.tree.dimension_order[d_i]);
						t.used_dim_cnt++;
					}
				}
			}
			assert(not t.tree.does_rule_colide(rule));
			t.tree.insert(rule);
		}

		if (rule.second.priority > t.max_priority) {
			t.max_priority = rule.second.priority;
			resort_on_priority_change(tree_i);
		}

		return true;
	}
	/*
	 * Insert rule to tree most suitable for this rule
	 * and sort the tress by the max priority rule in descending order
	 * */
	inline void insert(const rule_spec_t &rule) {
#ifndef NDEBUG
		for (auto r : rule.first) {
			assert(r.low <= r.high);
		}
#endif
		std::array<bool, MAX_TREE_CNT> insert_tried;
		std::fill(insert_tried.begin(), insert_tried.end(), false);
		// try to insert in to trees which already contains rules with same mask

		rule_mask_t this_rule_mask = TREE_T::rule_value_t::get_rule_mask(rule);
		for (size_t i = 0; i < tree_cnt; i++) {
			auto &t = *trees[i];
			auto &used_masks = t.used_rule_masks;
			if (used_masks.find(this_rule_mask) != used_masks.end()) {
				if (try_insert_to_tree_without_collisions(rule, this_rule_mask,
						i)) {
					return;
				}
				insert_tried[i] = true;
			}
		}
		// try to insert to rest of the treees
		for (size_t i = 0; i < tree_cnt; i++) {
			if (!insert_tried[i]) {
				try_insert_to_tree_without_collisions(rule, this_rule_mask, i);
				return;
			}
		}

		// else create new tree for this rule
		if (tree_cnt < MAX_TREE_CNT) {
			size_t i = tree_cnt;
			// the rule does not fit to any tree, generate new tree for this rule
			auto &t = *trees[i];
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
			used_rule_mask_incr(t, this_rule_mask);
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
		//if (tree_cnt > used_rule_masks.size()) {
		//	// the rules are stored in classifier in efficiently
		//	// if we store each group of rules with same mask
		//	// to a separate tree we can achieve a lower number of trees
		//}
	}

	/*
	 * Remove the rule if it is stored in classifier
	 * */
	inline void remove(const rule_spec_t &rule) {
		auto ti = rule_to_tree.find(rule);
		if (ti != rule_to_tree.end()) {
			ti->second->tree.remove(rule);
			rule_to_tree.erase(ti);
			rule_mask_t rule_mask = TREE_T::rule_value_t::get_rule_mask(rule);
			used_rule_mask_decr(*ti->second, rule_mask);

			if (ti->second->rules.size() < TREE_FIXATION_THRESHOLD)
				update_dimension_order(*ti->second);
		}
	}
	template<typename search_val_t>
	inline rule_value_t search(search_val_t val) const {
		rule_value_t actual_found;

		for (size_t i = 0; i < tree_cnt; i++) {
			auto &t = *trees[i];
			auto res = t.tree.search(val);
			if (res.is_valid()
					&& (!actual_found.is_valid()
							|| actual_found.priority < res.priority)) {
				actual_found = res;
			}
			// check if we can find rule with higher priority in some next tree
			unsigned next_tree_i = i + 1;
			if (actual_found.is_valid()
					and (next_tree_i >= tree_cnt
							or trees[next_tree_i]->max_priority
									<= actual_found.priority)) {
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
