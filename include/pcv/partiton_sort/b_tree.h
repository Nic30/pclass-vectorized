#pragma once
#include <iostream>
#include <immintrin.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <stdio.h>
#include <array>
#include <limits>
#include <vector>

#include <pcv/partiton_sort/mempool.h>
#include <pcv/partiton_sort/range.h>
#include <pcv/partiton_sort/key_info.h>

namespace pcv {
// https://stackoverflow.com/questions/24594026/initialize-m256i-from-64-high-or-low-bits-of-four-m128i-variables
// https://openproceedings.org/EDBT/2014/paper_107.pdf

/*
 * The B-Tree with multidimensional key
 *
 * This is B-tree divide to several layers. Each layer performs the lookup in single dimmension only.
 * */
class BTree {
public:
	using rule_id_t = uint16_t;
	static constexpr size_t D = 2;
	using rule_spec_t = std::pair<std::array<Range1d<uint32_t>, D>, rule_id_t>;
	using value_t = uint32_t;
	using index_t = uint16_t;

	using KeyInfo = _KeyInfo<uint32_t, BTree::index_t, BTree::value_t>;

	static const index_t INVALID_INDEX;
	static const rule_id_t INVALID_RULE;

	class Node: public ObjectWithStaticMempool<Node, 65536, false> {
	public:
		// perf-critical: ensure this is 64-byte aligned.
		// 8*4B values
		__m256i keys[2];
		// 8*1B dimension index
		__m64 dim_index;
		// the value of rule
		std::array<index_t, 8> value;
		std::array<index_t, 8> next_level;
		// 9*2B child index
		std::array<index_t, 9> child_index;
		// if bit in mask is set the key is present
		uint32_t key_mask;
		uint8_t key_cnt;

		bool is_leaf;
		uint16_t parent_index;

		static constexpr size_t MIN_DEGREE = 4;
		static constexpr size_t MAX_DEGREE = 8;

		Node() {
			keys[0] = keys[1] = _mm256_set1_epi32(
					std::numeric_limits<uint32_t>::max());
			dim_index = _m_from_int64(std::numeric_limits<uint64_t>::max());
			std::fill(value.begin(), value.end(), INVALID_INDEX);
			std::fill(next_level.begin(), next_level.end(), INVALID_INDEX);
			std::fill(child_index.begin(), child_index.end(), INVALID_INDEX);
			set_key_cnt(0);
			is_leaf = true;
			parent_index = INVALID_INDEX;
		}

		template<typename T>
		KeyInfo get_key(uint8_t index) const {
			assert(sizeof(T) == sizeof(uint32_t));
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
			reinterpret_cast<uint32_t*>(&keys[0])[index] = key_info.key.low;
			reinterpret_cast<uint32_t*>(&keys[1])[index] = key_info.key.high;

			this->value[index] = key_info.value;
			this->next_level[index] = key_info.next_level;
		}

		void set_child(unsigned index, Node * child);
		void set_next_layer(unsigned index, Node * next_layer_root);

		/* A utility function to split the child y of this node
		 * Note that y must be full when this function is called
		 * @param i index of key which should be transfered to this node
		 * @param y the child node
		 */
		void splitChild(int i, Node & y);

		void set_key_cnt(size_t key_cnt);
		/* A utility function to insert a new key in this node
		 * The assumption is, the node must be non-full when this
		 * function is called
		 */
		void insertNonFull(const rule_spec_t & rule);

		static Node & by_index(const index_t index);
		Node & child(const index_t index);
		Node * get_next_layer(unsigned index);
		/* Find index of the key in this node
		 * */
		int findKey(const Range1d<value_t> k);
		// A wrapper function to remove the key k in subtree rooted with
		// this node.
		void remove(Range1d<uint32_t> k, int lvl, const rule_spec_t & _k);
		// A function to remove the key present in idx-th position in
		// this node which is a leaf
		void removeFromLeaf(unsigned idx);

		/**
		 * A function to remove the key present in idx-th position in
		 * this node which is a non-leaf node
		 *
		 * @param idx index of the key to remove
		 * @param lvl actual level of the tree (to obtain correct key from the rule_spec vector)
		 * @param _k full key vector which is removed
		 */
		void removeFromNonLeaf(unsigned idx, unsigned lvl, const rule_spec_t & _k);
		// A function to borrow a key from child(idx-1) and insert it
		// into child(idx)
		void borrowFromPrev(int idx);
		// A function to borrow a key from the child(idx+1) and place
		// it in child(idx)
		void borrowFromNext(int idx);
		// A function to merge child(idx) with child(idx+1)
		// child(idx+1) is freed after merging
		void merge(int idx);
		// A function to fill child child(idx) which has less than t-1 keys
		void fill(int idx);

		// A function to get predecessor of keys[idx]
		KeyInfo getPred(unsigned idx);
		// A function to get successor of keys[idx]
		KeyInfo getSucc(unsigned idx);


		~Node();
	};

	Node * root;

	BTree() :
			root(nullptr) {
	}

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

	// A wrapper function to remove the key k in subtree rooted with
	// this node.
	void remove(const rule_spec_t & k);
	void insert(const rule_spec_t & rule);

	/*
	 * If the searched value is in range it means that the search in B-tree is finished
	 * and the val_index is index of item in this node where the search ended
	 * Otherwise the val_index is index of the children node which should be searched
	 * */
	class SearchResult {
	public:
		unsigned val_index;
		bool in_range;
		SearchResult() :
				val_index(-1), in_range(false) {
		}
	};
	// returns from 0 (if value < keys[0]) to 16 (if value >= keys[15])
	static SearchResult search_avx2(const Node & node, value_t val);
	static SearchResult search_seq(const Node & node, value_t val);

	void print_to_stream(std::ostream & str, Node & n);

	// serialize graph to string in dot format
	friend std::ostream & operator<<(std::ostream & str, BTree & t);

	operator std::string();

	~BTree();
};

}
