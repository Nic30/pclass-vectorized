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
	using KeyInfo = _KeyInfo<BTree::value_t, BTree::index_t>;

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

		/*
		 * Set key_cnt and also update key_mask
		 * */
		void set_key_cnt(size_t key_cnt);

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

		size_t size() const;
		~Node();
	}__attribute__((aligned(64)));

	Node * root;

	BTree(BTree const&) = delete;
	BTree& operator=(BTree const&) = delete;
	BTree();

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
