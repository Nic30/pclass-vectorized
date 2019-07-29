#pragma once
#include <iostream>
#include <immintrin.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <limits>
#include <tuple>
#include <functional>
#include <type_traits>

#ifndef NDEBUG
#include <cstring>
#endif

#include <pcv/common/range.h>
#include <pcv/partition_sort/key_info.h>
#include <pcv/partition_sort/b_tree_node.h>
#include <pcv/rule_parser/rule.h>

namespace pcv {


struct IntRuleValue {
	// the identifier of the rule store in tree
	using rule_id_t = uint32_t;
	using priority_t = uint32_t;

	static constexpr rule_id_t INVALID_RULE = (1 << 24) - 1;

	// invalid rule = rule with rule_id = INVALID_RULE and should have priority set to 0
	priority_t priority :8;
	rule_id_t rule_id :24;

	IntRuleValue() :
			priority(0), rule_id(INVALID_RULE) {
	}
	IntRuleValue(priority_t priority_, rule_id_t rule_id_) :
			priority(priority_), rule_id(rule_id_) {
	}
	bool is_valid() const {
		return rule_id != INVALID_RULE;
	}
	bool operator==(const IntRuleValue &other) const {
		return priority == other.priority && rule_id == other.rule_id;
	}
	bool operator!=(const IntRuleValue &other) const {
		return priority != other.priority || rule_id != other.rule_id;
	}
};

/*
 * The B-Tree with multidimensional key
 *
 * This is B-tree divide to several layers. Each layer performs the lookup in single dimension only.
 *
 * This tree uses path compression between the layers. This means if there is a path trough the tree which contains
 * only the nodes with a single items. All items on this paths are merged in to the single node with is_compressed flag set.
 * In order to reduce lookup speed an use SIMD efficiently. Note that the compressed node does not behaves like
 * b-tree node but just as a list of the ranges.
 *
 * @note that this is only implementation of the data structure. You can find the algorithms in b_tree_*.h files.
 *
 * @tparam cfg the class derived from pcv::_BTreeCfg which has parameters
 *             for specification of maximum sizes and other alg. configs
 * */
template<typename cfg>
class alignas(64) _BTree {
public:
	static constexpr size_t D = cfg::D;
	// range which is a key
	using key_t = typename cfg::Key_t;
	using key_range_t = Range1d<key_t>;
	// the type of index of the dimension in input vector or rule (0 to D-1)
	using level_t = unsigned;

	using rule_value_t = typename cfg::Value_t;
	using rule_id_t = typename rule_value_t::rule_id_t;
	using priority_t = typename rule_value_t::priority_t;
	static constexpr rule_id_t INVALID_RULE = rule_value_t::INVALID_RULE;
	static constexpr bool PATH_COMPRESSION = cfg::PATH_COMPRESSION;
	static constexpr size_t MAX_NODE_CNT = cfg::MAX_NODE_CNT;

	// index of the node in memorypool
	using index_t = typename std::conditional<cfg::MAX_NODE_CNT < std::numeric_limits<uint16_t>::max(), uint16_t, uint32_t>::type;
	static_assert(cfg::MAX_NODE_CNT < std::numeric_limits<uint32_t>::max());

	// util type which keeps informations about the key and child/next layer pointers in for item in node
	using KeyInfo = _KeyInfo<key_t, index_t, rule_value_t>;
	// type of value vector which can be searched in this data structure
	using key_vec_t = std::array<key_t, D>;

	// specification of the rule for insert/remove ops
	using rule_spec_t = std::pair<std::array<key_range_t, D>, rule_value_t>;
	using Node = _BTreeNode<cfg, level_t, rule_value_t, index_t, KeyInfo, key_range_t>;
	static constexpr index_t INVALID_INDEX = Node::INVALID_INDEX;

	Node *root;
	std::array<level_t, D> dimension_order;

	// the copy constructor is disabled in order to ensure there are not any unintended copies of this object
	_BTree(_BTree const&) = delete;
	_BTree& operator=(_BTree const&) = delete;
	// enable move constructor so it is possible to explicitly move the tree
	_BTree(_BTree &&o) noexcept :
			root(std::move(o.root)), dimension_order(
					std::move(o.dimension_order)) {
	}
	_BTree& operator=(_BTree &&o) noexcept {
		root = std::move(o.root);
		dimension_order = std::move(o.dimension_order);
		return *this;
	}

	_BTree() :
			root(nullptr) {
		for (size_t i = 0; i < dimension_order.size(); i++)
			dimension_order[i] = i;
	}

	// get number of keys stored on all levels in tree (!= number of stored rules)
	size_t size() const {
		if (root)
			return root->size();
		else
			return 0;
	}

	~_BTree() {
		delete root;
		root = nullptr;
	}

	static_assert(sizeof(Node) % 64 == 0);

}__attribute__((aligned(64)));

}
