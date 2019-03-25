#pragma once
#include <iostream>
#include <immintrin.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <stdio.h>
#include <array>
#include <limits>
#include <vector>
#include <set>
#include <tuple>

#include <pcv/common/range.h>
//#include <pcv/partiton_sort/mempool_mockup.h>
#include <pcv/partiton_sort/mempool.h>
#include <pcv/partiton_sort/key_info.h>

namespace pcv {

/*
 * The B-Tree with multidimensional key
 *
 * This is B-tree divide to several layers. Each layer performs the lookup in single dimension only.
 * */
class alignas(64) BTree {
public:
	using rule_id_t = uint16_t;
	static constexpr size_t D = 2;
	using rule_spec_t = std::pair<std::array<Range1d<uint32_t>, D>, rule_id_t>;
	using value_t = uint32_t;
	using index_t = uint16_t;

	using KeyInfo = _KeyInfo<uint32_t, BTree::index_t, BTree::value_t>;

	static const index_t INVALID_INDEX;
	static const rule_id_t INVALID_RULE;

	std::array<int, D> dimension_order;

	class alignas(64) Node: public ObjectWithStaticMempool<Node, 65536, false> {
	public:
		static constexpr size_t T = 4;
		static constexpr size_t MIN_DEGREE = T - 1;
		static constexpr size_t MAX_DEGREE = 2 * T - 1;
		// perf-critical: ensure this is 64-byte aligned.
		// 8*4B values
		__m256i keys[2];
		// 8*1B dimension index
		__m64 dim_index;
		// the value of rule
		std::array<index_t, MAX_DEGREE> value;
		std::array<index_t, MAX_DEGREE> next_level;
		// 9*2B child index
		std::array<index_t, MAX_DEGREE + 1> child_index;
		// if bit in mask is set the key is present
		uint32_t key_mask;
		uint8_t key_cnt;

		bool is_leaf;
		Node * parent;

		class KeyIterator {
		public:
			class State {
			public:
				Node * actual;
				unsigned index;

				bool operator!=(const State & other) const {
					return actual != other.actual or index != other.index;
				}

				void operator++() {
					std::tie(actual, index) = actual->getSucc_global(index);
				}

				KeyInfo operator*() {
					return actual->get_key<value_t>(index);
				}

			};

			State _end;
			State _begin;

			KeyIterator(Node * root) {
				_begin.actual = root;
				_begin.index = 0;
				while (_begin.actual->child(0)) {
					_begin.actual = _begin.actual->child(0);
				}
				_end.actual = nullptr;
				_end.index = 0;
			}

			constexpr State & end() {
				return _end;
			}

			constexpr State & begin() {
				return _begin;
			}
		};

		Node(Node const&) = delete;
		Node& operator=(Node const&) = delete;
		Node();
		void clean_children();

		template<typename T>
		KeyInfo get_key(uint8_t index) const {
			assert(sizeof(T) == sizeof(uint32_t));
			assert(index < MAX_DEGREE);
			auto low = reinterpret_cast<const uint32_t*>(&keys[0])[index];
			auto high = reinterpret_cast<const uint32_t*>(&keys[1])[index];
			return {
				Range1d<T>(low, high),
				value[index],
				next_level[index]
			};
		}

		template<typename T>
		void set_key(uint8_t index, KeyInfo key_info) {
			assert(sizeof(T) == sizeof(uint32_t));
			assert(index < MAX_DEGREE);
			reinterpret_cast<uint32_t*>(&keys[0])[index] = key_info.key.low;
			reinterpret_cast<uint32_t*>(&keys[1])[index] = key_info.key.high;

			this->value[index] = key_info.value;
			this->next_level[index] = key_info.next_level;
		}

		/*
		 * Set pointer to child node
		 * */
		void set_child(unsigned index, Node * child);
		/*
		 * Set pointer to next layer
		 * */
		void set_next_layer(unsigned index, Node * next_layer_root);

		/* A utility function to split the child y of this node
		 * Note that y must be full when this function is called
		 * @param i index of key which should be transfered to this node
		 * @param y the child node
		 **/
		void splitChild(unsigned i, Node & y);

		/*
		 * Set key_cnt and also update key_mask
		 * */
		void set_key_cnt(size_t key_cnt);

		/*
		 * Information about insert state
		 * */
		class InsertCookie {
		public:
			std::array<int, D> & dimensio_order;
			uint8_t level;

			InsertCookie(BTree & tree);
			Range1d<value_t> get_actual_key(const rule_spec_t & rule) const;
			bool required_more_levels(const rule_spec_t & rule) const;
		};

		/* A utility function to insert a new key in this node
		 * The assumption is, the node must be non-full when this
		 * function is called
		 * */
		void insertNonFull(const rule_spec_t & rule, InsertCookie & cookie);
		/*
		 * @note the rule must not collide with anything in the tree
		 * @param root root of tree where the rule should be inserted (can be nullptr)
		 * @param rule rule to insert
		 * @param cookie which sotores the state of insertion
		 * @return new root of the tree
		 * */
		static Node * insert_to_root(Node * root, const rule_spec_t & rule,
				InsertCookie & cookie);

		// get node from mempool from its index
		static inline Node & by_index(const index_t index) {
			return *reinterpret_cast<Node*>(BTree::Node::_Mempool_t::getById(
					index));
		}

		// get child node on specified index
		Node * child(const index_t index);
		const Node * child(const index_t index) const;

		// get root node of the next layer starting from this node on specified index
		Node * get_next_layer(unsigned index);
		const Node * get_next_layer(unsigned index) const;
		// Find index of the key in this node
		unsigned findKey(const Range1d<value_t> k);
		/* A wrapper function to remove the key k in subtree rooted with
		 * this node.
		 * */
		void remove(Range1d<value_t> k);
		/* A function to remove the key present in idx-th position in
		 * this node which is a leaf
		 * */
		void removeFromLeaf(unsigned idx);

		/**
		 * A function to remove the key present in idx-th position in
		 * this node which is a non-leaf node
		 *
		 * @param idx index of the key to remove
		 */
		void removeFromNonLeaf(unsigned idx);
		/* A function to borrow a key from child(idx-1) and insert it
		 * into child(idx)
		 * */
		void borrowFromPrev(unsigned idx);
		// A function to borrow a key from the child(idx+1) and place
		// it in child(idx)
		void borrowFromNext(unsigned idx);
		// A function to merge child(idx) with child(idx+1)
		// child(idx+1) is freed after merging
		void merge(unsigned idx);
		// A function to fill child child(idx) which has less than MIN_DEGREE-1 keys
		void fill(unsigned idx);

		KeyIterator iter_keys();
		// A function to get position of successor of keys[idx]
		// @return next node and next index of key in it
		// 		if there is no key after this one returns {nullptr, 0};
		std::pair<Node*, unsigned> getSucc_global(unsigned idx);

		// A function to get predecessor of keys[idx] (only in the local subtree)
		KeyInfo getPred(unsigned idx);
		// A function to get successor of keys[idx] (only in the local subtree)
		KeyInfo getSucc(unsigned idx);

		size_t size() const;
		~Node();
	}__attribute__((aligned(64)));

	Node * root;

	BTree(BTree const&) = delete;
	BTree& operator=(BTree const&) = delete;
	BTree();

	/*
	 * Search in onde level of the tree
	 *
	 * @return rule index or the INVALID_RULE constant
	 * */
	rule_id_t search(const value_t & val);

	/*
	 * Search in all levels of the tree
	 *
	 * [TODO] use array
	 * */
	rule_id_t search(const std::vector<value_t> & val);

	/*
	 * Search the tuples <node, index in node> for each part of the rule
	 * @note the items are in order specified by dimension_order array
	 **/
	void search_path(const rule_spec_t & rule,
			std::vector<std::pair<Node *, unsigned>> & path);

	/*
	 * Check if the rule collides wit any other rule in the tree
	 *
	 * @note the rule however can overwrite other rule or can use its prefix without the collision
	 * */
	bool does_rule_colide(const rule_spec_t & rule);

	/*
	 * @attention The range can be removed only if noting depends on it in next level
	 * */
	Node * remove_1d(const Range1d<value_t> & k, Node * current_root);
	/*
	 * A wrapper function to remove the key k in subtree rooted with
	 * this node.
	 */
	void remove(const rule_spec_t & k);

	inline void insert(const rule_spec_t & rule) {
		Node::InsertCookie cookie(*this);
		insert(rule, cookie);
	}

	/*
	 * if insert fails there is last node stored in cookie
	 * for reinsert
	 * */
	void insert(const rule_spec_t & rule, Node::InsertCookie & cookie);

	/*
	 * If the searched value is in range it means that the search in B-tree is finished
	 * and the val_index is index of item in this node where the search ended
	 * Otherwise the val_index is index of the children node which should be searched
	 **/
	class SearchResult {
	public:
		unsigned val_index;
		bool in_range;
		SearchResult() :
				val_index(-1), in_range(false) {
		}
	};

	// get number of keys stored on all levels in tree (!= number of stored rules)
	size_t size() const;

	void print_to_stream(std::ostream & str, const Node & n) const;

	// serialize graph to string in dot format
	friend std::ostream & operator<<(std::ostream & str, const BTree & t);
	operator std::string() const;

	~BTree();

}__attribute__((aligned(64)));

/*
 * Move key, value and next level pointer between two places
 * */
template<typename KEY_t>
inline void move_key(BTree::Node & src, uint8_t src_i, BTree::Node & dst,
		uint8_t dst_i) {
	dst.set_key<KEY_t>(dst_i, src.get_key<KEY_t>(src_i));
}

/*
 * Copy keys, child pointers etc between the nodes
 * */
template<typename KEY_t>
inline void transfer_items(BTree::Node & src, unsigned src_start,
		BTree::Node & dst, unsigned dst_start, unsigned key_cnt) {
	// Copying the keys from
	auto end = src_start + key_cnt;
	for (unsigned i = src_start; i < end; ++i)
		move_key<KEY_t>(src, i, dst, i + dst_start);

	// Copying the child pointers
	if (not dst.is_leaf) {
		for (unsigned i = src_start; i <= end; ++i) {
			auto ch = src.child(i);
			dst.set_child(i + dst_start, ch);
			//dst.child_index[i + dst_start] = src.child_index[i];
		}
	}
}

/*
 * Shift block of keys to position -1
 * */
template<typename KEY_t>
inline void shift_items_on_right_to_left(BTree::Node & n, uint8_t start) {
	for (unsigned i = start + 1; i < n.key_cnt; ++i)
		move_key<uint32_t>(n, i, n, i - 1);

	for (unsigned i = start + 2; i <= n.key_cnt; ++i) {
		n.child_index[i - 1] = n.child_index[i];
	}
}

/*
 * Check if the pointers in node are valid (recursively)
 * */
static inline void integrity_check(BTree::Node & n,
		std::set<BTree::Node*> * seen = nullptr) {
	std::set<BTree::Node*> _seen;
	if (seen == nullptr)
		seen = &_seen;
	assert(seen->find(&n) == seen->end());
	assert(n.key_cnt > 0);

	for (size_t i = 0; i < n.key_cnt; i++) {
		n.get_key<uint32_t>(i);
		auto nl = n.get_next_layer(i);
		if (nl) {
			assert(nl->parent == nullptr);
			integrity_check(*nl, seen);
		}
	}
	for (size_t i = 0; i <= n.key_cnt; i++) {
		auto ch = n.child(i);
		if (ch) {
			assert(ch->parent == &n);
			integrity_check(*ch, seen);
		}
	}

}

}
