#pragma once

#include "classifier.h"
#undef atomic_init
#undef atomic_store
#undef atomic_compare_exchange_strong_explicit
#undef atomic_compare_exchange_strong
#undef atomic_compare_exchange_weak_explicit
#undef atomic_compare_exchange_weak

#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/partition_sort_classifier.h>

struct RuleValue {
	// the identifier of the rule store in tree
	using rule_id_t = const struct cls_rule *;
	using priority_t = uint32_t;
	static constexpr rule_id_t INVALID_RULE = nullptr;

	// invalid rule = rule with rule_id = INVALID_RULE and should have priority set to 0
	rule_id_t rule_id;
	priority_t priority;

	RuleValue () :
			rule_id(INVALID_RULE), priority(0) {
	}
	RuleValue (priority_t priority_, rule_id_t rule_id_) :
		rule_id(rule_id_), priority(priority_) {
	}
	bool is_valid() const {
		return rule_id != INVALID_RULE;
	}
	bool operator==(const RuleValue  & other) const {
		return priority == other.priority && rule_id == other.rule_id;
	}
	bool operator!=(const RuleValue  & other) const {
		return priority != other.priority || rule_id != other.rule_id;
	}
};

constexpr size_t struct_flow_D = 329;
using BTree = pcv::BTreeImp<pcv::_BTreeCfg<uint16_t, RuleValue, struct_flow_D, (1<<20) - 1, 8, true>>;
using Classifier = pcv::PartitionSortClassifer<BTree, struct_flow_D, 10>;

/*
 * Structure to hide cpp classes for c
 * */
struct classifier_priv {
	Classifier cls;
	Classifier::rule_id_t next_rule_id;
	std::unordered_map<const struct cls_rule*, Classifier::rule_spec_t> to_pcv_rule;
	classifier_priv();
};
