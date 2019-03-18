#pragma once
#include <iostream>
#include <immintrin.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <stdio.h>
#include <array>
#include <limits>

#include "../partiton_sort/mempool.h"

// https://stackoverflow.com/questions/24594026/initialize-m256i-from-64-high-or-low-bits-of-four-m128i-variables
// https://openproceedings.org/EDBT/2014/paper_107.pdf
class BTree {
public:
	using rule_id_t = uint16_t;

	template<typename T>
	class Range1d {
	public:
		T low, high;
		Range1d(T low, T high): low(low), high(high) {}
		/*
		 * @note overlap not checked
		 * */
		bool operator<(const Range1d & other) const {
			return high <= other.low;
		}
		bool operator>(const Range1d & other) const {
			return low >= other.high;
		}
	};

	using rule_spec_t = std::pair<std::array<Range1d<uint32_t>, 1>, rule_id_t>;
	using value_t = uint32_t;
	using index_t = uint16_t;

	static const index_t INVALID_INDEX;
	static const rule_id_t INVALID_RULE;


	class Node: public ObjectWithStaticMempool<Node, 256, false> {
	public:
		// perf-critical: ensure this is 64-byte aligned.
		// 8*4B values
		__m256i keys[2];
		// 8*1B dimension index
		__m64 dim_index;
		// the value of rule
		std::array<index_t, 8> value;
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
			std::fill(child_index.begin(), child_index.end(), INVALID_INDEX);
			set_key_cnt(0);
			is_leaf = true;
			parent_index = INVALID_INDEX;
		}
		template<typename T>
		Range1d<T> get_key(uint8_t index) {
			assert(sizeof(T) == sizeof(uint32_t));
			auto low = reinterpret_cast<uint32_t*>(&keys[0])[index];
			auto high = reinterpret_cast<uint32_t*>(&keys[1])[index];
			return Range1d<T>(low, high);
		}
		template<typename T>
		void set_key(uint8_t index, Range1d<T> key) {
			assert(sizeof(T) == sizeof(uint32_t));
			reinterpret_cast<uint32_t*>(&keys[0])[index] = key.low;
			reinterpret_cast<uint32_t*>(&keys[1])[index] = key.high;
		}
		void set_child(unsigned index, Node * child);
		// A utility function to split the child y of this node
		// Note that y must be full when this function is called
		void splitChild(int i, Node & y);

		void set_key_cnt(size_t key_cnt);
		// A utility function to insert a new key in this node
		// The assumption is, the node must be non-full when this
		// function is called
		void insertNonFull(const rule_spec_t & rule);


		static Node & by_index(const index_t index);
		Node & child(const index_t index);
	};

	Node * root;

	BTree() :
			root(nullptr) {
	}

	/*
	 * @return rule index or the INVALID_RULE constant
	 * */
	rule_id_t search(const value_t & val);

	/*
	 * @return true if the rule was removed
	 * */
	bool discard(const rule_spec_t & rule);
	void insert(const rule_spec_t & rule);

	// returns from 0 (if value < keys[0]) to 16 (if value >= keys[15])
	static unsigned search_avx2(const Node & node, __m256i   const value);
};
