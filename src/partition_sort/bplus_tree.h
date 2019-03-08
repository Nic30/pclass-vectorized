#pragma once
#include <iostream>
#include <immintrin.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <stdio.h>
#include <array>
#include <limits>

#include "mempool.h"

// https://stackoverflow.com/questions/24594026/initialize-m256i-from-64-high-or-low-bits-of-four-m128i-variables
// https://openproceedings.org/EDBT/2014/paper_107.pdf
class BPlusTree {
public:
	using rule_id_t = uint16_t;
	using rule_spec_t = std::pair<std::array<uint32_t, 2>, rule_id_t>;
	using value_t = uint32_t;
	using index_t = uint16_t;

	static constexpr index_t INVALID_INDEX =
			std::numeric_limits<uint16_t>::max();
	static constexpr rule_id_t INVALID_RULE = INVALID_INDEX;

	class Node: public ObjectWithStaticMempool<Node, 256, false> {
	public:
		// perf-critical: ensure this is 64-byte aligned. (a full cache line)
		// 8*4B values
		__m256i keys[2];
		// 8*1B dimension index
		__m64 dim_index;
		// 9*2B child index
		std::array<index_t, 9> child_index;
		// if bit in mask is set the key is present
		uint32_t key_mask;
		bool is_leaf;
		uint16_t parent_index;

		Node() {
			keys[0] = keys[1] = _mm256_set1_epi32(
					std::numeric_limits<uint32_t>::max());
			dim_index = _m_from_int64(std::numeric_limits<uint64_t>::max());
			std::fill(child_index.begin(), child_index.end(), INVALID_INDEX);
			key_mask = 0;
			is_leaf = true;
			parent_index = INVALID_INDEX;
		}
	};
	//static_assert(sizeof(Node) == 2*64 - sizeof(void*));

	Node * root;

	BPlusTree() :
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

	/*
	 * @return add true if the rule was inserted
	 **/
	bool add(const rule_spec_t & rule);

	// returns from 0 (if value < keys[0]) to 16 (if value >= keys[15])
	static unsigned search_avx2(const Node & node, __m256i   const value);
};
