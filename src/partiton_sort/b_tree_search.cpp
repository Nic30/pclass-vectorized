#include <pcv/partiton_sort/b_tree.h>
#include <pcv/partiton_sort/b_tree_search.h>

using namespace std;

namespace pcv {

/*
 * 8*32b a > b
 *
 * @param a the signed integer vector
 * @param b the unsigned integer vector
 * */
inline __m256i __attribute__((__always_inline__)) _mm256_cmpgt_epu32(
		__m256i const a, __m256i const b) {
	constexpr uint32_t offset = 0x1 << 31;
	__m256i const fix_val = _mm256_set1_epi32(offset);
	return _mm256_cmpgt_epi32(_mm256_add_epi32(a, fix_val), b); // PCMPGTD
}

//void preprocess_avx2(union b_node* const node) {
//	__m256i   const perm_mask = _mm256_set_epi32(3, 2, 1, 0, 7, 6, 5, 4);
//	__m256i * const middle = (__m256i *) &node->i32[4];
//
//	__m256i x = _mm256_loadu_si256(middle);
//	x = _mm256_permutevar8x32_epi32(x, perm_mask);
//	_mm256_storeu_si256(middle, x);
//}

SearchResult search_seq(const BTree::Node & node,
		const Range1d<BTree::value_t> val) {
	SearchResult r;
	for (r.val_index = 0; r.val_index < node.key_cnt; r.val_index++) {
		BTree::KeyInfo cur = node.get_key<uint32_t>(r.val_index);
		if (val < cur.key.low) {
			break;
		} else if (cur.key == val) {
			r.in_range = true;
			break;
		}
	}
	return r;
}

SearchResult search_seq(const BTree::Node & node, BTree::value_t val) {
	SearchResult r;
	for (r.val_index = 0; r.val_index < node.key_cnt; r.val_index++) {
		BTree::KeyInfo cur = node.get_key<uint32_t>(r.val_index);
		if (val < cur.key.low) {
			break;
		} else if (cur.in_range(val)) {
			r.in_range = true;
			break;
		}
	}
	return r;
}

SearchResult search_avx2(const BTree::Node & node, BTree::value_t val) {
	__m256i value = _mm256_set1_epi32(val);
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

	__m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
	__m256i cmp = _mm256_packs_epi32(cmp1, cmp2); // PACKSSDW
	cmp = _mm256_permutevar8x32_epi32(cmp, perm_mask); // PERMD

	// finally create a move mask and count trailing
	// zeroes to get an index to the next node.
	uint32_t mask = _mm256_movemask_epi8(cmp); // PMOVMSKB
	auto next_index = _tzcnt_u32(mask) / 2; // TZCNT

	SearchResult r;
	r.val_index = next_index;
	r.in_range = false; // [TODO]
	assert(r.val_index < BTree::Node::MAX_DEGREE + 1);
	return r;
}


}
