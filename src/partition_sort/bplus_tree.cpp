#include "bplus_tree.h"
using namespace std;

void print_avx2_hex256(__m256i ymm) {
	array<uint64_t, sizeof(__m256i) / sizeof(u_int32_t)> buffer;
	_mm256_storeu_si256((__m256i*)&buffer[0], ymm);
	for (auto i: buffer) {
		std::cout << i << " ";
	}
}

// https://www.geeksforgeeks.org/b-tree-set-1-insert-2/
bool BPlusTree::add(const rule_spec_t & rule) {

}


/*
 * 8*32b a > b
 *
 * @param a the signed integer vector
 * @param b the unsigned integer vector
 * */
inline __m256i    __attribute__((__always_inline__))
   _mm256_cmpgt_epu32(
		__m256i  const a, __m256i    const b) {
	constexpr uint32_t offset = 0x1 << 31;
	__m256i  const fix_val = _mm256_set1_epi32(offset);
	return _mm256_cmpgt_epi32(_mm256_add_epi32(a, fix_val), b); // PCMPGTD
}

BPlusTree::rule_id_t BPlusTree::search(const value_t & val) {
	Node * n = root;
	if (n) {
		__m256i formated_val = _mm256_set1_epi32(val);
		auto next_child_i = search_avx2(*n, formated_val);
		auto next_absolute_i = n->child_index[next_child_i];
		if (n->is_leaf)
			return next_absolute_i;
		else
			n = reinterpret_cast<Node*>(Node::_Mempool_t::getById(
					size_t(next_absolute_i)));
	}

	return INVALID_RULE;
}

//void preprocess_avx2(union b_node* const node) {
//	__m256i   const perm_mask = _mm256_set_epi32(3, 2, 1, 0, 7, 6, 5, 4);
//	__m256i * const middle = (__m256i *) &node->i32[4];
//
//	__m256i x = _mm256_loadu_si256(middle);
//	x = _mm256_permutevar8x32_epi32(x, perm_mask);
//	_mm256_storeu_si256(middle, x);
//}

unsigned BPlusTree::search_avx2(const Node & node, __m256i  const value) {
	// compare the two halves of the cache line.
	// load 256b to avx
	__m256i cmp1 = _mm256_load_si256(&node.keys[0]);
	__m256i cmp2 = _mm256_load_si256(&node.keys[1]);

	// 8* > u32
	cmp1 = _mm256_cmpgt_epu32(cmp1, value);
	cmp2 = _mm256_cmpgt_epu32(cmp2, value);

	// merge the comparisons back together.
	//
	// a permute is required to get the pack results back into order
	// because AVX-256 introduced that unfortunate two-lane interleave.
	//
	// alternately, you could pre-process your data to remove the need
	// for the permute.

	__m256i  const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
	__m256i cmp = _mm256_packs_epi32(cmp1, cmp2); // PACKSSDW
	cmp = _mm256_permutevar8x32_epi32(cmp, perm_mask); // PERMD

	// finally create a move mask and count trailing
	// zeroes to get an index to the next node.
	uint32_t mask = _mm256_movemask_epi8(cmp); // PMOVMSKB
	return _tzcnt_u32(mask) / 2; // TZCNT
}
