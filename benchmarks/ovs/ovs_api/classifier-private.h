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
constexpr size_t struct_flow_D = 329;
using BTree = pcv::BTreeImp<uint16_t, struct_flow_D, 8, false>;
using Classifier = pcv::PartitionSortClassifer<BTree, struct_flow_D, 10>;

/*
 * Structure to hide cpp classes for c
 * */
struct classifier_priv {
	Classifier cls;
	Classifier::rule_id_t next_rule_id;
	std::unordered_map<Classifier::rule_id_t, const cls_rule *> to_rule;
	std::unordered_map<const struct cls_rule*, Classifier::rule_spec_t> to_pcv_rule;
	classifier_priv();
};
