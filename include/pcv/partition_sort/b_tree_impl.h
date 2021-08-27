#pragma once

#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/b_tree_cfg.h>
#include <pcv/partition_sort/b_tree_collision_check.h>
#include <pcv/partition_sort/b_tree_insert.h>
#include <pcv/partition_sort/b_tree_remove.h>
#include <pcv/partition_sort/b_tree_printer.h>
#include <pcv/partition_sort/b_tree_to_rules.h>
#include <pcv/partition_sort/dynamic_mempool.h>
#include <sstream>

namespace pcv {

/*
 * Wrapper around _BTree which connects the algorithms and data part to a single class
 *
 * @tparam cfg the class derived from pcv::_BTreeCfg which has parameters
 *             for specification of maximum sizes and other alg. configs
 * */
template<typename cfg>
class BTreeImp: public _BTree<cfg> {
public:
	using BTree = _BTree<cfg>;
	using Insert_t = BTreeInsert<cfg>;
	using CollisionCheck_t = BTreeCollisionCheck<cfg>;
	using Search_t = BTreeSearch<cfg>;
	using Remove_t = BTreeRemove<cfg>;
	using ToRules = _BTreeToRules<cfg>;

	static constexpr size_t D = BTree::D;
	using Node = typename BTree::Node;
	using rule_id_t = typename BTree::rule_id_t;
	using key_range_t = typename BTree::key_range_t;
	using rule_spec_t = typename BTree::rule_spec_t;
	using NodeAllocator = typename BTree::NodeAllocator;
	using key_t = typename BTree::key_t;
	using index_t = typename BTree::index_t;
	using KeyInfo = typename BTree::KeyInfo;
	using key_vec_t = typename BTree::key_vec_t;
	using formaters_t = std::array< std::function<void(std::ostream & str, key_range_t val)>, D>;
	using names_t = std::array<std::string, D>;
	using priority_t = typename BTree::priority_t;
	using rule_value_t = typename BTree::rule_value_t;
	using packet_spec_t = typename Search_t::packet_spec_t;
	using Printer_t = BTreePrinter<cfg, formaters_t, names_t>;

	static constexpr index_t INVALID_INDEX = BTree::INVALID_INDEX;
	static constexpr rule_id_t INVALID_RULE = BTree::INVALID_RULE;
	// print functions and key names for the debug

	const formaters_t formaters;
	const names_t names;
	Search_t searcher;

	BTreeImp(NodeAllocator & node_allocator) :
			BTree(node_allocator), formaters(_default_formaters()), names(_default_names()), searcher(
					*reinterpret_cast<const BTree*>(this)) {
	}
	BTreeImp(NodeAllocator & node_allocator, const formaters_t & _formaters, const names_t & _names) :
			BTree(node_allocator), formaters(_formaters), names(_names), searcher(
					*reinterpret_cast<const BTree*>(this)) {
	}
	BTreeImp(NodeAllocator & node_allocator, const packet_spec_t & in_packet_pos, const formaters_t & _formaters,
			const names_t & _names) :
			BTree(node_allocator), formaters(_formaters), names(_names), searcher(
					*reinterpret_cast<const BTree*>(this), in_packet_pos) {
	}
	inline void insert(const rule_spec_t & r) {
		Insert_t::insert(*reinterpret_cast<BTree*>(this), r);
	}
	inline bool does_rule_colide(const rule_spec_t & r) {
		return CollisionCheck_t::does_rule_colide(
				*reinterpret_cast<BTree*>(this), r);
	}
	template<typename search_val_t>
	inline rule_value_t search(search_val_t v) const {
		return searcher.search(v);
	}
	inline void remove(const rule_spec_t & r) {
		Remove_t::remove(*this, r);
	}

	// serialize graph to string in dot format
	friend std::ostream & operator<<(std::ostream & str, const BTreeImp & t) {
		Printer_t p(t.formaters, t.names);
		return p.print_top(str, t);
	}
	operator std::string() const {
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}

	static formaters_t _default_formaters() {
		formaters_t f;
		std::fill(f.begin(), f.end(),
				rule_vec_format::rule_vec_format_default<key_t>);
		return f;
	}
	static names_t _default_names() {
		names_t names;
		for (size_t i = 0; i < D; i++) {
			names[i] = std::to_string(i);
		}

		return names;
	}
};

}
