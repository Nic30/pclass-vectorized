#pragma once

#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/b_tree_collision_check.h>
#include <pcv/partition_sort/b_tree_insert.h>
#include <pcv/partition_sort/b_tree_remove.h>
#include <pcv/partition_sort/b_tree_printer.h>
#include <pcv/partition_sort/b_tree_to_rules.h>

namespace pcv {

/*
 * Wrapper around _BTree which connects the algorithms and data part to a single class
 *
 * @tparam Key_t the type of key which will be used by the nodes in the tree
 * @tparam _D maximal number of levels of the tree (number of fields in packet/dimensions)
 * @tparam _T parameter which specifies the number of items per node
 * */
template<typename Key_t, size_t _D, size_t _T = 4, bool _PATH_COMPRESSION = true>
class BTreeImp: public _BTree<Key_t, _D, _T, _PATH_COMPRESSION> {
public:
	using BTree = _BTree<Key_t, _D, _T, _PATH_COMPRESSION>;
	using Insert_t = BTreeInsert<Key_t, _D, _T, _PATH_COMPRESSION>;
	using CollisionCheck_t = BTreeCollisionCheck<Key_t, _D, _T, _PATH_COMPRESSION>;
	using Search_t = BTreeSearch<Key_t, _D, _T, _PATH_COMPRESSION>;
	using Remove_t = BTreeRemove<Key_t, _D, _T, _PATH_COMPRESSION>;
	using ToRules = _BTreeToRules<Key_t, _D, _T, _PATH_COMPRESSION>;

	using Node = typename BTree::Node;
	using rule_id_t = typename BTree::rule_id_t;
	static constexpr size_t D = BTree::D;
	using key_range_t = typename BTree::key_range_t;
	using rule_spec_t = typename BTree::rule_spec_t;
	using key_t = typename BTree::key_t;
	using index_t = typename BTree::index_t;
	using KeyInfo = typename BTree::KeyInfo;
	using val_vec_t = typename BTree::val_vec_t;
	using formaters_t = std::array< std::function<void(std::ostream & str, key_range_t val)>, D>;
	using names_t = std::array<std::string, D>;

	using Printer_t = BTreePrinter<Key_t, _D, _T, _PATH_COMPRESSION, formaters_t, names_t>;

	static constexpr index_t INVALID_INDEX = BTree::INVALID_INDEX;
	static constexpr rule_id_t INVALID_RULE = BTree::INVALID_RULE;
	// print functions and key names for the debug

	const formaters_t formaters;
	const names_t names;

	BTreeImp() :
		BTree(), formaters(_default_formaters()), names(_default_names()) {
	}

	BTreeImp(const formaters_t & _formaters, const names_t & _names) :
		BTree(), formaters(_formaters), names(_names) {
	}

	inline void insert(const rule_spec_t & r) {
		Insert_t::insert(*reinterpret_cast<BTree*>(this), r);
	}

	inline bool does_rule_colide(const rule_spec_t & r) {
		return CollisionCheck_t::does_rule_colide(
				*reinterpret_cast<BTree*>(this), r);
	}

	inline rule_id_t search(const val_vec_t & v) const {
		return Search_t::search(*reinterpret_cast<const BTree*>(this), v);
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
